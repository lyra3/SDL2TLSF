//
// Created by bee on 4/4/24.
//

#include "sdl_tlsf.h"

void sdl_tlsf_init(void) {
	hello();
	SDL_SetMemoryFunctions(sdl_tlsf_malloc, sdl_tlsf_calloc, sdl_tlsf_realloc, sdl_tlsf_free);
}

void *sdl_tlsf_malloc(size_t bytes) {
	return tlsf_malloc(NULL, bytes);
}

void sdl_tlsf_free(void *ptr) {
	tlsf_free(NULL, ptr);
}

void *sdl_tlsf_calloc(size_t nmemb, size_t size) {
	return tlsf_calloc(NULL, size, nmemb);
}

void *sdl_tlsf_realloc(void *ptr, size_t size) {
	return tlsf_realloc(NULL, ptr, size);
}