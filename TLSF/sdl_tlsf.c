//
// Created by bee on 4/4/24.
//

// ls /proc

#include "sdl_tlsf.h"


// Built-in Valgrind Memcheck
#include <valgrind/memcheck.h>


// Base Pool Size: 16 MB
const size_t base_pool_size = (1 << 20) * 16;

SDL_Mutex *tlsf_lock = NULL;

tlsf_instance *active_instance;
tlsf_instance *base_instance;

// Used to keep track of the pool id
size_t pool_id_counter = 0;

// Connects the tlsf instance to SDL's memory functions
void sdl_tlsf_init() {

	sdl_tlsf_init_with_size(base_pool_size);
}

void sdl_tlsf_init_with_size(size_t bytes) {

	pool_id_counter = 0;

	// Override SDL's memory functions
	SDL_SetMemoryFunctions(sdl_tlsf_malloc, sdl_tlsf_calloc, sdl_tlsf_realloc, sdl_tlsf_free);

	// Create the base instance and set it as the active instance
	base_instance = sdl_tlsf_create_instance(bytes);
	active_instance = base_instance;

	// Init our lock
	tlsf_lock = SDL_CreateMutex();
	if (tlsf_lock == NULL) {
		SDL_Log("Failed to create mutex\n");
	}

}

// Free the base instance
void sdl_tlsf_quit() {

	// Rebase the instance, just in case
	sdl_tlsf_rebase_instance();

	// Destroy active instance
	sdl_tlsf_destroy_instance(base_instance);

	// Honestly our best bet at destroying the mutex
	tlsf_lock = NULL;
}


tlsf_instance *sdl_tlsf_create_instance(size_t pool_size) {

	// Create some memory for the tlsf instance
	void *mem = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mem == MAP_FAILED) {
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create memory for tlsf instance\n");
	}

	// Notify Valgrind about the allocation
    VALGRIND_MALLOCLIKE_BLOCK(mem, pool_size, 0, 0);

	tlsf_instance new_instance;

	new_instance.instance = tlsf_create_with_pool(mem, pool_size);
	new_instance.num_pools = 1;

	size_t act_size = pool_size - tlsf_pool_overhead();

	// Create a new pool
	tlsf_pool *pool = tlsf_calloc(new_instance.instance, sizeof(tlsf_pool), 1);
	pool -> mem = mem;
	pool -> pool = tlsf_get_pool(new_instance.instance);
	pool -> bytes = pool_size;
	pool -> used = sizeof(tlsf_pool);

	pool_id_counter += 1;
	pool -> pool_id = pool_id_counter;


	// Address Range
	pool -> start = mem;
	pool -> end = (char *)pool -> start + act_size;

	// No previous or next pool
	pool -> next = NULL;
	pool -> prev = NULL;

	// Create a new pool list
	tlsf_pool_list poolList;
	poolList.header = pool;
	poolList.tail = pool;

	// Assign the pool list to the tlsf instance
	new_instance.tlsf_pools = poolList;

	// This is how we know if we need to add more memory to the pool
	new_instance.total_size = pool_size;
	new_instance.total_used = sizeof(tlsf_pool);

	new_instance.pool_size = pool_size;

	// Setup Lock
	// Store the current instance
	//	tlsf_instance *current_instance = sdl_tlsf_get_instance();
	//
	//	// Set the new instance as the active instance
	//	sdl_tlsf_set_instance(&new_instance);
	//
	//	// Use the new instance to create the lock
	//	new_instance.lock = SDL_CreateMutex();
	//	if (new_instance.lock == NULL) {
	//		SDL_Log("Failed to create mutex\n");
	//	}
	//
	//	// Set the instance back to the current instance
	//	sdl_tlsf_set_instance(current_instance);


	return new_instance;
}

tlsf_instance *sdl_tlsf_get_instance() {
	return &active_instance;
}

void sdl_tlsf_set_instance(tlsf_instance *instance) {
	SDL_LockMutex(tlsf_lock);
	active_instance = *instance;
	SDL_UnlockMutex(tlsf_lock);
}

