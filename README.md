# MemoryAllocator

> Custom memory allocators you can drop into micro-benchmarks to remove `malloc`/`free` overheads.  
> Includes tiled GEMM, graph builder, and multi-threaded allocation benchmarks.

This repository is a **tiny, focused memory allocator playground**:

- A small **bump / arena allocator** (`LinearAlloc`)
- A **benchmark harness** comparing `LinearAlloc` vs plain `malloc` in a few realistic workloads  
  (GEMM, graph building, multi-threaded allocation storm)

It’s designed to be easy to read in an interview and easy to drop into your own micro-benchmarks or toy engines.

---

## Table of contents

- [Motivation](#motivation)
- [What’s included](#whats-included)
- [Allocator design](#allocator-design)
- [Metrics & how to read them](#metrics--how-to-read-them)
- [Repository layout](#repository-layout)
- [Building & running](#building--running)
- [Results comparison](#results-comparison)
- [Example: using `LinearAlloc` in your code](#example-using-linearalloc-in-your-code)
- [Benchmarks: scenarios & intuition](#benchmarks-scenarios--intuition)
- [Future work](#future-work)

---

## Motivation

Traditional `malloc`/`free` is fantastic for general use, but it comes with:

- **Per-allocation metadata & bookkeeping**
- **Fragmentation** over time
- **Lock contention** in multi-threaded code
- Less predictable **performance in tight inner loops**

In many performance-critical paths, the pattern is actually much simpler:

- Allocate a bunch of objects during a phase / frame
- Throw them all away together
- Repeat next frame / phase

For those cases, a **bump (arena) allocator** is ideal:

- Hands out memory by moving a single pointer forward
- **No per-object free** — you just **reset** the arena
- Very small, predictable overhead

This repo explores that pattern and compares it against `malloc` on concrete workloads.

---

## What’s included

- **`LinearAlloc`**  
  A simple bump allocator with:
  - Alignment support
  - Simple growth policy
  - Basic telemetry / metrics

- **Benchmarks vs `malloc`:**
  - **GEMM 32×32**  
    100k+ iterations over tiled matrix multiplies.
  - **Graph build**  
    Builds a `k`-ary tree / graph using a pre-sized arena.
  - **Multi-thread allocation storm**  
    Each thread uses its own arena vs all threads sharing `malloc`.

- **Fuzz tests (`include/tests/fuzz_linear.h`)**  
  Randomized allocation/reset patterns that stress the allocator and help catch:
  - Off-by-one errors in pointer arithmetic  
  - Alignment / padding issues  
  - Bugs around arena exhaustion / growth logic

These are all small, self-contained examples focused on *allocator behavior*, not on sophisticated math or data structures.

---

## Allocator design

`LinearAlloc` is a classic arena/bump allocator:

- **Backing storage:**  
  A contiguous chunk: `[base_, base_ + alloc_size_)`

- **Single grow-only pointer:**

  ```text
  base_  ...  ptr_  ...  base_ + alloc_size_
           ^– next allocation comes from here
- **Alignment**
  Requests are rounded up to a power-of-two boundary (for example 64 bytes).
  Both the returned pointer and the internal ptr_ respect the chosen alignment.

- **Reset**
  Reset is O(1):
  ```
  alloc.reset();  // ptr_ = base_; all previous pointers become invalid

  ```

- **Overflow / growth policy**
  When the arena doesn’t have enough space:
  - Allocate a new backing block
  - Use a simple growth policy (e.g., 2–4× the requested size)
  - Keep the implementation easy to reason about for interview discussions
 
- **No per-object frees**
  Individual free operations are not supported.
  You either:
  - Never free (e.g., long-lived arena), or
  - Call reset() / destroy the arena when you’re done
 

## Metrics & How to Read Them

`LinearAlloc` tracks a few internal stats to help you reason about your workload:

- **`total_bytes_allocated_`**  
  Sum of all rounded-up request sizes returned by `allocate()`.  
  → How much memory your workload actually eats (including alignment padding).

- **`alloc_counter_`**  
  Count of successful `allocate()` calls that did *not* trigger a capacity bump.  
  → Sanity check that your steady-state usage fits within your arena size.

- **`peak_bytes_`**  
  Maximum observed `ptr_ - base_` distance.  
  → Essentially your “minimum viable arena size” for that workload.

- **`capacity_bump_count_`**  
  How many times you ran out of space and had to allocate a new backing block.  
  → If this is `> 0` during steady state, your arena is under-sized.

These metrics are especially useful when you run the benchmarks and want to translate results into:

- “What arena size should I pick in a real system?”
- “How sensitive is my app to rare arena growth events?”


## Repository Layout

Rough structure (adjust the file names if they differ):

- `include/linear_alloc.h`  
  Header-only bump allocator (`LinearAlloc` and related helpers).

- `src/benchmark.cpp`  
  Benchmarks:
  - Tiled GEMM  
  - Graph builder  
  - Multi-threaded allocation storm  

- `README.md`  
  This document.

The repo is intentionally small and easy to navigate.


## Building & Running

### Requirements

- C++17 or newer
- A POSIX-like environment (Linux/macOS) for the current benchmarks
- `-pthread` support for the multi-threaded benchmark

### Build

Example using `g++`:

```bash
git clone https://github.com/kedar2017/MemoryAllocator.git
cd MemoryAllocator

# Basic benchmark build (adjust sources as needed)
g++ -std=c++17 -O3 -pthread \
    -Iinclude \
    src/benchmark.cpp \
    -o alloc_bench
```

### Run 

```
./alloc_bench
```

Typical output (conceptually):

- Per-benchmark timings
- Comparison between malloc and LinearAlloc
- Key metrics (peak bytes, capacity bumps, etc.)

## Results comparison 

### x86 benchmark results (current run)

These results were collected on an **x86 machine**.  
I plan to repeat the same benchmarks on an **NVIDIA Jetson Orin (ARM)** to compare allocator behavior across architectures.

---

#### GEMM 32×32 – timing vs `malloc` (x86)

Speedup is `malloc_time / linear_time` (> 1.0 means `LinearAlloc` is faster).

| Tile size | `malloc` time (ms) | `LinearAlloc` time (ms) | Speedup (×) |
|-----------|--------------------|-------------------------|-------------|
| 2         | 8838.05            | 8849.53                 | 1.00×       |
| 4         | 8914.53            | 8901.45                 | 1.00×       |
| 8         | 9071.24            | 8896.82                 | 1.02×       |
| 16        | 9001.09            | 8857.50                 | 1.02×       |

Even when timings are close for small tiles, `LinearAlloc` avoids any capacity bumps and keeps the arena metrics clean.

---

#### GEMM – allocator metrics per matrix (x86)

The same metrics are reported for Matrix A and Matrix B at each tile size.

| Tile size | Matrix | `total_bytes_allocated_` | `alloc_counter_` | `peak_bytes_` | `capacity_bump_count_` |
|-----------|--------|--------------------------|------------------|---------------|-------------------------|
| 2         | A      | 102,400,000              | 1,600,000        | 1024          | 0                       |
| 2         | B      | 102,400,000              | 1,600,000        | 1024          | 0                       |
| 4         | A      | 204,800,000              | 2,400,000        | 1024          | 0                       |
| 4         | B      | 204,800,000              | 2,400,000        | 1024          | 0                       |
| 8         | A      | 307,200,000              | 2,800,000        | 1024          | 0                       |
| 8         | B      | 307,200,000              | 2,800,000        | 1024          | 0                       |
| 16        | A      | 409,600,000              | 3,000,000        | 1024          | 0                       |
| 16        | B      | 409,600,000              | 3,000,000        | 1024          | 0                       |

Key point: **no capacity bumps** across all tile sizes; peak usage per arena is tiny (1 KB) relative to the total bytes allocated over the whole run.

---

#### Multi-thread allocation storm – timing & metrics (x86)

| Variant               | Time (ms) | Speedup vs `malloc` (×) |
|-----------------------|-----------|--------------------------|
| `malloc`              | 669.109   | —                        |
| `LinearAlloc` aligned | 624.747   | **1.07×**                |

Allocator stats for the multi-thread run:

| Metric                   | Value         |
|--------------------------|--------------:|
| `total_bytes_allocated_` | 1,840,700,256 |
| `alloc_counter_`         | 19,173,961    |
| `peak_bytes_`            | 1,840,700,256 |
| `capacity_bump_count_`   | 0             |

This run allocates ~**1.84 GB** across ~**19.2M** allocations with **zero growth events**, and `LinearAlloc` is about **7% faster** than `malloc` on this workload.

---

#### Graph building benchmark – timing (x86)

| Variant       | Time (ms) | Relative to `LinearAlloc` |
|---------------|-----------|---------------------------|
| `LinearAlloc` | 494.738   | —                         |
| `malloc`      | 818.403   | **~1.65× slower**         |

On this x86 machine, the graph-building benchmark runs about **1.65× faster** with `LinearAlloc` than with `malloc`.


## Example: Using `LinearAlloc` in Your Code

This is a sketch of how `LinearAlloc` is typically used.  
You may need to adjust names slightly to match the actual header.

```cpp
#include "linear_alloc.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

struct Node {
    float x, y, z;
    uint32_t id;
};

int main() {
    // 1 MB arena for this example
    constexpr std::size_t arena_size = 1 << 20;

    // Backing storage can be heap, static, or from another allocator
    void* backing = std::malloc(arena_size);
    if (!backing) {
        std::cerr << "Failed to allocate backing storage\n";
        return 1;
    }

    LinearAlloc alloc{backing, arena_size};

    // Allocate some nodes
    constexpr std::size_t N = 10'000;
    Node* nodes = static_cast<Node*>(alloc.allocate(sizeof(Node) * N));

    for (std::size_t i = 0; i < N; ++i) {
        nodes[i].id = static_cast<uint32_t>(i);
        nodes[i].x  = float(i);
        nodes[i].y  = float(i) * 2.0f;
        nodes[i].z  = float(i) * 3.0f;
    }

    std::cout << "Peak bytes used: " << alloc.peak_bytes() << "\n";

    // When the whole graph/frame is done, reset in O(1)
    alloc.reset();

    // Optional: if you own the backing storage
    std::free(backing);
    return 0;
}
```

### Typical Patterns

- **Frame allocator**  
  Use one arena per frame, `reset()` at the start of the next frame.

- **Scratch allocator**  
  Use for temporary buffers inside a subsystem, `reset()` when the scope ends.

- **One-shot build**  
  Use for building graphs or trees where the whole structure is discarded as a unit.


## Benchmarks: Scenarios & Intuition

The benchmarks are chosen to hit different stress patterns:


### 1. GEMM 32×32 (100k+ iterations; tile sweep)

**Allocations:**

- Small, repeated, predictable  
- Good fit for a frame or scratch allocator  

**Expectation:**

- `LinearAlloc` eliminates per-iteration `malloc` overhead  
- CPU time becomes dominated by the math, not allocation  


### 2. Graph Build (k-ary tree, pre-sized arena)

**Allocations:**

- Many small nodes with pointer links  

**Arena sizing:**

- `peak_bytes_` tells you how big the arena needs to be  

**Expectation:**

- `LinearAlloc` significantly reduces allocator overhead vs `malloc`  
- You also avoid fragmentation over the lifetime of the graph  


### 3. Multi-thread Allocation Storm (per-thread arenas vs plain malloc)

**Allocations:**

- Many threads hammering allocations concurrently  

**Scenarios:**

- **Baseline:** all threads use `malloc` / `free`  
- **Arena:** each thread uses its own `LinearAlloc`  

**Expectation:**

- Per-thread arenas greatly reduce lock contention in the allocator  
- Easier to reason about scalability and NUMA effects  


## Future Work

Some natural extensions to this repo:


### More allocator types

- Pool / slab allocator for fixed-size objects  
- Stack allocator (LIFO semantics)  
- Segregated free list or free-list-backed general allocator  


### Allocator trait / concept

- Shared interface for benchmarks: `allocate`, `reset`, etc.  
- Plug-and-play comparisons between different allocator types  


### Better benchmark reporting

- p50 / p95 / p99 allocation latency stats  
- CSV or JSON output for easy plotting  

