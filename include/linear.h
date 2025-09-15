#include "allocator.h"

class LinearAlloc: Allocator {
public:

    uint32_t alloc_size_ = 0;
    void* ptr_;
    uint32_t offset_ = 0;
    void* base_;

    ~LinearAlloc () {free(base_);}

    void init (uint32_t size) {
        alloc_size_ = size;
        ptr_ = static_cast<void*> (malloc(size));
        base_ = ptr_;
    }

    void* allocate (uint32_t size) {
        if ((offset_ + size) > alloc_size_) return nullptr;
        void* curr = ptr_;
        ptr_ = static_cast<char*>(ptr_) + size;
        offset_ = offset_ + size;
        return curr;
    }

    void deallocate () {
        free(base_);
    }
};