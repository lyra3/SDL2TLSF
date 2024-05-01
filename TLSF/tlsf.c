#include "tlsf.h"

#include <stdlib.h>
#include <stdio.h>


void hello(void) {
	printf("Hello, World!\n");
}

void *tlsf_malloc(void *tlsf, size_t bytes) {

	// In order to test the setup of the library, and ensure the library is properly setup
	return malloc(bytes);
}

void *tlsf_calloc(tlsf_t tlsf, size_t elem_size, size_t num_elems){

	unsigned char *ptr = tlsf_malloc(tlsf, elem_size * num_elems);
	for (int i = 0; i < num_elems * elem_size; i++) {
		ptr[i] = 0;
	}
	return ptr;

//	return calloc(num_elems, elem_size);
}

void *tlsf_realloc(tlsf_t tlsf, void *ptr, size_t size){
	return realloc(ptr, size);
}

void tlsf_free(void *tlsf, void *ptr) {
	return free(ptr);
}