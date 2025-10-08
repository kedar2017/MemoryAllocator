#pragma once
#include "../linear.h"

void* thread_arena (size_t allocations, size_t num_bytes) {
    LinearAlloc arena;
    arena.init(num_bytes * allocations);
    
    for (int r = 0; r < 100; ++r) {
        for (size_t i = 0; i < allocations; ++i)
            arena.allocate(num_bytes);
        arena.reset();
    }
    return NULL;
}

void* thread_malloc (size_t allocations, size_t num_bytes) {
    for (int r = 0; r < 100; ++r) {
        for (int i = 0; i < allocations; ++i) {
            void *p = malloc(num_bytes);
            free(p);
        }
    }
    return NULL;
}
