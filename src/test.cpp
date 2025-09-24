#include "../include/workload/gemm.h"
#include <cstdio>

int main () {
    std::vector<uint8_t> A(1024 * 1024, 2);
    std::vector<uint8_t> B(1024 * 1024, 3);
    std::vector<uint16_t> C(1024 * 1024);

    uint8_t* A_ptr = reinterpret_cast<uint8_t*>(A.data());
    uint8_t* B_ptr = reinterpret_cast<uint8_t*>(B.data());
    uint16_t* C_ptr = reinterpret_cast<uint16_t*>(C.data());

    gemm(A_ptr, B_ptr, C_ptr, 1024, 24);
}