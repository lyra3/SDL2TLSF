//
// Created by bee on 4/4/24.
//

#include "SDL/include/SDL3/SDL.h"
#include "sdl_tlsf.h"
#include <stdlib.h>  // For random()

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    sdl_tlsf_init();

    if (SDL_Init(0) < 0) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

    // Testing with varied size allocations
    const int num_allocations = 100;
    void *pointers[num_allocations];
    size_t sizes[num_allocations];

    for (int i = 0; i < num_allocations; i++) {
        sizes[i] = (rand() % (1024 * 1024)) + 1;  // Allocate 1 byte to 1 MB
        pointers[i] = SDL_malloc(sizes[i]);
        if (pointers[i] == NULL) {
            SDL_Log("Memory allocation failed for size %zu\n", sizes[i]);
        } else {
            memset(pointers[i], 0, sizes[i]);  // Use the memory
            SDL_Log("Allocated %zu bytes\n", sizes[i]);
        }
    }

    // Randomly free and reallocate to create fragmentation
    for (int i = 0; i < num_allocations; i++) {
        if (rand() % 2) {  // Randomly choose blocks to free
            SDL_free(pointers[i]);
            pointers[i] = NULL;
            SDL_Log("Freed memory block %d\n", i);
        }
    }

    for (int i = 0; i < num_allocations; i++) {
        if (pointers[i] == NULL) {
            sizes[i] = (rand() % (512 * 1024)) + 1;  // Smaller realloc sizes
            pointers[i] = SDL_malloc(sizes[i]);
            if (pointers[i] == NULL) {
                SDL_Log("Re-allocation failed for size %zu\n", sizes[i]);
            } else {
                memset(pointers[i], 0, sizes[i]);
                SDL_Log("Re-allocated %zu bytes\n", sizes[i]);
            }
        }
    }


	// Switch to a new instance
//	SDL_Log("Switching to a new instance\n");
//	tlsf_instance new_instance = sdl_tlsf_create_instance((1 << 20) * 14);  // 14MB
//	sdl_tlsf_set_instance(&new_instance);

    // Memory pressure test: Request large blocks until failure
    void *large_blocks[10];
    int block_index = 0;
    while (block_index < 10) {
        large_blocks[block_index] = SDL_malloc((1 << 20) * 10);  // 10 MB blocks
        if (large_blocks[block_index] == NULL) {
            SDL_Log("Failed to allocate 10 MB block\n");
            break;
        }
        block_index++;
        SDL_Log("Allocated 10 MB block %d\n", block_index);
    }

    // Free large blocks
    for (int i = 0; i < block_index; i++) {
        SDL_free(large_blocks[i]);
    }

	SDL_Log("Rebasing to the base instance\n");
//	sdl_tlsf_rebase_instance();
	//	 Free all remaining blocks from the prior instance
    for (int i = 0; i < num_allocations; i++) {
        if (pointers[i]) {
            SDL_free(pointers[i]);
        }
    }

//    // Test integrity check of the memory instance
//    if (sdl_tlsf_check_active_instance() == 0) {
//        SDL_Log("No errors detected in TLSF instance\n");
//    } else {
//        SDL_Log("Errors detected in TLSF instance\n");
//    }
//
//
//	sdl_tlsf_set_instance(&new_instance);
//	if (sdl_tlsf_check_active_instance()) {
//		SDL_Log("Errors detected in new TLSF instance\n");
//	} else {
//		SDL_Log("No errors detected in new TLSF instance\n");
//	}

//	sdl_tlsf_destroy_instance(&new_instance);
//	sdl_tlsf_rebase_instance();
	SDL_Quit();
	sdl_tlsf_quit();


    return 0;
}
