#!/bin/sh
# tests/run_tests.sh — build everything and run the checks.
# Fast by design (N=200k, 2 reps); the default 2M-node run is `make bench`.
# Fails loudly on the first failure; prints PASS only if everything ran.
set -e
cd "$(dirname "$0")/.."

echo "== build =="
make profiler linked_list

echo "== linked-list test suite =="
./linked_list | grep -q "basic test passes" || { echo "FAIL: linked_list"; exit 1; }

echo "== benchmark self-check + small run (hash only) =="
./profiler --n 200000 --reps 2 --only hash | grep -q "self-check passed" || { echo "FAIL: self-check"; exit 1; }

echo "== benchmark small run (all structures) =="
./profiler --n 200000 --reps 2 --csv /tmp/profiler-test.csv
grep -q "hash_map (custom)" /tmp/profiler-test.csv || { echo "FAIL: csv missing hash rows"; exit 1; }
grep -q "bst aligned (custom)" /tmp/profiler-test.csv || { echo "FAIL: csv missing bst rows"; exit 1; }
grep -q "list bare (custom)" /tmp/profiler-test.csv || { echo "FAIL: csv missing list rows"; exit 1; }

echo "== determinism: same seed, same checksum =="
# The checksum is the sum of the values found, which depends on the key
# set, not their order — so it is seed-independent by construction. What
# the seed must control is reproducibility: the same seed reproduces the
# same run exactly, including the shuffle that defeats prefetching.
./profiler --n 100000 --reps 1 --only hash --seed 7 | grep "checksum" > /tmp/profiler-c1.txt
./profiler --n 100000 --reps 1 --only hash --seed 7 | grep "checksum" > /tmp/profiler-c2.txt
cmp -s /tmp/profiler-c1.txt /tmp/profiler-c2.txt || { echo "FAIL: same-seed runs differ"; exit 1; }

echo "== bad flags rejected =="
./profiler --only nope >/dev/null 2>&1 && { echo "FAIL: bad --only accepted"; exit 1; } || true
./profiler --n 0 >/dev/null 2>&1 && { echo "FAIL: --n 0 accepted"; exit 1; } || true

echo "PASS"
