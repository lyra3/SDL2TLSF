//
// Created by bee on 4/4/24.
//

// ls /proc

#include "sdl_tlsf.h"

const size_t base_pool_size = 1 << 20;

SDL_Mutex *tlsf_lock = NULL;

tlsf_instance active_instance;

// Connects the tlsf instance to SDL's memory functions
void sdl_tlsf_init() {

	sdl_tlsf_init_with_size(base_pool_size);
}

void sdl_tlsf_init_with_size(size_t bytes) {

	if (tlsf_lock != NULL)
		SDL_LockMutex(tlsf_lock);

	active_instance = sdl_tlsf_create_instance(bytes);
	SDL_SetMemoryFunctions(sdl_tlsf_malloc, sdl_tlsf_calloc, sdl_tlsf_realloc, sdl_tlsf_free);



	if (tlsf_lock != NULL)
		SDL_UnlockMutex(tlsf_lock);
	else
		tlsf_lock = SDL_CreateMutex();

}


void sdl_tlsf_init_mutex() {
	tlsf_lock = SDL_CreateMutex();
	if (tlsf_lock == NULL) {
		SDL_Log("Failed to create mutex\n");
	}
}

void sdl_tlsf_destroy_mutex() {
	SDL_DestroyMutex(tlsf_lock);
	tlsf_lock = NULL;
}

tlsf_instance sdl_tlsf_create_instance(size_t bytes) {

	// Create some memory for the tlsf instance
	void *mem = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mem == MAP_FAILED) {
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create memory for tlsf instance\n");
	}

	active_instance.instance = tlsf_create_with_pool(mem, bytes);

	size_t pool_size = bytes - tlsf_pool_overhead();

	tlsf_pool *pool = tlsf_malloc(active_instance.instance,sizeof(tlsf_pool));
	pool -> pool = tlsf_get_pool(active_instance.instance);
	pool -> bytes = pool_size;
	pool -> used = sizeof(tlsf_pool);

	// Address Range
	pool -> start = mem;
	pool -> end = (char *)pool -> start + pool_size;

	// No previous or next pool
	pool -> next = NULL;
	pool -> prev = NULL;

	// Create a new pool list
	tlsf_pool_list poolList;
	poolList.header = pool;
	poolList.tail = pool;

	// Assign the pool list to the tlsf instance
	active_instance.tlsf_pools = poolList;

	// This is how we know if we need to add more memory to the pool
	active_instance.total_size = pool_size;
	active_instance.total_used = sizeof(tlsf_pool);

	active_instance.pool_size = base_pool_size;

	return active_instance;
}

void sdl_tlsf_destroy_instance() {

	SDL_DestroyMutex(tlsf_lock);
	tlsf_lock = NULL;
//	if (active_instance.instance != NULL) {
//		tlsf_destroy(active_instance.instance);
//		munmap(tlsf_get_pool(active_instance.instance), active_instance.bytes);
//	}
}

void *sdl_tlsf_malloc(size_t bytes) {

//	SDL_Log("Malloc Called on %ld bytes\n", bytes);

	SDL_LockMutex(tlsf_lock);
	if (tlsf_lock == NULL) {
		SDL_Log("Not Locked\n");
	}





	// Check if we have enough memory to allocate
	SDL_Log("Total Size: %ld\n", active_instance.total_size);
	SDL_Log("Total Used: %ld\n", active_instance.total_used);
	SDL_Log("Extra Bytes: %ld\n", active_instance.total_size - active_instance.total_used);
	while (active_instance.total_size - active_instance.total_used < bytes) {

		// Add another pool to the instance
		sdl_tlsf_add_pool();
	}

	SDL_Log("Malloc Called on %ld bytes\n", bytes);
	void *ptr = tlsf_malloc(active_instance.instance, bytes);

	// If we successfully allocated memory
	if (ptr) {

		// Loop through the pool list and check if the pointer is within the range of the pool
//		tlsf_pool pool = sdl_tlsf_get_pool((size_t)ptr);
//		if (pool.pool != NULL) {
//			pool.used += bytes;
//		}

		active_instance.total_used += bytes;

	} else {
		SDL_Log("Failed to allocate memory\n");
		return NULL;

	}



	SDL_UnlockMutex(tlsf_lock);
	return ptr;
}

