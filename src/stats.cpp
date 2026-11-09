// stats.cpp — see stats.h.
#include "stats.h"

#include <iomanip>
#include <sstream>

namespace profiler {

Stats Stats::summarize(const std::vector<double>& samples, std::size_t ops_per_rep) {
    Stats s;
    if (samples.empty()) return s;

    std::vector<double> sorted = samples;
    std::sort(sorted.begin(), sorted.end());

    s.min_ns = sorted.front();
    s.max_ns = sorted.back();
    s.mean_ns = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();

    const std::size_t mid = sorted.size() / 2;
    s.median_ns = (sorted.size() % 2 == 0)
                      ? (sorted[mid - 1] + sorted[mid]) / 2.0
                      : sorted[mid];

    const std::size_t p95_idx =
        std::min(sorted.size() - 1,
                 static_cast<std::size_t>(std::ceil(0.95 * sorted.size())) - 1);
    s.p95_ns = sorted[p95_idx];

    double sq = 0;
    for (double v : sorted) sq += (v - s.mean_ns) * (v - s.mean_ns);
    s.stddev_ns = std::sqrt(sq / sorted.size());

    if (ops_per_rep > 0) s.ns_per_op = s.mean_ns / static_cast<double>(ops_per_rep);
    return s;
}

std::string Stats::to_string() const {
    std::ostringstream os;
    os << std::fixed << std::setprecision(1) << "mean " << mean_ns / 1e6 << " ms"
       << "  median " << median_ns / 1e6 << " ms"
       << "  p95 " << p95_ns / 1e6 << " ms"
       << "  min " << min_ns / 1e6 << " ms"
       << "  max " << max_ns / 1e6 << " ms"
       << "  (" << std::setprecision(1) << ns_per_op << " ns/op)";
    return os.str();
}

}  // namespace profiler
