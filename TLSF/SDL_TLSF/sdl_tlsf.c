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

	// Calculate total size to include instance and pool metadata
    size_t total_required_size = pool_size + sizeof(tlsf_instance) + sizeof(tlsf_pool);
	total_required_size += tlsf_pool_overhead();  // Add the overhead of the pool

    // Create some memory for the tlsf instance using mmap
    void *mem = mmap(NULL, total_required_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create memory for tlsf instance\n");
        return NULL;  // Early return on failure
    }

    // Notify Valgrind about the allocation
    VALGRIND_MALLOCLIKE_BLOCK(mem, total_required_size, 0, 0);

	// Initialize the tlsf_instance at the start of the mapped memory
    tlsf_instance *new_instance = (tlsf_instance *)mem;

    // Initialize the tlsf_pool directly after tlsf_instance in memory
    tlsf_pool *pool = (tlsf_pool *)((char *)mem + sizeof(tlsf_instance));
    memset(pool, 0, sizeof(tlsf_pool));  // Zero out the pool structure

    // Set up the memory pool directly after the tlsf_pool in memory
    void *pool_mem = (char *)pool + sizeof(tlsf_pool);

	// Now that we have the memory divied up we can initialize variables
    new_instance -> instance = tlsf_create_with_pool(pool_mem, pool_size);
    new_instance -> num_pools = 1;
    new_instance -> pool_size = pool_size;

    // Configure the pool object
    pool -> mem = pool_mem;
    pool -> pool = tlsf_get_pool(new_instance->instance); // Gets the pool from the instance
    pool -> bytes = pool_size;
    pool -> used = 0;

    // Address Range
    pool -> start = pool_mem;
    pool -> end = (char *)pool->start + pool_size;

    // Setup linked list pointers
    pool -> next = NULL;
    pool -> prev = NULL;

    // Link the pool into the new_instance
    new_instance -> tlsf_pools.header = pool;
    new_instance -> tlsf_pools.tail = pool;

    // Initialize the remaining fields
    new_instance -> total_size = pool_size;
    new_instance -> total_used = 0;


//    SDL_Log("Break here and view whats up!");

    return new_instance;
}

tlsf_instance *sdl_tlsf_get_instance() {
	return active_instance;
}

void sdl_tlsf_set_instance(tlsf_instance *instance) {
	SDL_LockMutex(tlsf_lock);
	active_instance = instance;
	SDL_UnlockMutex(tlsf_lock);
}

tlsf_instance *sdl_tlsf_rebase_instance() {

	SDL_LockMutex(tlsf_lock);

	tlsf_instance *current_instance = sdl_tlsf_get_instance();
	active_instance = base_instance;

	SDL_UnlockMutex(tlsf_lock);
	return current_instance;
}

void sdl_tlsf_destroy_instance( tlsf_instance *instance) {

	if (!instance) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Attempt to destroy a NULL instance\n");
        return;
    }

    SDL_LockMutex(tlsf_lock);  // Ensure thread safety

    // Free all pools except the head, which is contiguous with the tlsf_instance
    tlsf_pool *current_pool = instance->tlsf_pools.tail;
    while (current_pool != NULL && current_pool != instance->tlsf_pools.header) {
        tlsf_pool *prev_pool = current_pool->prev;
        sdl_tlsf_free_pool(current_pool);
        current_pool = prev_pool;
    }

    // Calculate initial mmap size
    size_t initial_alloc = sizeof(tlsf_instance) + sizeof(tlsf_pool) + instance->pool_size;
	initial_alloc += tlsf_pool_overhead();  // Add the overhead of the pool

    // Notify Valgrind that the memory is being freed
    VALGRIND_FREELIKE_BLOCK(instance, 0);

    // Free the entire memory block allocated via mmap
    munmap(instance, initial_alloc);



	// SDL_DestroyMutex(tlsf_lock);  // Destroy the mutex

    // Nullify global pointers if they pointed to this instance
    if (active_instance == instance) {
        active_instance = NULL;
    }
    if (base_instance == instance) {
        base_instance = NULL;

	// Don't try unlocking the mutex if we just freed all memory lol
    } else {
		SDL_UnlockMutex(tlsf_lock);  // Release the mutex
	}

}

