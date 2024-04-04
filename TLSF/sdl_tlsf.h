//
// Created by bee on 4/4/24.
//

#ifndef TLSF_SDL_TLSF_H
#define TLSF_SDL_TLSF_H

#include "tlsf.h"
#include "SDL/include/SDL3/SDL.h"

typedef void* tlsf_instance;
typedef void* tlsf_pool;

// This
void sdl_tlsf_init(void);

// Creates a new instance of tlsf
tlsf_instance sdl_tlsf_create_instance();

// Gets the current instance of tlsf
tlsf_instance sdl_tlsf_get_current_instance(void);

// Sets the current instance of tlsf
void sdl_tlsf_set_instance(tlsf_instance instance);

// Creates a memory pool within the current instance
tlsf_pool sdl_tlsf_create_pool(void *mem, size_t bytes);



// Memory allocation functions
void *sdl_tlsf_malloc(size_t bytes);
void sdl_tlsf_free(void *ptr);
void *sdl_tlsf_calloc(size_t nmemb, size_t size);
void *sdl_tlsf_realloc(void *ptr, size_t size);


#endif //TLSF_SDL_TLSF_H