tlsf_instance *sdl_tlsf_rebase_instance() {

	SDL_LockMutex(tlsf_lock);

	tlsf_instance *current_instance = sdl_tlsf_get_instance();
	active_instance = base_instance;

	SDL_UnlockMutex(tlsf_lock);
	return current_instance;
}

void sdl_tlsf_destroy_instance() {

	SDL_LockMutex(tlsf_lock);

	tlsf_instance current_instance = active_instance;

	tlsf_pool *head = active_instance.tlsf_pools.header;
	tlsf_pool *tail = active_instance.tlsf_pools.tail;


	tlsf_pool *free_pool = tail;

	// Go backwards through the pools and free them
	// But leave the head pool, for special handling
	while (free_pool != head) {

		if (free_pool -> pool_id == 2) {
			SDL_Log("Break\n");
		}

		SDL_Log("Freeing Pool: %zu\n", free_pool -> pool_id);
		tlsf_pool *prev = free_pool -> prev;
		sdl_tlsf_free_pool(free_pool);
		free_pool = prev;
	}

	// Free the last pool. From what I can gather
	// the last pool is kinda special and can't be freed as a pool
	pool_t pool_mem = head -> pool;
	size_t size = head -> bytes;
	void *mem = head -> mem;

	SDL_Log("Freeing Pool: %zu\n", head -> pool_id);

	// Free pool data structure from the pool itself (kinda wacky ik)
	tlsf_free(active_instance.instance, head);


	// Notify Valgrind that the pool is being freed
	VALGRIND_FREELIKE_BLOCK(mem, 0);

	// SysCall Free
	munmap(mem, size);


	// Mutex is init in the base instance, so you can't really access it anymore
	// But it's ok because you destroy the base instance last
	if (active_instance.instance != base_instance.instance) {
		SDL_UnlockMutex(tlsf_lock);
	}

}

void sdl_tlsf_print_instance(tlsf_instance instance) {

	SDL_LockMutex(tlsf_lock);

	tlsf_pool *pool = instance.tlsf_pools.header;

	while (pool != NULL) {
		SDL_Log("Pool ID: %zu\n", pool -> pool_id);
		SDL_Log("Pool Start: %p\n", pool -> start);
		SDL_Log("Pool End: %p\n", pool -> end);
		SDL_Log("Pool Bytes: %zu\n", pool -> bytes);
		SDL_Log("Pool Used: %zu\n", pool -> used);
		SDL_Log("\n");

		pool = pool -> next;
	}

	SDL_UnlockMutex(tlsf_lock);

}

void *sdl_tlsf_malloc(size_t bytes) {



	SDL_LockMutex(tlsf_lock);

	// Makes sure we are not allocating more memory than can fit in a pool
	if (bytes >= (active_instance.pool_size) - tlsf_pool_overhead()) {
		SDL_Log("Requested memory size is greater than pool size\n");

		SDL_UnlockMutex(tlsf_lock);
		return NULL;
	}


	// Check if we have enough memory to allocate
	if (active_instance.total_size - active_instance.total_used < bytes) {

		// Add another pool to the instance
		sdl_tlsf_add_pool();
	}

	void *ptr = tlsf_malloc(active_instance.instance, bytes);;

	if (!ptr) {

		// Test if the problem is having a contiguous block of memory
		sdl_tlsf_add_pool();
		ptr = tlsf_malloc(active_instance.instance, bytes);

		if (!ptr) {
			SDL_Log("Failed to allocate memory\n");
			SDL_UnlockMutex(tlsf_lock);
			return NULL;
		}
	}

	size_t block_size = tlsf_block_size(ptr);

	active_instance.total_used += block_size;

	// Update the pool list
	tlsf_pool *pool = sdl_tlsf_get_pool((size_t)ptr);
	if (pool == NULL) {
		SDL_Log("Failed to Assign Pool\n");
		SDL_UnlockMutex(tlsf_lock);
		return NULL;
	}
	pool -> used += block_size;


	SDL_UnlockMutex(tlsf_lock);
	return ptr;
}