void sdl_tlsf_free(void *ptr) {

//	SDL_Log("Free Called on %p\n", ptr);

	SDL_LockMutex(tlsf_lock);



	// TODO: Check if this is the last block in a pool
	// And if so, we can remove the pool

	tlsf_free(active_instance.instance, ptr);


	SDL_UnlockMutex(tlsf_lock);
}

void *sdl_tlsf_calloc(size_t nmemb, size_t size) {



	SDL_LockMutex(tlsf_lock);



	void *ptr = tlsf_calloc(active_instance.instance, size, nmemb);

	SDL_UnlockMutex(tlsf_lock);

	return ptr;
}

void *sdl_tlsf_realloc(void *ptr, size_t size) {



	SDL_LockMutex(tlsf_lock);



	void *ret_ptr = tlsf_realloc(active_instance.instance, ptr, size);

	SDL_UnlockMutex(tlsf_lock);

	return ret_ptr;
}

int sdl_tlsf_check_active_instance() {

	SDL_LockMutex(tlsf_lock);

	int ret_val = tlsf_check(active_instance.instance);

	SDL_UnlockMutex(tlsf_lock);

	return ret_val;
}

int sdl_tlsf_check_pool(pool_t pool) {

	SDL_LockMutex(tlsf_lock);

	int ret_val = tlsf_check_pool(pool);

	SDL_UnlockMutex(tlsf_lock);

	return ret_val;
}

// Loop through the pool list and check if the pointer is within the range of the pool
tlsf_pool sdl_tlsf_get_pool(size_t ptr_addr) {

	SDL_LockMutex(tlsf_lock);

	tlsf_pool_list poolList = active_instance.tlsf_pools;
	tlsf_pool *pool = poolList.header;

	while (pool != NULL) {
		if (ptr_addr >= (size_t)pool -> start && ptr_addr <= (size_t)pool -> end) {

			SDL_UnlockMutex(tlsf_lock);
			return *pool;
		}
		pool = pool -> next;
	}

	tlsf_pool empty_pool;
	empty_pool.pool = NULL;

	SDL_UnlockMutex(tlsf_lock);

	return empty_pool;


}

void sdl_tlsf_add_pool() {

	SDL_Log("Adding Pool\n");

	SDL_LockMutex(tlsf_lock);

	size_t pool_size = active_instance.pool_size;

	void *ptr = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED) {
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create memory for tlsf instance\n");
	}

	SDL_Log("Ptr: %p\n", ptr);

	tlsf_pool *pool = sdl_tlsf_malloc(sizeof(tlsf_pool));
	if (pool == NULL) {
		SDL_Log("Failed to allocate memory for pool\n");
	}


	pool -> pool = tlsf_add_pool(active_instance.instance, ptr, pool_size);
	if (pool -> pool == NULL) {
		SDL_Log("Failed to add pool\n");
	}

	//
	pool -> bytes = pool_size;
	pool -> used = sizeof(tlsf_pool);

	// Address Range
	pool -> start = ptr;
	pool -> end = (char *)ptr + (pool_size);

	// Get last pool
	tlsf_pool *node = active_instance.tlsf_pools.tail;


	// Add the new pool to the end of the list
	node->next = pool;

	// Set the new pool's previous to the last pool
	pool->next = NULL;
	pool->prev = node;


	// Set the new pool as the tail of the list
	active_instance.tlsf_pools.tail = pool;

	// Increase the total size of the instance based on the pool size
	active_instance.total_size += pool->bytes;

	SDL_Log("Total Size: %ld\n", active_instance.total_size);

	// Cause the debugger is fucking stupid
	tlsf_instance temp_instance = active_instance;
	SDL_Log("Bleh");

	SDL_UnlockMutex(tlsf_lock);
}