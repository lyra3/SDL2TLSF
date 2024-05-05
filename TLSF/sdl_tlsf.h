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

	size_t pool_id;

	// Memory Pool
	pool_t pool; // Initialized with tlsf_create_pool
	size_t bytes; //
	size_t used;

	// Actual Memory
	void *mem;

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

	size_t num_pools;
	size_t pool_size;

	size_t total_size;
	size_t total_used;

	// TODO: Explore Thread Specific Instances for shits and giggles
	// SDL_Mutex *lock;

} tlsf_instance;

//size_t base_pool_size = 1 << 20;

// The current instance of tlsf
extern tlsf_instance *active_instance;
extern tlsf_instance *base_instance;

// ###### INITIALIZATION ######
// Setups the base tlsf instance, provide initial memory size
void sdl_tlsf_init();

// Setups the tlsf with a specific memory size
void sdl_tlsf_init_with_size(size_t bytes);

// Setups the mutex for tlsf
void sdl_tlsf_init_mutex();

void sdl_tlsf_quit();

// ###### INSTANCE MANAGEMENT ######

// Creates a new instance of tlsf  (you can treat these as memory pools for specific data structures)
tlsf_instance *sdl_tlsf_create_instance(size_t bytes);

// Gets the current active instance of tlsf
tlsf_instance *sdl_tlsf_get_instance();

// Sets the instance of tlsf, enables the user to change which "pool" memory is accessed from.
// For example if they wanted a specific pool for a specific data structure.
void sdl_tlsf_set_instance(tlsf_instance *instance);

// Sets the instance of tlsf to the base instance, returns the previous instance
tlsf_instance *sdl_tlsf_rebase_instance();

// Destroys that memory pool within the instance.
void sdl_tlsf_destroy_instance(tlsf_instance *instance);

void sdl_tlsf_print_instance(tlsf_instance *instance);

// ###### INSTANCE LOCAL MEMORY MANAGEMENT ######
// Used within a tlsf instance to add a memory pool
// Could be called outside if you want to add memory pools ahead of allocation
void sdl_tlsf_add_pool();
void sdl_tlsf_free_pool(tlsf_pool *pool);
void sdl_tlsf_free_pool_mem(tlsf_pool *pool);

// Memory allocation functions, overloads the SDL memory functions
void *sdl_tlsf_malloc(size_t bytes);
void sdl_tlsf_free(void *ptr);
void *sdl_tlsf_calloc(size_t nmemb, size_t size);
void *sdl_tlsf_realloc(void *ptr, size_t size);


// Debugging, returns 0 if no errors
int sdl_tlsf_check_active_instance();
int sdl_tlsf_check_pool(pool_t pool);

// Helper Gadgets
// Gets the  pool based on the address of the pointer
tlsf_pool *sdl_tlsf_get_pool(size_t ptr_addr);

#endif //TLSF_SDL_TLSF_H
