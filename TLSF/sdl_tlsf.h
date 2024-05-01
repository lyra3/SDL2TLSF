//
// Created by bee on 4/4/24.
//

#ifndef TLSF_SDL_TLSF_H
#define TLSF_SDL_TLSF_H

#include "tlsf.h"
#include "SDL/include/SDL3/SDL.h"

typedef void* tlsf_instance;
typedef void* tlsf_pool;

// The current instance of tlsf
extern tlsf_instance active_instance;

// Setups the tlsf instance
void sdl_tlsf_init();

// ###### INSTANCE MANAGEMENT ######

// Creates a new instance of tlsf  (you can treat these as memory pools for specific data structures)
tlsf_instance sdl_tlsf_create_instance();

// Gets the current active instance of tlsf
tlsf_instance sdl_tlsf_get_instance();

// Sets the instance of tlsf, enables the user to change which "pool" memory is accessed from.
// For example if they wanted a specific pool for a specific data structure.
void sdl_tlsf_set_instance(tlsf_instance instance);

// ###### INSTANCE MEMORY MANAGEMENT ######

// Creates a memory pool within the instance
// So this adds a memory pool to the current instance.
tlsf_pool sdl_tlsf_add_memory(size_t bytes);

// Destroys that memory pool within the instance.
void sdl_tlsf_destroy_memory(tlsf_pool pool);




// Memory allocation functions
void *sdl_tlsf_malloc(size_t bytes);
void sdl_tlsf_free(void *ptr);
void *sdl_tlsf_calloc(size_t nmemb, size_t size);
void *sdl_tlsf_realloc(void *ptr, size_t size);


#endif //TLSF_SDL_TLSF_H
