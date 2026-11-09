// main.cpp — the profiler benchmark.
//
// This file used to be only the design note below. It is now the tool the
// note describes:
//
//   insert 2M nodes
//   custom allocator or posix_memalign to not allow OS to put it to random
//   places (new does not guarantee alignment)
//   shuffle search queries to not allow prefetching
//
// What it does, per run:
//   * correctness self-checks on every structure (small N, before timing),
//   * bulk insert of N keys, then point lookups over a shuffled query set
//     (so the CPU cannot prefetch the next key) and over the same keys in
//     order, for comparison,
//   * each phase repeated `--reps` times and summarised as mean / median /
//     p95 / min / max, in ns per operation,
//   * every hand-rolled structure measured against its stdlib counterpart
//     (unordered_map, map, forward_list) under the same keys and seed.
//
// Keys and query buffers live in one 64-byte-aligned pool (see alloc.h).
// Node allocation inside the structures still uses `new`: that allocation
// behaviour is part of what is being measured, not hidden by the harness.
// BST keys are inserted in shuffled order — an unbalanced BST fed sorted
// keys degenerates to a linked list, which would measure the input order,
// not the tree.
//
// Usage:
//   make                 # builds ./profiler
//   ./profiler                     # defaults: N=2,000,000, reps=3
//   ./profiler --n 200000 --reps 5 --only hash --csv results.csv
//   ./profiler --help
//
// Exit code is 0 on a clean run, 1 if a self-check fails, 2 on bad flags.
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <forward_list>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "src/alloc.h"
#include "src/bst.h"
#include "src/hash_map.h"
#include "src/stats.h"

// linked_lists.cpp has its own test main(); PROFILER_BENCH compiles it as
// a header so the benchmark can time the two hand-rolled lists.
#define PROFILER_BENCH
#include "linked_lists.cpp"
#undef PROFILER_BENCH

namespace {

struct Options {
    std::size_t n = 2'000'000;
    int reps = 3;
    std::uint64_t seed = 42;
    std::string only = "all";
    std::string csv_path;
};

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n"
              << "  --n N        keys per structure (default 2000000)\n"
              << "  --reps R     repetitions per phase (default 3)\n"
              << "  --seed S     RNG seed, for reproducible shuffles (default 42)\n"
              << "  --only X     hash | bst | list | all (default all)\n"
              << "  --csv FILE   also write one row per phase to FILE\n"
              << "  --help       print this and exit\n";
}

bool parse_args(int argc, char** argv, Options& out) {
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto need_value = [&](const char* flag, std::string& dst) {
            if (i + 1 >= argc) {
                std::cerr << flag << " needs a value\n";
                return false;
            }
            dst = argv[++i];
            return true;
        };
        std::string value;
        if (arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        } else if (arg == "--n") {
            if (!need_value("--n", value)) return false;
            out.n = static_cast<std::size_t>(std::stoull(value));
        } else if (arg == "--reps") {
            if (!need_value("--reps", value)) return false;
            out.reps = std::stoi(value);
        } else if (arg == "--seed") {
            if (!need_value("--seed", value)) return false;
            out.seed = std::stoull(value);
        } else if (arg == "--only") {
            if (!need_value("--only", value)) return false;
            out.only = value;
        } else if (arg == "--csv") {
            if (!need_value("--csv", value)) return false;
            out.csv_path = value;
        } else {
            std::cerr << "unknown flag: " << arg << "\n";
            return false;
        }
    }
    if (out.n == 0 || out.reps <= 0) {
        std::cerr << "--n and --reps must be positive\n";
        return false;
    }
    if (out.only != "all" && out.only != "hash" && out.only != "bst" && out.only != "list") {
        std::cerr << "--only must be hash, bst, list, or all\n";
        return false;
    }
    return true;
}

// One benchmark row, printed as a table line and optionally as CSV.
struct Row {
    std::string structure;
    std::string phase;
    profiler::Stats stats;
};

void print_header() {
    std::cout << "\n"
              << "structure            phase           mean (ms)   median      p95         ns/op\n"
              << "-------------------  --------------  ----------  ----------  ----------  ----------\n";
}

