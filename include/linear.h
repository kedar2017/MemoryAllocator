#pragma once
#include "allocator.h"

size_t alignment (size_t addr, size_t align_len) {
    return ((addr + (align_len - 1)) & ~(align_len-1));
}

class LinearAlloc: Allocator {
public:

    size_t alloc_size_ = 0;
    void* ptr_;
    void* base_;
    void* raw_;
    std::vector<void*> ptr_tracker_;
    size_t init_alloc_size_ = 0;

    // Metrics 
    size_t total_bytes_allocated_ = 0;
    size_t capacity_bump_count_ = 0;
    size_t alloc_counter_ = 0;
    size_t peak_bytes_ = 0;

    ~LinearAlloc () {
        free(ptr_tracker_[0]);
    }

    void init (size_t size, size_t align_len = 64) {
        size = alignment(size, align_len);
        raw_ = static_cast<void*> (malloc(size + align_len));
        ptr_tracker_.push_back(raw_);
        base_ = (void*)alignment((size_t)raw_, align_len);
        alloc_size_ = size;
        ptr_ = base_;
        init_alloc_size_ = alloc_size_;
    }

    void* allocate (size_t size, size_t align_len = 64) {
        size = alignment(size, align_len);
        size_t curr = (size_t)ptr_;
        size_t next = curr + size;
        if (next - (size_t)base_ > alloc_size_) {
            capacity_bump_count_++;
            void* temp = static_cast<void*> (malloc(4 * size + align_len));
            alloc_size_ = 4 * size;
            ptr_tracker_.push_back(temp);
            raw_ = temp;
            base_ = (void*)alignment((size_t)raw_, align_len);
            size_t curr_off = (size_t)base_;
            size_t next_off = curr_off + size;
            ptr_ = (void*)next_off;
            total_bytes_allocated_ += size;
            return reinterpret_cast<void*>(curr_off);
        }
        alloc_counter_++;
        total_bytes_allocated_ += size;
        ptr_ = (void*)(next);
        if ((size_t)ptr_ - (size_t)base_ > peak_bytes_) {
            peak_bytes_ = (size_t)ptr_ - (size_t)base_;
        }
        return reinterpret_cast<void*>(curr);
    }

    // Resets and keep just the first/original chunk
    void reset (size_t align_len = 64) {
        raw_ = ptr_tracker_[0];
        base_ = (void*)alignment((size_t)raw_, align_len);
        ptr_ = base_;
        alloc_size_ = init_alloc_size_;
        for (int i = ptr_tracker_.size() - 1; i > 0; i--) {
            free(ptr_tracker_[i]);
            if (!ptr_tracker_.empty()) {
                ptr_tracker_.pop_back();
            }
        }
    }
};