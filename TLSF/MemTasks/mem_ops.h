//
// Created by bee on 5/5/24.
//

#ifndef TLSF_MEM_OPS_H
#define TLSF_MEM_OPS_H

#include "../SDL/include/SDL3/SDL.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Structure to configure the memory test
typedef struct {
    size_t initial_alloc_size;  // Initial size of allocations
    size_t max_alloc_size;      // Maximum size of allocations
    int num_operations;         // Number of operations to perform
    int test_reallocation;      // Flag to test reallocation
    int test_deallocation;      // Flag to test deallocation
} MemoryTestConfig;

void mem_speed_test(MemoryTestConfig config, int seed);
void memory_stress_test(int seed);
void mem_thread_test(int seed);
void window_test(const char *title);
void tlsf_best_case_test(int seed);

#endif //TLSF_MEM_OPS_H
