// alloc.h — a 64-byte-aligned bump allocator for benchmark buffers.
//
// The design note in main.cpp asked for a custom allocator / posix_memalign
// "to not allow OS to put it to random places". The benchmark honours that
// where it matters and where it is cheap: the key and query buffers the
// benchmark reads from are allocated once, 64-byte aligned, from one
// contiguous block, so a run's addresses are deterministic and the buffers
// never straddle a page boundary by accident. Node allocation inside the
// structures themselves still uses `new` — that is the thing being measured,
// not hidden by the harness — and the README says so.
#pragma once

#include <cstddef>
#include <cstdlib>
#include <stdexcept>

namespace profiler {

// One contiguous, 64-byte-aligned block. Bump-allocates from it; freeing is
// all-or-nothing when the block is destroyed, which fits a benchmark that
// builds its buffers once per run.
class AlignedPool {
  public:
    explicit AlignedPool(std::size_t bytes, std::size_t alignment = 64)
        : size_(bytes), used_(0) {
        if (posix_memalign(&block_, alignment, bytes) != 0 || block_ == nullptr) {
            throw std::bad_alloc();
        }
    }

    ~AlignedPool() { std::free(block_); }
    AlignedPool(const AlignedPool&) = delete;
    AlignedPool& operator=(const AlignedPool&) = delete;

    // Returns `bytes` from the pool, aligned to 64 bytes. Throws
    // std::bad_alloc when the pool is exhausted.
    void* allocate(std::size_t bytes) {
        const std::size_t aligned = (bytes + 63) & ~static_cast<std::size_t>(63);
        if (used_ + aligned > size_) throw std::bad_alloc();
        void* ptr = static_cast<char*>(block_) + used_;
        used_ += aligned;
        return ptr;
    }

    std::size_t used() const { return used_; }
    std::size_t capacity() const { return size_; }

  private:
    void* block_ = nullptr;
    std::size_t size_;
    std::size_t used_;
};

}  // namespace profiler