void print_row(const Row& row) {
    std::cout.width(20);
    std::cout << std::left << row.structure;
    std::cout.width(16);
    std::cout << std::left << row.phase;
    std::cout.width(0);
    std::cout << std::right << std::fixed;
    std::cout.precision(1);
    std::cout << std::setw(10) << row.stats.mean_ns / 1e6 << "  " << std::setw(10)
              << row.stats.median_ns / 1e6 << "  " << std::setw(10) << row.stats.p95_ns / 1e6
              << "  " << std::setw(10) << row.stats.ns_per_op << "\n";
    std::cout << std::left;
}

// Runs `fn` `reps` times, timing each repetition. `fn` returns a checksum
// the caller folds into the sink so the compiler cannot drop the work.
profiler::Stats time_reps(int reps, std::size_t ops, const std::function<std::uint64_t()>& fn,
                          std::uint64_t& sink) {
    std::vector<double> samples;
    samples.reserve(static_cast<std::size_t>(reps));
    for (int i = 0; i < reps; ++i) {
        profiler::Timer t;
        sink += fn();
        samples.push_back(t.elapsed_ns());
    }
    return profiler::Stats::summarize(samples, ops);
}

// Self-checks. These run before any timing; a failure aborts the run with
// exit code 1, because timing a broken structure measures nothing.
bool self_check() {
    UnorderedMapChain_norm<int, int> map;
    if (!map.insert(1, 10) || !map.insert(2, 20) || map.insert(1, 99)) return false;
    if (map.find(1) == nullptr || *map.find(1) != 99) return false;
    if (map.find(3) != nullptr || !map.contains(2) || map.contains(3)) return false;
    if (!map.erase(2) || map.erase(2) || map.size() != 1) return false;
    UnorderedMapChain_norm<int, int> copy(map);  // copy keeps the survivor
    if (copy.size() != 1 || *copy.find(1) != 99) return false;
    UnorderedMapChain_norm<int, int> moved(std::move(copy));
    if (moved.size() != 1 || copy.size() != 0) return false;
    for (int i = 0; i < 1000; ++i) map.insert(i, i * 2);  // forces rehashes
    for (int i = 0; i < 1000; ++i) {
        if (map.find(i) == nullptr || *map.find(i) != i * 2) return false;
    }

    BSTAligned<int> aligned;
    BSTPacked<int> packed;
    for (int v : {5, 2, 8, 1, 3}) {
        aligned.insert(v);
        packed.insert(v);
    }
    if (aligned.size() != 5 || packed.size() != 5) return false;
    if (!aligned.contains(3) || aligned.contains(9)) return false;
    if (!packed.contains(1) || packed.contains(7)) return false;

    SinglyLinkedList_bare<int> bare;
    bare.push_front(1);
    bare.push_front(2);
    if (bare.size() != 2 || bare.read_list() != "2 1 ") return false;
    SinglyLinkedList_norm<int> norm;
    norm.push_front(1);
    SinglyLinkedList_norm<int> norm_copy(norm);
    if (norm_copy.size() != 1) return false;

    return true;
}

