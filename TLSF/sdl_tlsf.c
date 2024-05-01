//
// Created by bee on 4/4/24.
//

#include "sdl_tlsf.h"

tlsf_instance active_instance;

void sdl_tlsf_init(void) {
	hello();

	active_instance = sdl_tlsf_create_instance();
	SDL_SetMemoryFunctions(sdl_tlsf_malloc, sdl_tlsf_calloc, sdl_tlsf_realloc, sdl_tlsf_free);

}


tlsf_instance sdl_tlsf_create_instance() {

	return NULL;
//	return tlsf_create(srbk(bytes), bytes);
}

void sdl_tlsf_destroy() {

	active_instance = NULL;
}

void *sdl_tlsf_malloc(size_t bytes) {
	SDL_Log("Hello World\n");
	return tlsf_malloc(active_instance, bytes);
}

void sdl_tlsf_free(void *ptr) {
	tlsf_free(active_instance, ptr);
}

void *sdl_tlsf_calloc(size_t nmemb, size_t size) {
	return tlsf_calloc(active_instance, size, nmemb);
}

void *sdl_tlsf_realloc(void *ptr, size_t size) {
	return tlsf_realloc(active_instance, ptr, size);
}