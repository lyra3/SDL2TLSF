//
// Created by bee on 4/4/24.
//

#include "SDL/include/SDL3/SDL.h"
#include "SDL_TLSF/sdl_tlsf.h"
#include "MemTasks/mem_ops.h"

#include <time.h>    // For time()


int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	// Initialize primary memory management with a significant amount of memory.
    sdl_tlsf_init_with_size((1 << 20) * 2000);  // 128MB

    if (SDL_Init(0) < 0) {
        SDL_Log("SDL_Init failed (%s)", SDL_GetError());
        return 1;
    }

	MemoryTestConfig config;
    config.initial_alloc_size = 1024;  // 1 KB
    config.max_alloc_size = 1024 * 1024 * 20;  // 10 MB
    config.num_operations = 1000;  // Number of allocation/realloc/dealloc operations
    config.test_reallocation = 1;  // Enable reallocation testing
    config.test_deallocation = 1;  // Enable deallocation testing

	// Twenty-Three is number one
	size_t base_seed = 231;

	// Perform this test 5 times increase the seed by 1 each time
	for (int i = 0; i < 50; i++) {

//		tlsf_instance *profile = sdl_tlsf_create_instance((1 << 20) * 256);
//		sdl_tlsf_set_instance(profile);


		SDL_Log("Running test %d", i + 1);
		mem_speed_test(config, base_seed + i);
		memory_stress_test(base_seed + i);

//		sdl_tlsf_destroy_instance(profile);

	}


    SDL_Quit();
    sdl_tlsf_quit();

    return 0;
}