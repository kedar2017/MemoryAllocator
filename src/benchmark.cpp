#include "../include/workload/gemm.h"
#include <cstdio>
#include <chrono>
#include <array>

using clk = std::chrono::steady_clock;

void wall_time_compare_large_num_mult (size_t M_len, size_t tile_size, size_t align_len, std::vector<double>& malloc_ms, std::vector<double>& lin_alloc_ms,
                                       LinearAlloc* allocA, LinearAlloc* allocB,
                                       uint16_t* C_malloc_ptr, uint16_t* C_lin_alloc_aligned_ptr) {

    std::vector<uint8_t> A(M_len * M_len, 2);
    std::vector<uint8_t> B(M_len * M_len, 3);
    uint8_t* A_ptr = reinterpret_cast<uint8_t*>(A.data());
    uint8_t* B_ptr = reinterpret_cast<uint8_t*>(B.data());

    auto t2 = clk::now();

    for (int i = 0; i < 100000; i++) {
        gemm_malloc(A_ptr, B_ptr, C_malloc_ptr, M_len, tile_size);
    }

    auto t3 = clk::now();

    double ms_malloc = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "Duration in ms (malloc): " << ms_malloc << " for tile size " << tile_size <<" \n";
    malloc_ms.push_back(ms_malloc);

    auto t4 = clk::now();

    for (int i = 0; i < 100000; i++) {
        gemm_linear_alloc_aligned(A_ptr, B_ptr, C_lin_alloc_aligned_ptr, M_len, tile_size, align_len, allocA, allocB);
        allocA->reset();
        allocB->reset();
    }

    auto t5 = clk::now();
    double ms_lin_alloc_aligned = std::chrono::duration<double, std::milli>(t5 - t4).count();
    std::cout << "Duration in ms (linear alloc aligned): " << ms_lin_alloc_aligned << " for tile size " << tile_size  << " \n";
    lin_alloc_ms.push_back(ms_lin_alloc_aligned);
}

int main () {
    /*
    Fixed matrix size of 32x32 
    sweep across tile sizes in *tile_size_sweep*
    comparison between the wall times for 100K multiplies of 32x32 matrices
    */
    std::array<size_t, 4> tile_size_sweep = {2, 4, 8, 16};
    std::vector<double> gemm_malloc_duration_ms;
    std::vector<double> gemm_linear_alloc_duration_ms;
    LinearAlloc allocA;
    allocA.init(32 * 32, 64);
    LinearAlloc allocB;
    allocB.init(32 * 32, 64);

    std::vector<uint16_t> C_malloc(32 * 32);
    uint16_t* C_malloc_ptr = reinterpret_cast<uint16_t*>(C_malloc.data());
    std::vector<uint16_t> C_lin_alloc_aligned(32 * 32);
    uint16_t* C_lin_alloc_aligned_ptr = reinterpret_cast<uint16_t*>(C_lin_alloc_aligned.data());

    for (int i = 0; i < tile_size_sweep.size(); i++) {
        wall_time_compare_large_num_mult(32, tile_size_sweep[i], 64, gemm_malloc_duration_ms, gemm_linear_alloc_duration_ms, &allocA, &allocB, 
                                         C_malloc_ptr, C_lin_alloc_aligned_ptr);
    }
}