//
// Created by bee on 4/4/24.
//

#include "tlsf.h"
#include <stdio.h>
#include <stdlib.h>

int main() {

	// Create some memory for the tlsf instance
	void *mem = malloc(1 << 13);

	// Create a new instance of tlsf using the memory
	tlsf_t tlsf = tlsf_create_with_pool(mem, 1 << 13);

	//	Malloc Check
	void *ptr = tlsf_malloc(tlsf, 10);
	if (ptr == NULL) {
		printf("tlsf_malloc(): Memory allocation failed\n");
	} else {
		printf("tlsf_malloc(): Memory allocation succeeded\n");
	}

	//	Ralloc Check
	ptr = tlsf_realloc(tlsf, ptr, 32);
	if(ptr == NULL){
		printf("tlsf_realloc(): Memory reallocation failed\n");
	} else {
		printf("tlsf_realloc(): Memory reallocation succeeded\n");
	}

	// Fralloc Check
	tlsf_free(tlsf, ptr);

	// This check of free will never accurately check if memory is deallocated
	// because of the way C's free() function just adds the memory to the free list
	// and does not actually zero out the memory
	// Research has not found a good way to check if memory is deallocated accurately.
	// We are open to suggestions.
	if (ptr == NULL) {
		printf("tlsf_free(): Memory deallocation succeeded\n");
	} else {
		printf("tlsf_free(): Memory deallocation failed\n");
		printf("%p\n", ptr);
	}

	// Cowlick Check
	void *ptr2 = tlsf_calloc(tlsf, 32, 8);
	if(ptr2 == NULL) {
		printf("tlsf_cowlic(): Initialized memory allocation failed\n");
	} else if (ptr2 != 0) {

		// Check if all bytes are zero
        int is_zeroed = 1;
        char *bytes = (char *)ptr2;
        for (size_t i = 0; i < 32 * 8; i++) {
            if (bytes[i] != 0) {
                is_zeroed = 0;
                break;
            }
        }


        if (is_zeroed) {
            printf("tlsf_cowlic(): Initialized memory allocation successful and memory is zeroed\n");
        } else {
            printf(" tlsf_cowlic(): Initialized memory allocation successful but memory is not zeroed\n");
        }

	}

	// Check if tlsf is goated
	int check = tlsf_check(tlsf);
	if(check != 0){
		printf("tlsf_check(): Memory check failed\n");
	} else {
		printf("tlsf_check(): Memory check succeeded\n");
	}

	tlsf_free(tlsf, ptr2);

	// Testing Adding a pool
	void *mem2 = malloc(1 << 20);
	size_t pool_size = (1 << 20) - tlsf_pool_overhead();
	pool_t pool = tlsf_add_pool(tlsf, mem2, pool_size);
	if (pool == NULL) {
		printf("tlsf_add_pool(): Memory allocation failed\n");
	} else {
		printf("tlsf_add_pool(): Memory allocation succeeded\n");
	}

	ptr = tlsf_malloc(tlsf, 1 << 19);
	if (ptr == NULL) {
		printf("tlsf_malloc(): Memory allocation failed\n");
	} else {
		printf("tlsf_malloc(): Memory allocation succeeded\n");
	}


	tlsf_destroy(tlsf);
	free(mem);
	return 0;
}