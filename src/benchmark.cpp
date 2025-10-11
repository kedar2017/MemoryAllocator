#include "../include/workload/gemm.h"
#include "../include/workload/thread_alloc.h"
#include "../include/workload/graph_build.h"
#include <cstdio>
#include <chrono>
#include <array>
#include <thread>
#include <cmath>

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

void wall_time_compare_multi_thread_alloc (size_t allocations, size_t threads, size_t num_bytes, std::vector<double> thread_alloc_malloc_duration_ms, 
                                           std::vector<double> thread_alloc_linear_alloc_duration_ms) {
    auto t0 = clk::now();
    std::vector<std::thread> workers_malloc;
    for (int i = 0; i < threads; i++) {
        workers_malloc.emplace_back(thread_malloc, allocations, num_bytes);
    }
    for (auto& x: workers_malloc) x.join();
    auto t1 = clk::now();

    double ms_malloc = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "Multi-thread --- Duration in ms (malloc): " << ms_malloc << " \n";
    thread_alloc_malloc_duration_ms.push_back(ms_malloc);

    auto t2 = clk::now();
    std::vector<std::thread> workers_lin_alloc;
    for (int i = 0; i < threads; i++) {
        workers_lin_alloc.emplace_back(thread_arena, allocations, num_bytes);
    }
    for (auto& x: workers_lin_alloc) x.join();
    auto t3 = clk::now();

    double lin_alloc_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "Multi-thread --- Duration in ms (linear alloc aligned): " << lin_alloc_ms << " \n";
    thread_alloc_linear_alloc_duration_ms.push_back(lin_alloc_ms);

}

void wall_time_compare_graph_build (int num_children, int num_layers, int align_len) {
    LinearAlloc graph_arena;
    
    auto t0 = clk::now();
    graph_arena.init(((std::pow(num_children, num_layers) - 1) / (num_children - 1)) * alignment_helper(2 + 8 * (num_children), align_len), align_len);
    build_graph_lin_alloc(num_children, num_layers, align_len, &graph_arena);
    auto t1 = clk::now();

    std::cout << "total_bytes_allocated_ " << graph_arena.total_bytes_allocated_ << "\n";
    std::cout << "alloc_counter_ " << graph_arena.alloc_counter_ << "\n";
    std::cout << "peak_bytes_ " << graph_arena.peak_bytes_ << "\n";
    std::cout << "capacity_bump_count_ " << graph_arena.capacity_bump_count_ << "\n";
    double lin_alloc_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "Graph building --- Duration in ms (linear alloc): " << lin_alloc_ms << " \n";

    auto t2 = clk::now();    
    build_graph_malloc(num_children, num_layers);
    auto t3 = clk::now();

    double malloc_ms = std::chrono::duration<double, std::milli>(t3 - t2).count();
    std::cout << "Graph building --- Duration in ms (malloc): " << malloc_ms << " \n";
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

    size_t allocations = 100000;
    size_t threads = 50;
    size_t num_bytes = 64;
    std::vector<double> thread_alloc_malloc_duration_ms;
    std::vector<double> thread_alloc_linear_alloc_duration_ms;

    wall_time_compare_multi_thread_alloc(allocations, threads, num_bytes, thread_alloc_malloc_duration_ms, thread_alloc_linear_alloc_duration_ms);

    wall_time_compare_graph_build(8, 9, 32);
}