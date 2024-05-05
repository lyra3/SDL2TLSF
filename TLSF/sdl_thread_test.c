//
// Created by bee on 5/5/24.
//

#include "SDL/include/SDL3/SDL.h"
#include "sdl_tlsf.h"
#include <stdlib.h>  // For random()
#include <string.h>  // For memset()

#define NUM_THREADS 4
#define OPERATIONS_PER_THREAD 1000
#define MAX_BLOCK_SIZE (1024 * 1024)  // Maximum block size of 1MB

typedef struct {
    int thread_id;
    tlsf_instance local_instance;
} thread_data;

void *thread_memory_operations(void *ptr) {
    thread_data *data = (thread_data *)ptr;
    int thread_id = data->thread_id;
    tlsf_instance *instance = &data->local_instance;

    // Each thread creates its own instance with an initial pool of 4MB
    *instance = sdl_tlsf_create_instance((1 << 20) * 4);  // 4MB

    // Perform random memory operations
    for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        size_t size = (rand() % MAX_BLOCK_SIZE) + 1;
		sdl_tlsf_set_instance(instance);
        void *memory = SDL_malloc(size);
        if (memory) {
            memset(memory, 0, size);  // Use the memory
            if (rand() % 2) {
                size_t new_size = (rand() % MAX_BLOCK_SIZE) + 1;
				sdl_tlsf_set_instance(instance);
                void *new_memory = sdl_tlsf_realloc(memory, new_size);
                if (new_memory) {
                    memory = new_memory;
                }
            }
			sdl_tlsf_set_instance(instance);
            sdl_tlsf_free( memory);
        }
    }

    // Check and log the integrity of the instance
    if (sdl_tlsf_check_instance(instance) == 0) {
        SDL_Log("Thread %d: No errors detected in TLSF instance\n", thread_id);
    } else {
        SDL_Log("Thread %d: Errors detected in TLSF instance\n", thread_id);
    }

    // Clean up the instance
    sdl_tlsf_destroy_instance(instance);

    free(ptr);
    return NULL;
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    srand(time(NULL));

    SDL_Thread *threads[NUM_THREADS];

    // Launch threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data *data = malloc(sizeof(thread_data));
        data->thread_id = i;
        threads[i] = SDL_CreateThread(thread_memory_operations, "MemoryThread", (void *)data);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        SDL_WaitThread(threads[i], NULL);
    }

    SDL_Quit();
    return 0;
}
