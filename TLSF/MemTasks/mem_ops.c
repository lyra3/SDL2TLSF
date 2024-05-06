//
// Created by bee on 5/5/24.
//

#include "mem_ops.h"



#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600




// Function to perform the speed test
void mem_speed_test(MemoryTestConfig config, int seed) {
    srand(seed);  // Seed random number generator

    // Dynamic array to hold memory pointers
    void **memory_blocks = (void **)SDL_malloc(config.num_operations * sizeof(void *));
    if (!memory_blocks) {
        SDL_Log("Failed to allocate memory for pointers array.");
        return;
    }

    SDL_Log("Starting memory operations...\n");

    // Allocation phase
    for (int i = 0; i < config.num_operations; i++) {
        size_t size = config.initial_alloc_size + (rand() % (config.max_alloc_size - config.initial_alloc_size + 1));
        memory_blocks[i] = SDL_malloc(size);
        if (!memory_blocks[i]) {
            SDL_Log("Memory allocation failed for size %zu at iteration %d", size, i);
            continue;
        }
        // Optionally fill the allocated memory
        SDL_memset(memory_blocks[i], rand() % 256, size);
    }

    // Reallocation phase
    if (config.test_reallocation) {
        for (int i = 0; i < config.num_operations; i++) {
            if (memory_blocks[i]) {
                size_t new_size = config.initial_alloc_size + (rand() % (config.max_alloc_size - config.initial_alloc_size + 1));
                void *new_block = SDL_realloc(memory_blocks[i], new_size);
                if (new_block) {
                    memory_blocks[i] = new_block;
                } else {
                    SDL_Log("Memory reallocation failed for new size %zu at iteration %d", new_size, i);
                    // Keep the old block if realloc failed
                }
            }
        }
    }

    // Deallocation phase
    if (config.test_deallocation) {
        for (int i = 0; i < config.num_operations; i++) {
            if (memory_blocks[i]) {
                SDL_free(memory_blocks[i]);
                memory_blocks[i] = NULL;
            }
        }
    }

    SDL_free(memory_blocks);
    SDL_Log("Memory operations completed.\n");
}

void memory_stress_test(int seed) {

    srand(seed);

    const int num_operations = 1000;
    const int max_alloc_size = (1 << 20) * 5;  // Up to 5MB per block
    void **pointers = SDL_malloc(num_operations * sizeof(void*));
    size_t *sizes = SDL_malloc(num_operations * sizeof(size_t));

    if (!pointers || !sizes) {
        SDL_Log("Failed to allocate test structures");
        if (pointers) SDL_free(pointers);
        if (sizes) SDL_free(sizes);
        SDL_Quit();
        return;
    }

    for (int i = 0; i < num_operations; i++) {
        sizes[i] = (rand() % max_alloc_size) + 1;  // Allocate between 1 byte and max_alloc_size
        pointers[i] = SDL_malloc(sizes[i]);
        if (!pointers[i]) {
            SDL_Log("Failed to allocate memory for size %zu", sizes[i]);
            continue;
        }
        memset(pointers[i], i % 256, sizes[i]);
    }

    for (int i = 0; i < num_operations; i++) {
        int index = rand() % num_operations;
        if (pointers[index]) {
            SDL_free(pointers[index]);
            pointers[index] = NULL;

            if (rand() % 2) {
                sizes[index] = (rand() % max_alloc_size) + 1;
                pointers[index] = SDL_malloc(sizes[index]);
                if (pointers[index]) {
                    memset(pointers[index], index % 256, sizes[index]);
                } else {
                    SDL_Log("Failed to reallocate memory block at index %d", index);
                }
            }
        }
    }

    // Final cleanup: Free any remaining allocated memory
    for (int i = 0; i < num_operations; i++) {
        if (pointers[i]) {
            SDL_free(pointers[i]);
        }
    }

    SDL_free(pointers);
    SDL_free(sizes);

}


#define NUM_THREADS 10
#define NUM_OPERATIONS 200
#define MAX_ALLOC_SIZE (1 << 20) * 10  // Up to 10MB per block

// Thread function for performing memory operations
int stress_test_thread(void *arg) {
    int thread_id = *(int *)arg;
    srand(time(NULL) + thread_id); // Ensure different seeds for different threads

    void **pointers = malloc(NUM_OPERATIONS * sizeof(void *));
    size_t *sizes = malloc(NUM_OPERATIONS * sizeof(size_t));

    if (!pointers || !sizes) {
        SDL_Log("Failed to allocate test structures for thread %d", thread_id);
        return -1;
    }

    // Perform random memory operations
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        sizes[i] = (rand() % MAX_ALLOC_SIZE) + 1;
        pointers[i] = SDL_malloc(sizes[i]);
        if (!pointers[i]) {
            SDL_Log("Thread %d failed to allocate memory for size %zu", thread_id, sizes[i]);
            continue;
        }
        memset(pointers[i], i % 256, sizes[i]);

        // Randomly decide to free and reallocate memory
        if (rand() % 3 == 0) {
            SDL_free(pointers[i]);
            sizes[i] = (rand() % MAX_ALLOC_SIZE) + 1;
            pointers[i] = SDL_malloc(sizes[i]);
            if (pointers[i]) {
                memset(pointers[i], (i % 256) + 1, sizes[i]);
            }
        }
    }

    // Free any remaining allocated memory
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        if (pointers[i]) {
            SDL_free(pointers[i]);
        }
    }

    free(pointers);
    free(sizes);
    return 0;
}

void memory_thread_test(int seed) {
    SDL_Thread *threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        threads[i] = SDL_CreateThread(stress_test_thread, "StressTestThread", (void *)&thread_ids[i]);
        if (threads[i] == NULL) {
            SDL_Log("Failed to create thread %d: %s", i, SDL_GetError());
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        if (threads[i] != NULL) {
            int thread_return_value;
            SDL_WaitThread(threads[i], &thread_return_value);
            SDL_Log("Thread %d exited with status %d", i, thread_return_value);
        }
    }
}


#define BLOCK_SIZE (1024 * 1024) * 50 // Each block is 50MB
#define MAX_BLOCKS 100000   // Support up to 1,000,000 MB
void window_test(const char *title) {

	SDL_Window* window = SDL_CreateWindow(title, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_Quit();
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    int running = 1;
    SDL_Event event;
    size_t memory_load = 0;
    void** memory_blocks = SDL_calloc(MAX_BLOCKS, sizeof(void*));
    if (!memory_blocks) {
        SDL_Log("Failed to allocate memory for tracking blocks");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }
    int num_blocks = 0;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = 0;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.keysym.sym == SDLK_UP && num_blocks < MAX_BLOCKS) {
                    memory_blocks[num_blocks] = SDL_malloc(BLOCK_SIZE);
                    if (memory_blocks[num_blocks]) {
                        SDL_memset(memory_blocks[num_blocks], 0, BLOCK_SIZE);
                        num_blocks++;
                        memory_load += BLOCK_SIZE;
                        SDL_Log("Allocated: %zu MB", memory_load / (1024 * 1024));
                    }
                } else if (event.key.keysym.sym == SDLK_DOWN && num_blocks > 0) {
                    num_blocks--;
                    SDL_free(memory_blocks[num_blocks]);
                    memory_load -= BLOCK_SIZE;
                    SDL_Log("Deallocated: %zu MB", memory_load / (1024 * 1024));
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        SDL_Delay(100); // Reduce CPU usage
    }

    // Clean up all allocated memory
    for (int i = 0; i < num_blocks; i++) {
        SDL_free(memory_blocks[i]);
    }
    SDL_free(memory_blocks);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}