void sdl_tlsf_free(void *ptr) {

	SDL_LockMutex(tlsf_lock);

	// Get the pool that the pointer is within
	tlsf_pool *pool = sdl_tlsf_get_pool((size_t) ptr);

	// Get the size of the freed block
	size_t block_size = tlsf_block_size(ptr);

	// Actually free the memory
	tlsf_free(active_instance.instance, ptr);

	// Update the pool list
	pool -> used -= block_size;


	// Update the total used memory
	active_instance.total_used -= block_size;

	// Check how much memory is left in the pool
	if (pool -> used <= 64 && active_instance.num_pools > 1) {

		SDL_Log("Freeing Tail");

		// Remove the pool
		sdl_tlsf_free_pool(pool);
	}


	SDL_UnlockMutex(tlsf_lock);
}

void *sdl_tlsf_calloc(size_t nmemb, size_t size) {

	SDL_LockMutex(tlsf_lock);

	size_t bytes = nmemb * size;

	// Check if we have enough memory to allocate
	if (active_instance.total_size - active_instance.total_used < bytes) {

		// Add another pool to the instance
		sdl_tlsf_add_pool();
	}

	void *ptr = tlsf_calloc(active_instance.instance, size, nmemb);

	if (!ptr) {

		// Test if the problem is having a contiguous block of memory
		sdl_tlsf_add_pool();
		ptr = tlsf_calloc(active_instance.instance, size, nmemb);

		if (!ptr) {
			SDL_Log("Failed to allocate memory\n");
			SDL_UnlockMutex(tlsf_lock);
			return NULL;
		}
	}

	size_t block_size = tlsf_block_size(ptr);

	active_instance.total_used += block_size;

	// Update the pool list
	tlsf_pool *pool = sdl_tlsf_get_pool((size_t)ptr);
	if (pool == NULL) {
		SDL_Log("Failed to Assign Pool\n");
		SDL_UnlockMutex(tlsf_lock);
		return NULL;
	}
	pool -> used += block_size;


	SDL_UnlockMutex(tlsf_lock);

	return ptr;
}

