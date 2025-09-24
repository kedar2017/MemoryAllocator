#include "allocator.h"

class LinearAlloc: Allocator {
public:

    size_t alloc_size_ = 0;
    void* ptr_;
    void* base_;

    ~LinearAlloc () {free(base_);}

    void init (size_t size) {
        base_ = static_cast<void*> (malloc(size));
        alloc_size_ = size;
        ptr_ = base_;
    }

    void* allocate (size_t size) {
        size_t curr = (size_t)ptr_;
        size_t next = curr + size;
        if (next - (size_t)base_ > alloc_size_) {
            return nullptr;
        }
        ptr_ = (void*)(next);
        return reinterpret_cast<void*>(curr);
    }

    void deallocate () {
        free(base_);
    }
};