void sdl_tlsf_print_instance(tlsf_instance *instance) {

	SDL_LockMutex(tlsf_lock);

	tlsf_pool *pool = instance -> tlsf_pools.header;

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
	if (bytes >= (active_instance -> pool_size) - tlsf_pool_overhead()) {
		SDL_Log("Requested memory size is greater than pool size\n");

		SDL_UnlockMutex(tlsf_lock);
		return NULL;
	}


	// Check if we have enough memory to allocate
	if (active_instance -> total_size - active_instance -> total_used < bytes) {

		// Add another pool to the instance
		sdl_tlsf_add_pool();
	}

	void *ptr = tlsf_malloc(active_instance -> instance, bytes);;

	if (!ptr) {

		// Test if the problem is having a contiguous block of memory
		sdl_tlsf_add_pool();
		ptr = tlsf_malloc(active_instance -> instance, bytes);

		if (!ptr) {
			SDL_Log("Failed to allocate memory\n");
			SDL_UnlockMutex(tlsf_lock);
			return NULL;
		}
	}

	size_t block_size = tlsf_block_size(ptr);

	active_instance -> total_used += block_size;

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
	tlsf_free(active_instance -> instance, ptr);

	// Update the pool list
	pool -> used -= block_size;


	// Update the total used memory
	active_instance -> total_used -= block_size;

	// Check how much memory is left in the pool
	if (pool -> used == 0 && active_instance -> num_pools > 1) {
//
////		SDL_Log("Freeing Tail");
//
//		// Remove the pool
		sdl_tlsf_free_pool(pool);
	}


	SDL_UnlockMutex(tlsf_lock);
}