void *sdl_tlsf_realloc(void *ptr, size_t size) {

	SDL_LockMutex(tlsf_lock);

    // Get the current block size
    size_t current_size = ptr ? tlsf_block_size(ptr) : 0;

    // Check if downsizing or upsizing
    if (size > current_size) {
        // Make sure we are not reallocating more memory than can fit in a pool
        if (size >= (active_instance.pool_size - tlsf_pool_overhead())) {
            SDL_Log("Requested realloc size is greater than pool size\n");
            SDL_UnlockMutex(tlsf_lock);
            return NULL;
        }

        // Check if we need more total memory than available
        if (active_instance.total_size - active_instance.total_used < size - current_size) {
            sdl_tlsf_add_pool();  // Add another pool to the instance
        }
    }

    // Attempt to reallocate memory
    void *new_ptr = tlsf_realloc(active_instance.instance, ptr, size);
    if (!new_ptr && size > current_size) {
        // If realloc fails and it's a size increase, try adding a pool and reallocating
        sdl_tlsf_add_pool();
        new_ptr = tlsf_realloc(active_instance.instance, ptr, size);
    }

    // If still fails, or it's a decrease and failed, return NULL
    if (!new_ptr) {
        SDL_Log("Failed to reallocate memory\n");
        SDL_UnlockMutex(tlsf_lock);
        return NULL;
    }

    // Adjust the used memory counters
    if (new_ptr != ptr) {
        // Successfully reallocated to a new block
        tlsf_pool *old_pool = sdl_tlsf_get_pool((size_t)ptr);
        tlsf_pool *new_pool = sdl_tlsf_get_pool((size_t)new_ptr);

        // Update the old and new pool used memory
        if (old_pool) old_pool->used -= current_size;
        if (new_pool) new_pool->used += tlsf_block_size(new_ptr);

        // Check if the old pool is empty and consider removing it
        if (old_pool && old_pool->used <= 0 && active_instance.num_pools > 1) {
            sdl_tlsf_free_pool(old_pool);
        }
    } else {
        // Reallocated in place, adjust only the current pool's used memory
        tlsf_pool *pool = sdl_tlsf_get_pool((size_t)new_ptr);
        if (pool) {
            pool->used += tlsf_block_size(new_ptr) - current_size;
        }
    }

    SDL_UnlockMutex(tlsf_lock);
    return new_ptr;
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
tlsf_pool *sdl_tlsf_get_pool(size_t ptr_addr) {

	SDL_LockMutex(tlsf_lock);

	tlsf_pool_list poolList = active_instance.tlsf_pools;
	tlsf_pool *pool = poolList.header;

	while (pool != NULL) {
		if (ptr_addr >= (size_t)pool -> start && ptr_addr <= (size_t)pool -> end) {

			SDL_UnlockMutex(tlsf_lock);
			return pool;
		}
		pool = pool -> next;
	}

	SDL_UnlockMutex(tlsf_lock);

	return NULL;


}

void sdl_tlsf_add_pool() {

	SDL_LockMutex(tlsf_lock);

	size_t pool_size = active_instance.pool_size;
	size_t act_size = pool_size - tlsf_pool_overhead();

	void *ptr = mmap(NULL, pool_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (ptr == MAP_FAILED) {
		SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create memory for tlsf instance\n");
	}

	// Notify Valgrind about the allocation
    VALGRIND_MALLOCLIKE_BLOCK(ptr, pool_size, 0, 0);

	pool_t pool = tlsf_add_pool(active_instance.instance, ptr, act_size);
	if (pool == NULL) {
		SDL_Log("Failed to add pool to instance\n");
	}

	// Create a new pool
	tlsf_pool *new_pool = tlsf_calloc(active_instance.instance, sizeof(tlsf_pool), 1);
	new_pool -> mem = ptr;
	new_pool -> pool = pool;
	new_pool -> bytes = pool_size;
	new_pool -> used = sizeof(tlsf_pool);

	pool_id_counter += 1;
	new_pool -> pool_id = pool_id_counter;

	// Address Range
	new_pool -> start = ptr;
	new_pool -> end = (char *)new_pool -> start + act_size;

	// No Next but prev is tail
	new_pool -> next = NULL;
	new_pool -> prev = active_instance.tlsf_pools.tail;

	// Update the pool list
	tlsf_pool *current_tail = active_instance.tlsf_pools.tail;
	current_tail -> next = new_pool;
	active_instance.tlsf_pools.tail = new_pool;

	tlsf_instance *current_instance = sdl_tlsf_get_instance();

	// Increase the total size of the instance based on the pool size
	active_instance.num_pools += 1;
	active_instance.total_size += pool_size;

	SDL_Log("Adding Pool: %zu to Instance\n", new_pool -> pool_id);

	SDL_UnlockMutex(tlsf_lock);
}

void sdl_tlsf_free_pool(tlsf_pool *pool) {

	SDL_LockMutex(tlsf_lock);

	size_t id = pool -> pool_id;

	SDL_Log("Freeing Pool: %zu to Instance\n", id);

	// Removing the pool from the list
	tlsf_pool *prev = pool -> prev;
	tlsf_pool *next = pool -> next;

	// Update the prev and next's that are connected to the pool
	if (prev) prev->next = next;
	if (next) next->prev = prev;

	// Update the header and tail if necessary
	if (pool == active_instance.tlsf_pools.header) {
		active_instance.tlsf_pools.header = next;
	}

	if (pool == active_instance.tlsf_pools.tail) {
		active_instance.tlsf_pools.tail = prev;
	}

	// Update Active Instance
	active_instance.num_pools -= 1;
	active_instance.total_size -= active_instance.pool_size;

	sdl_tlsf_free_pool_mem(pool);

	SDL_Log("Freed Pool: %zu to Instance\n", id);


	SDL_UnlockMutex(tlsf_lock);
}

void sdl_tlsf_free_pool_mem(tlsf_pool *pool) {

	SDL_LockMutex(tlsf_lock);

	void *mem = pool -> mem;
	size_t size = pool -> bytes;
	pool_t pool_mem = pool -> pool;


	// Free pool data structure from the pool itself (kinda wacky)
	sdl_tlsf_get_pool((size_t) pool);

	tlsf_free(active_instance.instance, pool);

	// Free the pool
	tlsf_remove_pool(active_instance.instance, pool_mem);


	// Notify Valgrind that the pool is being freed
	VALGRIND_FREELIKE_BLOCK(mem, 0);

	// SysCall Free
	munmap(mem, size);

	SDL_UnlockMutex(tlsf_lock);
}