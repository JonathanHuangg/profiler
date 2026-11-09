// stats.h — timing statistics for the profiler benchmark.
//
// Collects per-repetition timings (nanoseconds) and summarises them as
// mean / median / p95 / min / max, plus nanoseconds-per-operation.
// Implemented in stats.cpp.
#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <string>
#include <vector>

namespace profiler {

// Wall-clock timer. Start it, then call elapsed_ns() as often as needed.
class Timer {
  public:
    Timer() : start_(std::chrono::steady_clock::now()) {}
    void reset() { start_ = std::chrono::steady_clock::now(); }
    double elapsed_ns() const {
        return static_cast<double>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - start_)
                .count());
    }

  private:
    std::chrono::steady_clock::time_point start_;
};

struct Stats {
    double mean_ns = 0;
    double median_ns = 0;
    double p95_ns = 0;
    double min_ns = 0;
    double max_ns = 0;
    double stddev_ns = 0;
    // Nanoseconds per operation, derived from mean over `ops_per_rep`.
    double ns_per_op = 0;

    // Summarise `samples` (one timing per repetition). `ops_per_rep` is the
    // number of operations each repetition performed.
    static Stats summarize(const std::vector<double>& samples, std::size_t ops_per_rep);

    std::string to_string() const;
};

}  // namespace profiler
