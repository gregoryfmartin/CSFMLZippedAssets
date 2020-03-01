#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <zip.h>

#include <SFML/System.h>
#include <SFML/Window.h>
#include <SFML/Graphics.h>

///////////////////////////////////////////////////////////////////////////////
// Typedefs
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Texture Map Entry
//
// String/Texture Pair
///////////////////////////////////////////////////////////////////////////////
typedef struct {
    const char* name;
    sfTexture * texture;
} texture_map_entry_t;

///////////////////////////////////////////////////////////////////////////////
// Texture Map
//
// Collection of Texture Map Entries
///////////////////////////////////////////////////////////////////////////////
typedef struct {
    texture_map_entry_t** entries;
    int size;
    int count;
} texture_map_t;




///////////////////////////////////////////////////////////////////////////////
// Global Variables
///////////////////////////////////////////////////////////////////////////////
zip_t         * archive_file    = NULL;
texture_map_t * texture_listing = NULL;
sfRenderWindow* game_window     = NULL;
sfSprite      * sprite          = NULL;




///////////////////////////////////////////////////////////////////////////////
// Texture Map (Vector) Functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// texture_map_init
///////////////////////////////////////////////////////////////////////////////
void texture_map_init (texture_map_t* tmap) {
    tmap->entries = NULL;
    tmap->size    = 0;
    tmap->count   = 0;
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_create
///////////////////////////////////////////////////////////////////////////////
texture_map_t* texture_map_create () {
    texture_map_t* tmap = malloc (sizeof (texture_map_t*));
    texture_map_init (tmap);
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_count
///////////////////////////////////////////////////////////////////////////////
int texture_map_count (texture_map_t* tmap) {
    return tmap->count;
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_add
///////////////////////////////////////////////////////////////////////////////
void texture_map_add (texture_map_t* tmap, texture_map_entry_t* entry) {
    // Determine if the entry name already exists, disallow adding if it already does
    for (int i = 0; i < tmap->count; i++) {
        if (tmap->entries[i]->name == entry->name) {
            return;
        }
    }

    if (tmap->size == 0) {
        tmap->size    = 10;
        tmap->entries = malloc (sizeof (texture_map_entry_t*) * tmap->size);
        memset (tmap->entries, '\0', sizeof (texture_map_entry_t*) * tmap->size);
    }

    if (tmap->size == tmap->count) {
        tmap->size *= 2;
        tmap->entries = realloc (tmap->entries, sizeof (texture_map_entry_t*) * tmap->size);
    }

    tmap->entries[tmap->count] = malloc (sizeof (texture_map_entry_t*));
    tmap->entries[tmap->count] = entry;
    tmap->count++;
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_set
///////////////////////////////////////////////////////////////////////////////
void texture_map_set (texture_map_t* tmap, int index, texture_map_entry_t* entry) {
    if (index >= tmap->count) {
        return;
    }
    tmap->entries[index] = entry;
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_get_index
///////////////////////////////////////////////////////////////////////////////
texture_map_entry_t* texture_map_get_index (texture_map_t* tmap, int index) {
    if (index > tmap->count) {
        return NULL;
    }

    return tmap->entries[index - 1];
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_get_name
///////////////////////////////////////////////////////////////////////////////
texture_map_entry_t* texture_map_get_name (texture_map_t* tmap, const char* name) {
    for (int i = 0; i < tmap->count; i++) {
        if (tmap->entries[i]->name == name) {
            return tmap->entries[i];
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_delete_index
///////////////////////////////////////////////////////////////////////////////
void texture_map_delete_index (texture_map_t* tmap, int index) {
    if (index >= tmap->count) {
        return;
    }

    tmap->entries[index] = NULL;

    int i, j;
    texture_map_entry_t** newarr = (texture_map_entry_t**) malloc (sizeof (texture_map_entry_t*) * tmap->count * 2);
    for (i = 0, j = 0; i < tmap->count; i++) {
        if (tmap->entries[i] != NULL) {
            newarr[j] = tmap->entries[i];
            j++;
        }
    }

    free (tmap->entries);
    tmap->entries = newarr;
    tmap->count--;
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_delete_name
///////////////////////////////////////////////////////////////////////////////
void texture_map_delete_name (texture_map_t* tmap, const char* name) {
    unsigned char name_found = 0;

    for (int i = 0; i < tmap->count; i++) {
        if (tmap->entries[i]->name == name) {
            name_found = 1;
            tmap->entries[i] = NULL;
        }
        if (name_found) {
            break;
        }
    }

    if (name_found) {
        int a, b = 0;
        texture_map_entry_t** newarr = (texture_map_entry_t**) malloc (sizeof (texture_map_entry_t*) * tmap->count * 2);
        for (a = 0, b = 0; a < tmap->count; a++) {
            if (tmap->entries[a] != NULL) {
                newarr[b] = tmap->entries[a];
                b++;
            }
        }

        free (tmap->entries);
        tmap->entries = newarr;
        tmap->count--;
    } else {
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_free
///////////////////////////////////////////////////////////////////////////////
void texture_map_free (texture_map_t* tmap) {
    free (tmap->entries);
    free (tmap);
}




///////////////////////////////////////////////////////////////////////////////
// Texture Map Entry Functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// texture_map_entry_init
///////////////////////////////////////////////////////////////////////////////
void texture_map_entry_init (texture_map_entry_t* tme) {
    tme->name    = NULL;
    tme->texture = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// texture_map_entry_create
///////////////////////////////////////////////////////////////////////////////
texture_map_entry_t* texture_map_entry_create (const char* n, sfTexture* t) {
    texture_map_entry_t* tme = malloc (sizeof (texture_map_entry_t));
    texture_map_entry_init (tme);
    tme->name    = n;
    tme->texture = t;

    return tme;
}




///////////////////////////////////////////////////////////////////////////////
// Zip Archive Functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// open_zip_file
///////////////////////////////////////////////////////////////////////////////
zip_t* open_zip_file (const char* file, int* err) {
    return zip_open (file, ZIP_CHECKCONS | ZIP_RDONLY, err);
}

///////////////////////////////////////////////////////////////////////////////
// close_zip_file
///////////////////////////////////////////////////////////////////////////////
void close_zip_file (zip_t* ar) {
    int r = zip_close (ar);
    if (0 > r) {
        zip_error_t* err = zip_get_error (ar);
        printf ("Failed to close the specified archive file! The following error codes were generated: %i, %i\n", err->zip_err, err->sys_err);
        exit (err->zip_err);
    }
}

///////////////////////////////////////////////////////////////////////////////
// populate_texture_map
///////////////////////////////////////////////////////////////////////////////
void populate_texture_map (zip_t* ar, texture_map_t* tmap) {
    zip_int64_t      arentries = zip_get_num_entries (ar, ZIP_FL_UNCHANGED);
    for (zip_int64_t a = 0; a < arentries; a++) {
        zip_stat_t estat;
        int        r = zip_stat_index (ar, a, ZIP_FL_UNCHANGED, &estat);

        // Check to see if there were any issues getting stats for the entry
        if (0 > r) {
            zip_error_t* err = zip_get_error (ar);
            printf ("Error reading from archive file! The following codes were generated: %i, %i\n",
                    err->zip_err,
                    err->sys_err);
            exit (err->zip_err);
        }

        // Read the raw data from the entry to determine the type of assert we've encountered
        if (strstr (estat.name, ".png") != NULL || strstr (estat.name, ".jpg") != NULL) {
            // We've likely found an image file
            // This is where shit is really going to get messy
            void      * buf = malloc (sizeof (void*) * estat.size + 1); // Create a buffer for data from the file
            zip_file_t* zf  = zip_fopen_index (ar, a, ZIP_FL_UNCHANGED); // Open the file at the current index
            zip_int64_t ra = zip_fread (zf, buf, estat.size); // Read the bits from the file into the buffer using the decompressed size as a delimiter
            texture_map_entry_t* ent = texture_map_entry_create (estat.name, sfTexture_createFromMemory (buf, estat.size, NULL)); // Create a texture from the data buffer
            texture_map_add (tmap, ent); // Add the entry to the listing
        }
    }
    close_zip_file (ar);
}


///////////////////////////////////////////////////////////////////////////////
// CSFML Functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// csfml_init
///////////////////////////////////////////////////////////////////////////////
void csfml_init () {
    sfVideoMode vm = sfVideoMode_getDesktopMode();
    vm.width = 800;
    vm.height = 600;

    game_window = sfRenderWindow_create (vm, "CSFML Zipped Assets Demo", sfTitlebar | sfClose, NULL);
    sfRenderWindow_setFramerateLimit (game_window, 60);
    sfRenderWindow_setVerticalSyncEnabled (game_window, sfTrue);
}


///////////////////////////////////////////////////////////////////////////////
// General Utility Functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// cleanup
///////////////////////////////////////////////////////////////////////////////
void cleanup () {
    // This function is already called in a different function
    // If it's called twice, the program will segfault
    //close_zip_file(archive_file);
    texture_map_free (texture_listing);
    sfSprite_destroy (sprite);
    sfRenderWindow_destroy (game_window);

    archive_file = NULL;
    texture_listing = NULL;
    sprite = NULL;
    game_window = NULL;
}

int main (int argc, char* argv[]) {
    // Initialize CSFML
    csfml_init ();

    // Attempt to open the archive file
    int zerr;
    archive_file = open_zip_file ("./assets.zip", &zerr);
    if (archive_file == NULL) {
        printf ("Failed opening the assets zip file! The following error codes were generated: %i\n", zerr);
        exit (zerr);
    }

    // Attempt to populate the Texture Listing
    texture_listing = texture_map_create ();
    populate_texture_map (archive_file, texture_listing);

    sprite = sfSprite_create ();
    sfSprite_setTexture (sprite, texture_map_get_index (texture_listing, 3)->texture, sfFalse);
    sfVector2f pos;
    pos.x = pos.y = 50.0f;
    sfSprite_setPosition (sprite, pos);

    // Game Loop
    while (sfRenderWindow_isOpen(game_window)) {
        sfEvent ev;

        while (sfRenderWindow_pollEvent (game_window, &ev)) {
            switch (ev.type) {
                case sfEvtClosed:
                    sfRenderWindow_close (game_window);
                    break;
            }
        }

        sfRenderWindow_clear (game_window, sfBlack);
        sfRenderWindow_drawSprite (game_window, sprite, NULL);
        sfRenderWindow_display (game_window);
    }

    cleanup();

    return 0;
}
