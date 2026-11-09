# profiler — benchmark hand-rolled C++ structures against the stdlib
#
#   make            build ./profiler and ./linked_list
#   make test       build everything and run the checks (small N, fast)
#   make bench      build and run the default benchmark (N=2M, ~1-2 min)
#   make clean      remove built binaries
CXX ?= g++
# -Wno-interference-size: bst.h deliberately uses
# std::hardware_destructive_interference_size; its value varying with
# -mtune is expected, not a bug.
CXXFLAGS ?= -O3 -std=c++20 -Wall -Wextra -Wno-interference-size -I.

all: profiler linked_list

profiler: main.cpp src/stats.cpp src/stats.h src/alloc.h src/hash_map.h src/bst.h linked_lists.cpp
	$(CXX) $(CXXFLAGS) main.cpp src/stats.cpp -o profiler

linked_list: linked_lists.cpp
	$(CXX) $(CXXFLAGS) linked_lists.cpp -o linked_list

test: all
	./tests/run_tests.sh

bench: profiler
	./profiler

clean:
	rm -f profiler linked_list

.PHONY: all test bench clean
