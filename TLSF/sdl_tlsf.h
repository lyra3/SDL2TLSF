//
// Created by bee on 4/4/24.
//

#ifndef TLSF_SDL_TLSF_H
#define TLSF_SDL_TLSF_H

#include <sys/mman.h>

#include "tlsf.h"
#include "SDL/include/SDL3/SDL.h"

// Our Memory Pool
typedef struct tlsf_pool {

	// Memory Pool
	pool_t pool; // Initialized with tlsf_create_pool
	size_t bytes; //
	size_t used;

	// Address Range
	void* start; // from mmap
	void* end; // bytes + start

	// Doubly linked list
	struct tlsf_pool *next;
	struct tlsf_pool *prev;

} tlsf_pool;

// List of memory pools
typedef struct {
	tlsf_pool *header;
	tlsf_pool *tail;
} tlsf_pool_list;

// Our TLSF Instance, contains the tlsf instance and the memory pool list
typedef struct {

	tlsf_t instance;
	tlsf_pool_list tlsf_pools;

	size_t pool_size;

	size_t total_size;
	size_t total_used;

} tlsf_instance;

//size_t base_pool_size = 1 << 20;

// The current instance of tlsf
extern tlsf_instance active_instance;

// Setups the tlsf instance, provide initial memory size
void sdl_tlsf_init();

void sdl_tlsf_init_mutex();
void sdl_tlsf_destroy_mutex();

// Setups the tlsf with a specific memory size
void sdl_tlsf_init_with_size(size_t bytes);

// ###### INSTANCE MANAGEMENT ######

// Creates a new instance of tlsf  (you can treat these as memory pools for specific data structures)
tlsf_instance sdl_tlsf_create_instance(size_t bytes);

// Gets the current active instance of tlsf
tlsf_instance sdl_tlsf_get_instance();

// Sets the instance of tlsf, enables the user to change which "pool" memory is accessed from.
// For example if they wanted a specific pool for a specific data structure.
void sdl_tlsf_set_instance(tlsf_instance instance);

// ###### INSTANCE MEMORY MANAGEMENT ######

// Creates a memory pool within the instance
// So this adds a memory pool to the current instance.
// /* Add/remove memory pools. */
// pool_t tlsf_add_pool(tlsf_t tlsf, void* mem, size_t bytes);
// void tlsf_remove_pool(tlsf_t tlsf, pool_t pool);
void sdl_tlsf_add_pool();
void sdl_tlsf_remove_pool();

// Destroys that memory pool within the instance.
void sdl_tlsf_destroy_instance();




// Memory allocation functions
void *sdl_tlsf_malloc(size_t bytes);
void sdl_tlsf_free(void *ptr);
void *sdl_tlsf_calloc(size_t nmemb, size_t size);
void *sdl_tlsf_realloc(void *ptr, size_t size);


// Debugging, returns 0 if no errors
int sdl_tlsf_check_active_instance();
int sdl_tlsf_check_pool(pool_t pool);

// Helper Gadgets

// Gets the  pool based on the address of the pointer
tlsf_pool sdl_tlsf_get_pool(size_t ptr_addr);

#endif //TLSF_SDL_TLSF_H
