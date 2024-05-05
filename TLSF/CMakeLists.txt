cmake_minimum_required(VERSION 3.27)
project(TLSF C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")


### TLSF SETUP ###
# Create the TLFS static library
add_library(TLSF STATIC tlsf.c)


### GENERIC TEST ###
# Create the Test executable
add_executable(Generic_Test main.c)

# Link the TLSF library with the Test executable
target_link_libraries(Generic_Test TLSF)


### SDL3 SETUP ###
# Setup the SDL3 library
add_subdirectory(SDL)

# Grab the right headers
include_directories(SDL/include)


### SDL_TLSF COMPATIBILITY LAYER ###
# Create the SDL_TLSF static library
add_library(SDL_TLSF STATIC sdl_tlsf.c)

# Link the SDL3 library with the SDL_TLSF library
target_link_libraries(SDL_TLSF TLSF SDL3::SDL3)


### SDL TEST ###
# Setup the executable for the SDL Test
add_executable(SDL_Test main_sdl.c)

# Link the TLSF library with the Test executable
target_link_libraries(SDL_Test SDL_TLSF TLSF SDL3::SDL3)

### Vanilla SDL Test ###
add_executable(Vanilla_SDL vanilla_sdl.c)

# Link the SDL3 library with the Test executable
target_link_libraries(Vanilla_SDL SDL3::SDL3)