std::vector<Row> bench_hash(const std::uint32_t* keys, const std::uint32_t* queries,
                            std::size_t n, int reps, std::uint64_t& sink) {
    std::vector<Row> rows;
    // Hand-rolled chained hash map.
    {
        std::vector<double> insert_samples;
        UnorderedMapChain_norm<std::uint32_t, std::uint32_t> map;
        map.insert(0, 0);  // warm the allocator paths once, untimed
        map.clear();
        for (int r = 0; r < reps; ++r) {
            map.clear();
            profiler::Timer t;
            for (std::size_t i = 0; i < n; ++i) map.insert(keys[i], keys[i]);
            insert_samples.push_back(t.elapsed_ns());
        }
        rows.push_back({"hash_map (custom)", "insert",
                        profiler::Stats::summarize(insert_samples, n)});
        // Lookups run against the fully built map from the last rep.
        auto lookup = [&]() {
            std::uint64_t hits = 0;
            for (std::size_t i = 0; i < n; ++i) {
                if (const auto* v = map.find(queries[i])) hits += *v;
            }
            return hits;
        };
        rows.push_back({"hash_map (custom)", "lookup shuffled",
                        time_reps(reps, n, lookup, sink)});
    }
    // stdlib baseline, same keys, same order.
    {
        std::vector<double> insert_samples;
        std::unordered_map<std::uint32_t, std::uint32_t> map;
        map.reserve(n);
        for (int r = 0; r < reps; ++r) {
            map.clear();
            profiler::Timer t;
            for (std::size_t i = 0; i < n; ++i) map[keys[i]] = keys[i];
            insert_samples.push_back(t.elapsed_ns());
        }
        rows.push_back({"unordered_map (std)", "insert",
                        profiler::Stats::summarize(insert_samples, n)});
        auto lookup = [&]() {
            std::uint64_t hits = 0;
            for (std::size_t i = 0; i < n; ++i) {
                auto it = map.find(queries[i]);
                if (it != map.end()) hits += it->second;
            }
            return hits;
        };
        rows.push_back({"unordered_map (std)", "lookup shuffled",
                        time_reps(reps, n, lookup, sink)});
    }
    return rows;
}

std::vector<Row> bench_bst(const std::uint32_t* keys, const std::uint32_t* queries,
                           std::size_t n, int reps, std::uint64_t& sink) {
    std::vector<Row> rows;
    // Keys are already shuffled by the caller: sorted input would
    // degenerate these unbalanced trees into linked lists.
    auto run_one = [&](const std::string& name, auto make_tree) {
        using Tree = decltype(make_tree());
        std::vector<double> insert_samples;
        Tree tree;
        for (int r = 0; r < reps; ++r) {
            tree = Tree();  // fresh tree each rep; move assignment
            profiler::Timer t;
            for (std::size_t i = 0; i < n; ++i) tree.insert(keys[i]);
            insert_samples.push_back(t.elapsed_ns());
        }
        rows.push_back({name, "insert", profiler::Stats::summarize(insert_samples, n)});
        auto lookup = [&]() {
            std::uint64_t hits = 0;
            for (std::size_t i = 0; i < n; ++i) {
                if (tree.contains(queries[i])) hits += 1;
            }
            return hits;
        };
        rows.push_back({name, "lookup shuffled", time_reps(reps, n, lookup, sink)});
    };
    run_one("bst aligned (custom)", [] { return BSTAligned<std::uint32_t>(); });
    run_one("bst packed (custom)", [] { return BSTPacked<std::uint32_t>(); });
    {
        std::map<std::uint32_t, std::uint32_t> tree;
        std::vector<double> insert_samples;
        for (int r = 0; r < reps; ++r) {
            tree.clear();
            profiler::Timer t;
            for (std::size_t i = 0; i < n; ++i) tree[keys[i]] = keys[i];
            insert_samples.push_back(t.elapsed_ns());
        }
        rows.push_back({"map (std, red-black)", "insert",
                        profiler::Stats::summarize(insert_samples, n)});
        auto lookup = [&]() {
            std::uint64_t hits = 0;
            for (std::size_t i = 0; i < n; ++i) {
                if (tree.find(queries[i]) != tree.end()) hits += 1;
            }
            return hits;
        };
        rows.push_back({"map (std, red-black)", "lookup shuffled",
                        time_reps(reps, n, lookup, sink)});
    }
    return rows;
}

