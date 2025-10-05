#include "../linear.h"
#include <vector>

void prepA (uint8_t* A, uint8_t* scratchA, size_t M, size_t tile_size, size_t offset) {
    for (size_t j = 0; j < tile_size; j++) {
        for (size_t i = 0; i < M; i++) {
            scratchA[i + (M * j)] = A[i * M + (j + offset)];
        }
    }
}

void prepB (uint8_t* B, uint8_t* scratchB, size_t M, size_t tile_size, size_t offset) {
    for (size_t i = 0; i < tile_size; i++) {
        for (size_t j = 0; j < M; j++) {
            scratchB[i * M + j] = B[(i + offset) * M + j];
        }
    }
}

void gemm_linear_alloc_aligned (uint8_t* A, uint8_t* B, uint16_t* C, size_t M, size_t tile_size, size_t align_len, LinearAlloc* allocA, LinearAlloc* allocB) {
    for (int t = 0; t < M; t = t + tile_size) {
        size_t kB = std::min(tile_size, M - t);
        uint8_t* scratchA = (uint8_t*) allocA->allocate(M * kB, align_len);
        uint8_t* scratchB = (uint8_t*) allocB->allocate(M * kB, align_len);

        prepA(A, scratchA, M, kB, t);
        prepB(B, scratchB, M, kB, t);

        for (int p = 0; p < kB; p++) {
            for (int i = 0; i < M; i++) {
                uint8_t tempA = scratchA[i + M * p];
                for (int j = 0; j < M; j++) {
                    C[i * M + j] += tempA * scratchB[j + M * p];
                }
            }
        }
    }
}

void gemm_malloc (uint8_t* A, uint8_t* B, uint16_t* C, size_t M, size_t tile_size) {
    for (int t = 0; t < M; t = t + tile_size) {
        size_t kB = std::min(tile_size, M - t);
        uint8_t* scratchA = (uint8_t*)malloc(M * kB);
        uint8_t* scratchB = (uint8_t*)malloc(M * kB);
        prepA(A, scratchA, M, kB, t);
        prepB(B, scratchB, M, kB, t);

        for (int p = 0; p < kB; p++) {
            for (int i = 0; i < M; i++) {
                uint8_t tempA = scratchA[i + M * p];
                for (int j = 0; j < M; j++) {
                    C[i * M + j] += tempA * scratchB[j + M * p];
                }
            }
        }
        free(scratchA);
        free(scratchB);
    }
}