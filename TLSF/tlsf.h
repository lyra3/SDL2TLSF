#ifndef TLFS_TLSF_H
#define TLFS_TLSF_H

#include <stddef.h>

typedef void* tlsf_t;
typedef void* pool_t;

void hello(void);

/*memory pool things*/
//tlsf_t tlsf_create(void* mem);
//tlsf_t tlsf_create_with_pool(void* mem, size_t bytes);
//void tlsf_destroy(tlsf_t tlsf);
//pool_t tlsf_get_pool(tlsf_t tlsf);
//pool_t tlsf_add_pool(tlsf_t tlsf, void* mem, size_t bytes);
//void tlsf_remove_pool(tlsf_t tlsf, pool_t pool);

/*malloc/memalign/calloc/realloc/free*/
void *tlsf_malloc(void *tlsf, size_t bytes);
//void *tlsf_memalign(tlsf_t tlsf, size_t align, size_t bytes);
void *tlsf_calloc(void *tlsf, size_t elem_size, size_t num_elems);
void *tlsf_realloc(tlsf_t tlsf, void *ptr, size_t size);
void tlsf_free(void *tlsf, void *ptr);

/*Overheads/limits of internal structures*/
//size_t tlsf_block_size(void* ptr);
//size_t tlsf_size(void);
//size_t tlsf_align_size(void);
//size_t tlsf_block_size_min(void);
//size_t tlsf_block_size_max(void);
//size_t tlsf_pool_overhead(void);
//size_t tlsf_alloc_overhead(void);

#endif //TLFS_TLSF_H
