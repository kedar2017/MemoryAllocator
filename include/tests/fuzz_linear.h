#pragma once
#include <random>
#include <cassert>
#include <iostream>
#include "../linear.h"

static bool ptr_in_any_chunk(LinearAlloc& A, void* p, size_t size) {
    for (auto raw : A.ptr_tracker_) {
        auto base = (void*)((size_t)raw); // raw, unaligned
        if (p >= raw && p < (void*)((size_t)raw + A.init_alloc_size_ + 256)) {
            return true;
        }
    }
    return false;
}

void test_alloc_rng_sizes (int iters) {
    LinearAlloc A;
    // give it a smallish base so we trigger capacity bumps
    A.init(4 * 1024, 64);

    std::mt19937 rng(12345);
    std::uniform_int_distribution<size_t> size_dist(1, 4096);
    std::uniform_int_distribution<int>    align_pick(0, 3);   // 0→8,1→16,2→32,3→64
    std::uniform_int_distribution<int>    op_pick(0, 9);      // 0→reset, others→alloc
    int num_allocations = 0;

    size_t align;
    switch (align_pick(rng)) {
        case 0: align = 8; break;
        case 1: align = 16; break;
        case 2: align = 32; break;
        default: align = 8; break;
    }

    for (int i = 0; i < iters; ++i) {
        int op = op_pick(rng);
        if (op == 0) {
            A.reset();
            // after reset we should have exactly 1 chunk
            assert(A.ptr_tracker_.size() == 1);
            continue;
        }

        size_t sz = size_dist(rng);
        void* p = A.allocate(sz, align);
        assert(((size_t)p % align) == 0);
        assert(ptr_in_any_chunk(A, p, sz));
        assert(A.total_bytes_allocated_ >= sz);
        num_allocations++;
    }

    assert(A.alloc_counter_ >= num_allocations);

    // final reset should clean extra chunks
    A.reset();
    assert(A.ptr_tracker_.size() == 1);
}

void test_alloc_max_capacity_bumps (int iters) {
    LinearAlloc A;
    A.init(4 * 1024, 64);

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int>    align_pick(0, 3);   // 0→8,1→16,2→32,3→64
    std::uniform_int_distribution<int>    op_pick(0, 9);      // 0→reset, others→alloc
    int num_allocations = 0;

    size_t align;
    switch (align_pick(rng)) {
        case 0: align = 8; break;
        case 1: align = 16; break;
        case 2: align = 32; break;
        default: align = 8; break;
    }

    for (int i = 0; i < iters; ++i) {
        int op = op_pick(rng);
        if (op == 0) {
            A.reset();
            // after reset we should have exactly 1 chunk
            assert(A.ptr_tracker_.size() == 1);
            continue;
        }

        // Always allocate more than the initialized - to trigger the capacity bump everytime
        size_t sz = 4 * 1024 + 1;
        void* p = A.allocate(sz, align);
        assert(((size_t)p % align) == 0);
        assert(ptr_in_any_chunk(A, p, sz));
        assert(A.total_bytes_allocated_ >= sz);
        num_allocations++;
    }

    assert(A.alloc_counter_ >= num_allocations);
    assert(A.capacity_bump_count_ >= num_allocations);

    // final reset should clean extra chunks
    A.reset();
    assert(A.ptr_tracker_.size() == 1);
    std::cout << "Test passed!! \n";
}
