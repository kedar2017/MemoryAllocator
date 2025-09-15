#include "../include/linear.h"
#include <cstdio>

int main () {
    struct TestStruct {
        int a = 0;
        int b = 0;
        int c = 0;
    };

    LinearAlloc temp;
    temp.init(100);
    auto temp_alloc = static_cast<TestStruct*> (temp.allocate(sizeof(TestStruct)));
    int* temp_alloc_new = static_cast<int*> (temp.allocate(sizeof(int)));
}