#include "allocator.h"

inline size_t alignment (void* a) {
    return (((size_t) a + 63) & ~(63)) - (size_t)a;
}

class LinearAlloc: Allocator {
public:

    size_t alloc_size_ = 0;
    void* ptr_;
    size_t offset_ = 0;
    void* base_;

    ~LinearAlloc () {free(base_);}

    void init (size_t size) {
        alloc_size_ = size;
        ptr_ = static_cast<void*> (malloc(size));
        base_ = ptr_;
    }

    void* allocate (uint32_t size) {
        size_t adjust = alignment(ptr_);
        void* curr = ptr_;
        offset_ = offset_ + adjust + size;
        if (offset_ > alloc_size_) return nullptr;
        ptr_ = static_cast<char*>(ptr_) + adjust + size;
        return reinterpret_cast<void*>((size_t)curr + adjust);
    }

    void deallocate () {
        free(base_);
    }
};