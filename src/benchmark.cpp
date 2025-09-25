#include "../include/workload/gemm.h"
#include <cstdio>
#include <chrono>

using clk = std::chrono::steady_clock;

void wall_time_compare_gemm () {
    auto t0 = clk::now();
    std::vector<uint8_t> A(550 * 550, 2);
    std::vector<uint8_t> B(550 * 550, 3);
    std::vector<uint16_t> C_lin_alloc(550 * 550);
    uint8_t* A_ptr = reinterpret_cast<uint8_t*>(A.data());
    uint8_t* B_ptr = reinterpret_cast<uint8_t*>(B.data());
    uint16_t* C_lin_alloc_ptr = reinterpret_cast<uint16_t*>(C_lin_alloc.data());
    gemm_linear_alloc(A_ptr, B_ptr, C_lin_alloc_ptr, 550, 4);
    auto t1 = clk::now();

    double ms_lin_alloc = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "Duration in ms (linear alloc): " << ms_lin_alloc << " \n";

    auto t2 = clk::now();
    std::vector<uint16_t> C_malloc(550 * 550);
    uint16_t* C_malloc_ptr = reinterpret_cast<uint16_t*>(C_malloc.data());
    gemm_malloc(A_ptr, B_ptr, C_malloc_ptr, 550, 4);
    auto t3 = clk::now();

    double ms_malloc = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "Duration in ms (malloc): " << ms_malloc << " \n";

    auto t4 = clk::now();

    std::vector<uint16_t> C_lin_alloc_aligned(555 * 555);
    uint16_t* C_lin_alloc_aligned_ptr = reinterpret_cast<uint16_t*>(C_lin_alloc_aligned.data());
    gemm_linear_alloc_aligned(A_ptr, B_ptr, C_lin_alloc_aligned_ptr, 555, 8, 128);

    auto t5 = clk::now();
    double ms_lin_alloc_aligned = std::chrono::duration<double, std::milli>(t5 - t4).count();
    std::cout << "Duration in ms (linear alloc aligned): " << ms_lin_alloc_aligned << " \n";
}

int main () {
    wall_time_compare_gemm();

}