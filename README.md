# profiler

A small C++ benchmark: hand-rolled data structures measured against the
stdlib under the same keys, the same seed, and the same phases. It grew
out of a study repo — the structures were the point, and this makes the
comparison they were built for actually runnable.

## What it measures

| Structure | stdlib baseline | Phases |
| --- | --- | --- |
| `UnorderedMapChain_norm` — chained hash map (`src/hash_map.h`) | `std::unordered_map` | insert, shuffled lookup |
| `BSTAligned` / `BSTPacked` — binary search tree, cache-line-aligned vs packed nodes (`src/bst.h`) | `std::map` | insert, shuffled lookup |
| `SinglyLinkedList_bare` / `_norm` (`linked_lists.cpp`) | `std::forward_list` | push_front |

Every run starts with correctness self-checks (insert/find/erase, copy
and move, rehashing, BST membership, list contents). A failed check aborts
the run — timing a broken structure measures nothing.

The harness follows the design note this repo started with: keys and
query buffers live in one 64-byte-aligned pool (`posix_memalign`, see
`src/alloc.h`), and lookups run over an independently shuffled query set
so the CPU cannot prefetch the next key. BST keys are inserted in
shuffled order — sorted input would degenerate an unbalanced BST into a
linked list and measure the input order, not the tree. Node allocation
inside the structures still uses `new`: that allocation behaviour is part
of what is being measured, not hidden by the harness.

Each phase runs `--reps` times and is reported as mean / median / p95 /
min / max, in ns per operation. A checksum of the work is printed so the
compiler cannot optimise the benchmark away.

## Build and run

Needs a C++20 compiler (verified with g++ 13.3.0 on Ubuntu 24.04).

```sh
make test    # builds everything, runs the checks (N=200k, fast)
make bench   # default benchmark: N=2,000,000, 3 reps
./profiler --n 200000 --reps 5 --only hash --csv results.csv
./profiler --help
```

`--only` selects `hash`, `bst`, `list`, or `all`. `--seed` makes the
shuffles reproducible.

## What a run looks like

N=200k, 3 reps, this machine (g++ 13.3, `-O3`):

```
hash_map (custom)    insert          14.6 ms   73 ns/op
unordered_map (std)  insert           6.5 ms   32 ns/op
bst aligned (custom) insert         207.2 ms  1036 ns/op
bst packed (custom)  insert         144.7 ms   723 ns/op
map (std, red-black) insert          99.2 ms   496 ns/op
list bare (custom)   push_front       2.7 ms    13 ns/op
forward_list (std)   push_front       2.0 ms    10 ns/op
```

Two results worth noting, both from the same run rather than from first
principles: the chained map's shuffled lookups beat `unordered_map`
(5.4 vs 7.6 ns/op at N=200k — small tables, hot cache), and the
cache-line-aligned BST is *slower* than the packed one on both insert
and lookup. Padding each node to a full line costs more in memory
traffic than it saves in false sharing for a pointer-chasing tree.
Re-run it on your own machine before believing either number — that is
what the tool is for.

The default run (`make bench`, N=2M, 3 reps) takes about 4 minutes on
this machine and the picture holds, with the gap widening once the
structures no longer fit in cache:

```
hash_map (custom)    insert         490.9 ms   245 ns/op
unordered_map (std)  insert         147.1 ms    74 ns/op
bst aligned (custom) insert       10442.2 ms  5221 ns/op
bst packed (custom)  insert        9882.1 ms  4941 ns/op
map (std, red-black) insert       10248.9 ms  5124 ns/op
list bare (custom)   push_front     111.2 ms    56 ns/op
forward_list (std)   push_front     432.3 ms   216 ns/op
```

## Layout

- `main.cpp` — the benchmark: flags, self-checks, phases, output.
- `src/hash_map.h` — the chained hash map (completed `maps.cpp`).
- `src/bst.h` — the BST, aligned and packed layouts (completed `tree.cpp`).
- `src/stats.h`, `src/stats.cpp` — mean / median / p95 / min / max.
- `src/alloc.h` — the 64-byte-aligned pool for key/query buffers.
- `linked_lists.cpp` — the two singly linked lists and their test suite;
  still builds standalone (`g++ linked_lists.cpp`), and the benchmark
  includes it with `PROFILER_BENCH` defined to time the lists.
- `tests/run_tests.sh` — `make test`: builds, runs the list suite, runs
  the benchmark small, checks the CSV has every structure, checks
  same-seed reproducibility, and checks bad flags are rejected.
- `Networking.md`, `effective_cpp.md`, `tour_of_cpp.md` — the study
  notes this repo started as, untouched.

## History of this change

`maps.cpp` did not compile (duplicate `clear()`, a missing semicolon)
and had no insert/find/erase; it is replaced by `src/hash_map.h`.
`tree.cpp` did not compile (missing semicolons/braces, and the
`__cpp_lib_hardware_interference_size` guard was inverted — it used the
constant when the macro was *not* defined); it is replaced by `src/bst.h`,
which keeps the aligned-vs-packed experiment and makes it measurable.
`main.cpp` was a three-line comment describing a benchmark; it is now the
benchmark. `stats.cpp` / `stats.h` were empty (0 bytes); the stats code
lives in `src/` now. The checked-in `linked_list` binary (a stale macOS
arm64 build artifact) is deleted; `make` rebuilds it. The only change to
`linked_lists.cpp` itself is a `size()` accessor on the `_norm` list (its
sibling already had one) and a `PROFILER_BENCH` guard around its test
`main()` so the benchmark can include it.
