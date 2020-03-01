cmake_minimum_required (VERSION 3.15)
project (zipped_assets C)

set (CMAKE_C_STANDARD 11)

add_executable (zipped_assets main.c)

target_link_libraries(zipped_assets zip)
target_link_libraries(zipped_assets csfml-system)
target_link_libraries(zipped_assets csfml-window)
target_link_libraries(zipped_assets csfml-graphics)

