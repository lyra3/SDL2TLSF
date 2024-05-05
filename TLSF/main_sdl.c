//
// Created by bee on 4/4/24.
//

#include "SDL/include/SDL3/SDL.h"
#include "sdl_tlsf.h"
#include <stdlib.h>  // For random()
#include <time.h>    // For time()

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    sdl_tlsf_init_with_size((1 << 20) * 16);  // 8MB

	if (SDL_Init(0) < 0) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

    srand(time(NULL));  // Initialize random seed

    // Testing with varied size allocations
    const int num_allocations = 100;
    void *pointers[num_allocations];
    size_t sizes[num_allocations];

    // Allocate and write data
    for (int i = 0; i < num_allocations; i++) {
        sizes[i] = (rand() % (1024 * 1024)) + 1;  // Allocate 1 byte to 1 MB
        pointers[i] = SDL_malloc(sizes[i]);
        if (pointers[i] == NULL) {
            SDL_Log("Memory allocation failed for size %zu\n", sizes[i]);
        } else {
            // Fill with a known pattern
            memset(pointers[i], i % 256, sizes[i]);
            SDL_Log("Allocated and wrote to %zu bytes\n", sizes[i]);
        }
    }

    // Check written data and free randomly
    for (int i = 0; i < num_allocations; i++) {
        if (pointers[i]) {
            unsigned char *data = (unsigned char *)pointers[i];
            for (size_t j = 0; j < sizes[i]; j++) {
                if (data[j] != (unsigned char)(i % 256)) {
                    SDL_Log("Data mismatch detected at block %d, index %zu\n", i, j);
                    break;
                }
            }
            if (rand() % 2) {
                SDL_free(pointers[i]);
                pointers[i] = NULL;
                SDL_Log("Freed memory block %d\n", i);
            }
        }
    }

    // Reallocate memory for nullified pointers and write data again
    for (int i = 0; i < num_allocations; i++) {
        if (pointers[i] == NULL) {
            sizes[i] = (rand() % (512 * 1024)) + 1;  // Smaller realloc sizes
            pointers[i] = SDL_malloc(sizes[i]);
            if (pointers[i] == NULL) {
                SDL_Log("Re-allocation failed for size %zu\n", sizes[i]);
            } else {
                memset(pointers[i], (i % 256) + 1, sizes[i]);
                SDL_Log("Re-allocated and wrote to %zu bytes\n", sizes[i]);
            }
        }
    }

    // Memory pressure test: Request large blocks until failure
	// Create Specific Instance
	SDL_Log("Creating a specific instance\n");
	tlsf_instance *big_pool = sdl_tlsf_create_instance((1 << 20) * 75);  // 50MB
	sdl_tlsf_set_instance(&big_pool);

	// Initializing 10 blocks of 10MB
	// Should need two pools
    void *large_blocks[10];
    int block_index = 0;
    while (block_index < 10) {
        large_blocks[block_index] = SDL_malloc((1 << 20) * 10);  // 10 MB blocks
        if (large_blocks[block_index] == NULL) {
            SDL_Log("Failed to allocate 10 MB block\n");
            break;
        }
        memset(large_blocks[block_index], block_index, (1 << 20) * 10);  // Write to the allocated memory
        block_index++;
        SDL_Log("Allocated and wrote to 10 MB block %d\n", block_index);
    }

    // Free large blocks
    for (int i = 0; i < block_index; i++) {
        SDL_free(large_blocks[i]);
    }
//
	sdl_tlsf_destroy_instance(big_pool);
	sdl_tlsf_rebase_instance();

    // Free all remaining blocks
    for (int i = 0; i < num_allocations; i++) {
        if (pointers[i]) {
            SDL_free(pointers[i]);
        }
    }



    SDL_Quit();
	sdl_tlsf_quit();


    return 0;
}