void *sdl_tlsf_calloc(size_t nmemb, size_t size) {

	SDL_LockMutex(tlsf_lock);

	size_t bytes = nmemb * size;

	// Check if we have enough memory to allocate
	if (active_instance -> total_size - active_instance -> total_used < bytes) {

		// Add another pool to the instance
		sdl_tlsf_add_pool();
	}

	void *ptr = tlsf_calloc(active_instance -> instance, size, nmemb);

	if (!ptr) {

		// Test if the problem is having a contiguous block of memory
		sdl_tlsf_add_pool();
		ptr = tlsf_calloc(active_instance -> instance, size, nmemb);

		if (!ptr) {
			SDL_Log("Failed to allocate memory\n");
			SDL_UnlockMutex(tlsf_lock);
			return NULL;
		}
	}

	size_t block_size = tlsf_block_size(ptr);

	active_instance -> total_used += block_size;

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
        if (size >= (active_instance -> pool_size - tlsf_pool_overhead())) {
            SDL_Log("Requested realloc size is greater than pool size\n");
            SDL_UnlockMutex(tlsf_lock);
            return NULL;
        }

        // Check if we need more total memory than available
        if (active_instance -> total_size - active_instance -> total_used < size - current_size) {
            sdl_tlsf_add_pool();  // Add another pool to the instance
        }
    }

    // Attempt to reallocate memory
    void *new_ptr = tlsf_realloc(active_instance -> instance, ptr, size);
    if (!new_ptr && size > current_size) {
        // If realloc fails and it's a size increase, try adding a pool and reallocating
        sdl_tlsf_add_pool();
        new_ptr = tlsf_realloc(active_instance -> instance, ptr, size);
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
        if (old_pool) old_pool -> used -= current_size;
        if (new_pool) new_pool -> used += tlsf_block_size(new_ptr);

        // Check if the old pool is empty and consider removing it
        if (old_pool && old_pool -> used <= 0 && active_instance -> num_pools > 1) {
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

	int ret_val = tlsf_check(active_instance  -> instance);

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

	tlsf_pool_list poolList = active_instance  -> tlsf_pools;
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

    size_t pool_size = active_instance->pool_size;
    size_t alloc_size = pool_size + sizeof(tlsf_pool) + tlsf_pool_overhead();

    void *mem = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Failed to create memory for new pool\n");
        SDL_UnlockMutex(tlsf_lock);
        return;
    }

    VALGRIND_MALLOCLIKE_BLOCK(mem, alloc_size, 0, 0);

    tlsf_pool *new_pool = (tlsf_pool *)mem;
    void *pool_mem = (char *)mem + sizeof(tlsf_pool);

    pool_t pool = tlsf_add_pool(active_instance->instance, pool_mem, pool_size);
    if (pool == NULL) {
        SDL_Log("Failed to add pool to instance\n");
        munmap(mem, alloc_size);
        SDL_UnlockMutex(tlsf_lock);
        return;
    }

	// Setup the metadata for the new pool
    new_pool->mem = pool_mem;
    new_pool->pool = pool;
    new_pool->bytes = pool_size;
    new_pool->used = 0;

	pool_id_counter++;
    new_pool->pool_id = pool_id_counter;

	// Address Range
    new_pool->start = pool_mem;
    new_pool->end = (char *)pool_mem + pool_size;

    new_pool->next = NULL;  // This new pool is the new tail, so no next.
    new_pool->prev = active_instance->tlsf_pools.tail;  // Link to the previous tail.

    if (active_instance->tlsf_pools.tail) {
        active_instance->tlsf_pools.tail->next = new_pool;  // Link the old tail to the new tail.
    }

    active_instance->tlsf_pools.tail = new_pool;  // Update the tail to the new pool.

    if (!active_instance->tlsf_pools.header) {
        active_instance->tlsf_pools.header = new_pool;  // If there's no head, this is also the head (shouldn't happen unless the instance is reset somehow).
    }

    active_instance->num_pools++;
    active_instance->total_size += alloc_size;

//    SDL_Log("Added new pool: %zu to instance", new_pool->pool_id);

    SDL_UnlockMutex(tlsf_lock);
}

void sdl_tlsf_free_pool(tlsf_pool *pool) {

	SDL_LockMutex(tlsf_lock);

	size_t id = pool -> pool_id;

//	SDL_Log("Freeing Pool: %zu to Instance\n", id);

	// Removing the pool from the list
	tlsf_pool *prev = pool -> prev;
	tlsf_pool *next = pool -> next;

	// Update the prev and next's that are connected to the pool
	if (prev) prev->next = next;
	if (next) next->prev = prev;

	// Update the header and tail if necessary
	if (pool == active_instance -> tlsf_pools.header) {
		active_instance -> tlsf_pools.header = next;
	}

	if (pool == active_instance -> tlsf_pools.tail) {
		active_instance -> tlsf_pools.tail = prev;
	}

	// Update Active Instance
	active_instance -> num_pools -= 1;
	active_instance -> total_size -= active_instance -> pool_size;

	sdl_tlsf_free_pool_mem(pool);

//	SDL_Log("Freed Pool: %zu to Instance\n", id);


	SDL_UnlockMutex(tlsf_lock);
}

void sdl_tlsf_free_pool_mem(tlsf_pool *pool) {

	SDL_LockMutex(tlsf_lock);

	size_t pool_size = active_instance->pool_size;
    size_t alloc_size = pool_size + sizeof(tlsf_pool) + tlsf_pool_overhead();

	// Free the pool
	tlsf_remove_pool(active_instance -> instance, pool -> pool);

	// Notify Valgrind that the pool is being freed
	VALGRIND_FREELIKE_BLOCK(pool, 0);

	// Free the entire block of memory containing the pool
	munmap(pool, alloc_size);

	SDL_UnlockMutex(tlsf_lock);
}