std::vector<Row> bench_list(const std::uint32_t* keys, std::size_t n, int reps,
                            std::uint64_t& sink) {
    std::vector<Row> rows;
    {
        std::vector<double> samples;
        for (int r = 0; r < reps; ++r) {
            SinglyLinkedList_bare<std::uint32_t> list;
            profiler::Timer t;
            for (std::size_t i = 0; i < n; ++i) list.push_front(keys[i]);
            samples.push_back(t.elapsed_ns());
            sink += list.size();
        }
        rows.push_back({"list bare (custom)", "push_front",
                        profiler::Stats::summarize(samples, n)});
    }
    {
        std::vector<double> samples;
        for (int r = 0; r < reps; ++r) {
            SinglyLinkedList_norm<std::uint32_t> list;
            profiler::Timer t;
            for (std::size_t i = 0; i < n; ++i) list.push_front(keys[i]);
            samples.push_back(t.elapsed_ns());
            sink += list.size();
        }
        rows.push_back({"list norm (custom)", "push_front",
                        profiler::Stats::summarize(samples, n)});
    }
    {
        std::vector<double> samples;
        for (int r = 0; r < reps; ++r) {
            std::forward_list<std::uint32_t> list;
            profiler::Timer t;
            for (std::size_t i = 0; i < n; ++i) list.push_front(keys[i]);
            samples.push_back(t.elapsed_ns());
            sink += static_cast<std::uint64_t>(std::distance(list.begin(), list.end()));
        }
        rows.push_back({"forward_list (std)", "push_front",
                        profiler::Stats::summarize(samples, n)});
    }
    return rows;
}

}  // namespace

int main(int argc, char** argv) {
    Options opts;
    if (!parse_args(argc, argv, opts)) {
        print_usage(argv[0]);
        return 2;
    }

    std::cout << "profiler — hand-rolled structures vs stdlib\n"
              << "N=" << opts.n << " reps=" << opts.reps << " seed=" << opts.seed
              << " cache line=" << CACHE_LINE << " bytes\n";

    if (!self_check()) {
        std::cerr << "self-check FAILED — refusing to benchmark broken structures\n";
        return 1;
    }
    std::cout << "self-check passed\n";

    // One aligned pool holds both buffers, so their addresses are
    // deterministic run to run. Keys 0..N-1 shuffled once (insert order
    // and BST shape both depend on it); queries are an independent shuffle
    // of the same keys, so lookups cannot be prefetched in order.
    profiler::AlignedPool pool(2 * opts.n * sizeof(std::uint32_t) + 4096);
    auto* keys = static_cast<std::uint32_t*>(pool.allocate(opts.n * sizeof(std::uint32_t)));
    auto* queries =
        static_cast<std::uint32_t*>(pool.allocate(opts.n * sizeof(std::uint32_t)));
    std::iota(keys, keys + opts.n, 0u);
    std::memcpy(queries, keys, opts.n * sizeof(std::uint32_t));
    std::mt19937_64 rng(opts.seed);
    std::shuffle(keys, keys + opts.n, rng);
    std::shuffle(queries, queries + opts.n, rng);

    std::uint64_t sink = 0;  // folded checksums; printed so work is observable
    std::vector<Row> rows;
    if (opts.only == "all" || opts.only == "hash") {
        auto r = bench_hash(keys, queries, opts.n, opts.reps, sink);
        rows.insert(rows.end(), r.begin(), r.end());
    }
    if (opts.only == "all" || opts.only == "bst") {
        auto r = bench_bst(keys, queries, opts.n, opts.reps, sink);
        rows.insert(rows.end(), r.begin(), r.end());
    }
    if (opts.only == "all" || opts.only == "list") {
        auto r = bench_list(keys, opts.n, opts.reps, sink);
        rows.insert(rows.end(), r.begin(), r.end());
    }

    print_header();
    for (const Row& row : rows) print_row(row);
    std::cout << "checksum (prevents dead-code elimination): " << sink << "\n";

    if (!opts.csv_path.empty()) {
        std::ofstream csv(opts.csv_path);
        csv << "structure,phase,mean_ns,median_ns,p95_ns,min_ns,max_ns,ns_per_op\n";
        for (const Row& row : rows) {
            csv << '"' << row.structure << "\"," << row.phase << ',' << row.stats.mean_ns << ','
                << row.stats.median_ns << ',' << row.stats.p95_ns << ',' << row.stats.min_ns
                << ',' << row.stats.max_ns << ',' << row.stats.ns_per_op << "\n";
        }
        std::cout << "wrote " << opts.csv_path << "\n";
    }
    return 0;
}
