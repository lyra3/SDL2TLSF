//
// Created by bee on 4/4/24.
//

#include "SDL/include/SDL3/SDL.h"
#include "sdl_tlsf.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;



	// Initialize our "custom" memory allocator
	sdl_tlsf_init();

	SDL_Log("Mutex init\n");
	sdl_tlsf_init_mutex();

	SDL_Log("SDL_Init\n");
    if (SDL_Init(0) < 0) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

	void *ptr = SDL_malloc(10);
	if (ptr == NULL) {
		SDL_Log("sdl_tlsf_malloc(): Memory allocation failed\n");
	} else {
		SDL_Log("sdl_tlsf_malloc(): Memory allocation succeeded\n");
	}


	ptr = SDL_realloc(ptr, 32);
	if (ptr == NULL) {
		SDL_Log("sdl_tlsf_realloc(): Memory reallocation failed\n");
	} else {
		SDL_Log("sdl_tlsf_realloc(): Memory reallocation succeeded\n");
	}

	SDL_free(ptr);

	// This check of free will never accurately check if memory is deallocated
	// because of the way C's free() function just adds the memory to the free list
	// and does not actually zero out the memory
	// Research has not found a good way to check if memory is deallocated accurately.
	// We are open to suggestions.
	if (ptr == NULL) {
		SDL_Log("sdl_tlsf_free(): Memory deallocation succeeded\n");
	} else {
		SDL_Log("sdl_tlsf_free(): Memory deallocation failed\n");
	}

	ptr = SDL_calloc(32, 8);
	if (ptr == NULL) {
		SDL_Log("sdl_tlsf_calloc(): Memory allocation failed\n");
	} else {
		SDL_Log("sdl_tlsf_calloc(): Memory allocation succeeded\n");
	}

	SDL_free(ptr);

	// Testing dynamicly adding a pool, by allocating more memory than the current pool size
	ptr = SDL_malloc((1 << 20));
	if (ptr == NULL) {
		SDL_Log("sdl_tlsf_malloc(): Memory allocation failed\n");
	} else {
		SDL_Log("sdl_tlsf_malloc(): Memory allocation succeeded\n");
	}



//	//	Double Check statuc
//	if (sdl_tlsf_check_active_instance() == 0) {
//		SDL_Log("sdl_tlsf_check_active_instance(): No errors\n");
//	} else {
//		SDL_Log("sdl_tlsf_check_active_instance(): Errors found\n");
//	}
//
//	if (active_instance.tlsf_pools.header != NULL && tlsf_get_pool(active_instance.instance) == active_instance.tlsf_pools.header -> pool) {
//		SDL_Log("active_instance.pool_list.header: %p\n", active_instance.tlsf_pools.header);
//
//	}
//	else {
//		SDL_Log("active_instance.pool_list.header: %p\n", active_instance.tlsf_pools.header -> pool);
//		SDL_Log("tlsf_get_pool(active_instance.instance): %p\n", tlsf_get_pool(active_instance.instance));
//	}
//
//	// Double Check statuc
//	if (sdl_tlsf_check_active_instance() == 0) {
//		SDL_Log("sdl_tlsf_check_active_instance(): No errors\n");
//	} else {
//		SDL_Log("sdl_tlsf_check_active_instance(): Errors found\n");
//	}

	SDL_Quit();
    sdl_tlsf_destroy_instance();

	return 0;

}