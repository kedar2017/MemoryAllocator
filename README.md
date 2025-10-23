# MemoryAllocator

TL;DR: A tiny bump allocator you can drop into micro‑benchmarks and toy engines to remove malloc/free overhead from tight loops. Includes three simple benchmarks: tiled GEMM, graph builder, and allocation storm (multi‑threaded).


- Traditional malloc/free can be costly in some aspects (thread contention, fragmentation).

- A bump (arena) allocator hands out memory by moving a single pointer forward. Reset is O(1). No per‑allocation metadata, no frees.

- Perfect for patterns like frame allocators, temporary scratch buffers, and one‑shot graph construction.

What's included:

- LinearAlloc — a single‑header bump allocator with alignment support and basic telemetry.
- Benchmarks comparing against malloc:

GEMM 32×32 (100k iterations; tile sweep)

Graph build (k‑ary tree, pre‑sized arena)

Multi‑thread allocation storm (per‑thread arenas vs plain malloc)


Design overview:

- Single grow‑only pointer (ptr_) inside a contiguous chunk (base_ … base_+alloc_size_).

- Alignment: rounds allocation sizes and returned addresses up to a power‑of‑two boundary (defaults to 64 bytes).

- Reset returns the arena to its base instantly; previously returned pointers become invalid.

- Growth policy on overflow: allocate a new backing block (current code uses 4× request; see TODOs for alternatives).

- Metrics: total_bytes_allocated_, alloc_counter_, peak_bytes_, capacity_bump_count_


Metrics & how to read them

- total_bytes_allocated_: sum of rounded‑up request sizes returned by allocate(). Helpful for sizing the arena.

- alloc_counter_: number of successful allocate() calls that didn’t trigger a capacity bump.

- peak_bytes_: max distance ptr_ - base_ — use this as the “minimum viable arena size” for a given workload.

- capacity_bump_count_: number of times we ran out of space and had to allocate a new backing block.


Benchmark results:

TODO: update this 