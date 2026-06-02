# Tour of C++ Notes

## Chapter 1

### 1) The Compilation Pipeline
* **a) Preprocessing:** Preprocessor expands macros (`#define`) and contents of header files are copied into the source file. This generates a massive Translation Unit (TU).
* **b) Compilation (parsing to assembly):** TU is parsed into an abstract syntax tree. Type checking is done. Generates an intermediate representation (IR). Optimization passes are run on IR. Then you emit target-specific assembly code.
* **c) Assembly:** Assembler converts text assembly into machine code. This is the `.o` file. Memory addresses for external functions are still left as symbols.
* **d) Linking:** Linker resolves the symbols. Object files and standard library are matched and placeholders are filled with memory addresses; everything is merged to produce the final executable binary.

---

### 2) Function Overloading and Ambiguity
`print(int, double)` vs `print(double, int)` called with `print(0, 0)` throws an error. C++ does overloading through a mechanism called **Name Mangling**. The compiler turns a function signature into unique strings. When calling the function, the compiler performs overload resolution and checks what's possible.

---

### 3) Initialization and Narrowing Conversion
Assignment initialization (`=`) vs uniform/brace initialization (`{}`).

In the past, `MyClass obj()` did not instantiate an object. It declared a function named `obj` that returns `MyClass`. With `MyClass obj{};` now you know you are making an object.

* `int Hello{7.2}` catches a compile-time error.
* `int hello = 7.2` does silent truncation.

---

### 4) Auto Keyword
`auto` is compile-time deduction. After compilation, there is no overhead. Note that `auto` strips away references and const qualifiers by default.

---

### 5) Const vs Constexpr
* **`const`:** Promise of correctness. Code cannot modify this but the value might only be known at runtime. Allocated on the stack or heap like any normal variable.
* **`constexpr`:** Strict requirement for compile-time evaluation. Needs to have a value that the compiler can compute during translation. This value is embedded directly into `.rodata` or inlined. Computation shifts from runtime -> compile-time.

---

### 6) Best Practices in Loops
Make sure you use `for (const auto& x : container)` to bind by reference and avoid copying unless you need to mutate the copy.

---

## Chapter 2

Structs use constructors to guarantee an object is initializable. At the compiler level, `struct` and `class` are the same thing. The generated Abstract Syntax Tree (AST) for both are identical. The differences are as follows:

* **a) Default Visibility:** Members of a `struct` are `public` by default. Members of a `class` are `private`.
* **b) Default Inheritance:** A `struct` inherits publicly by default. A `class` inherits privately by default.

#### Data Layout
Defining a `class` or `struct` with an `int` size, the compiler determines the memory layout. It adds padding to satisfy hardware alignment. Putting a 4-byte `int` before an 8-byte `int*`, the compiler will inject a 4-byte dead space to align the pointer.

#### Methods
Functions do not take up space inside an object instance. They are translated to standalone functions that take an invisible, implicit `this` pointer which holds the memory address of the instance calling it.

#### Virtual Dispatch
Adding virtual functions, the compiler injects a `vptr` into the object memory. This points to the `vtable` used to resolve function calls dynamically at runtime. This increases the memory footprint by 8 bytes and introduces pointer chasing.

---

### What are virtual functions?
Virtual functions are used to enable runtime polymorphism. You can pass a base class type into a system and have it execute the overridden implementation of the derived class.

* **Static Binding:** Looks at the static type of the pointer, calculates the memory offset, and emits a direct hard-coded `CALL` instruction.
```cpp
class CPUKernel {
    public:
        void execute() {/* ... */}
};

int main() {
    CPUKernel kernel;
    kernel.execute(); // hardcoded static jump
}
```

* **Dynamic Binding:** The CPU does a chain of pointer dereferences to find the correct instruction address. At compile time, compiler only knows base but object in memory is derived. Must use vtable indirection at runtime. 

```cpp
class Kernel {
    public:
        virtual void execute() {/* ... */}
};

class GPUKernel : public Kernel {
    public:
        void execute() override {/* ... */}
};

void run(Kernel* k) {
    k->execute(); // need to read vptr, offsets into vtable, read address, then indirect call
}

int main() {
    GPUKernel gpuK;
    run(&gpuK);
}

```


#### Vtable and Vptr
* **Vtable:** Compiler generates a static array of raw function pointers for that specific class. The array is constructed at compile time and baked into read-only data (`.rodata`). One `vtable` per class.
* **Vptr:** The pointer is injected.

#### Calling a virtual function through a base pointer:
1. Dereference the object. Read the 8-byte `vptr`.
2. Index the `vtable`.
3. Read the actual memory address of the target function.
4. Indirect call -> CPU jumps to the resolved address, passing `basePtr` as the hidden `this` pointer.

The pointer jumps cause a performance penalty due to the extra memory loads and branch prediction misses in the CPU pipeline.

```cpp


// bad architecture
class BadElementScaler {
    public:
        virtual float scaleElement(float val) = 0;
        virtual ~BadElementScaler() = default;
};

class CPUElementScaler : BadElementScaler {
    float scaleElement(float val) override {
        return val * 2.0; 
    }
}; 

void runBadPipeline(BadElementScaler* scaler, float*matrix, size_t size) {
    for (int i = 0; i < size; i++) {

        // bad because you are doing the lookup size times
        matrix[i] = scaler->scaleElement(matrix[i]);
    }
}


```
Calling virtual function scaler->scaleElement()

1) Memory read 1 - CPU reads object's vptr from memory
2) Memory read 2 - CPU uses vptr to find vtable, add offset of routeMatrix, and read actual function pointer
3) Stall - CPU cannot know target address until reads complete. 
4) Pipeline flush - if branch predictor guesses wrong, must flush. 
```cpp
class MatrixScaler {
public:
    // Pure virtual function: sets up our architectural interface boundary.
    virtual void scaleData(float* matrix, size_t size) = 0;
    virtual ~MatrixScaler() = default;
};
class CPUMatrixScaler : public MatrixScaler {
    public:
        void scaleData(float* matrix, size_t size) override {
            for (size_t i = 0; i < size; ++i) {
                matrix[i] = matrix[i] * 2.0f;
            }
        }
};

void runGoodPipeline(MatrixScaler*scaler, float*matrix, size_t size) {
    scaler->scaleData(matrix, size);
}
```

More on this:

At compilation, you have something like this:

```
[.rodata segment]
vtable for GPUDispatcher: [&GPUDispatcher::routeMatrix]
vtable for CPUDispatcher: [&CPUDispatcher::routeMatrix]

```

When you create an object, it is something like this:
```
[Memory address 0x1000 (GPUDispatcher instance)]
Bytes 0-7: vptr (points straight to vtable for GPUDispatcher)
Bytes 8-15: any other variables
```

---

### c) Unions
Unions tell the compiler to allocate all member variables at the exact same memory address. They are the size of the largest variable inside + maybe padding. If you write to `union.s` and try to read from `union.i`, you invoke undefined behavior.

---

### d) Enums
Two types:

#### a) Unscoped Enums (C-style)
```cpp
enum Color { RED, BLUE };
```
This is not great because they implicitly convert to `int`s, allowing them to be passed into things where it makes no sense, yet they still run as `int`s.

#### b) Scoped Enums (C++)
```cpp
enum class Color : uint8_t { RED, BLUE };
```
You can declare the underlying type and force the compiler to pack it into 1 byte in the case of `uint8_t`.

---

### std::unique_ptr and RAII

The main problem it solves is the stack vs. heap. Allocating on the heap uses the `new` keyword, which is forever until you use `delete`. Naturally, local variables placed on the stack are guaranteed to be destroyed when they go out of scope. RAII wraps the heap-allocated resource inside a stack-allocated class object.

`std::unique_ptr` implements RAII. It is simply a lightweight wrapper around a raw pointer.

```cpp
void processImage() {
    std::unique_ptr<Image> smart_ptr = std::make_unique<Image>();

    if (imageIsCorrupted) {
        return; 
    }

    // no need for delete keyword
}
```

A unique pointer's destructor looks something like this:

```cpp
~unique_ptr() {
    if (internal_raw_pointer != nullptr) {
        delete internal_raw_pointer;
    }
}
```

```cpp

class MatrixBuffer {
    private:
        float* data;
    public:
        MatrixBuffer(size_t size) {
            data = new float[size];
        }

        ~MatrixBuffer() {
            delete[] data;
        }
};

void runPipeline() {
    MatrixBuffer buffer(10000);
} // scope will end and destructor will be called
```

Abstract Types: define an interface without specifying the implementation. Done via virtual functions.

You only write custom RAII class when interfacing with raw resources. Examples include:

1) Operating System handles (File descriptors and sockets) -> opening a network socket on POSIX, you just get an int. FDs are indexes into a private, per-process array maintained by the File Descriptor Table. The table points to kernel's internal file structs. 

2) Hardware and Driver contexts: hipstream_t or cudaStream_t is usually a typedef for a pointer to an opaque struct. The pointer tracks hardware queues. Losing the pointer, GPU driver still thinks the queue is active and reserved for the process. Need to call hipStreamDestroy(stream)

3) Library Mutexes and Synchronization Primitives.
pthread_mutex_t is a raw data structure containing atomic integers used for spinlocks and other stuff. It's raw because the state changes by odifying bits. std::lock_guard does not own memory of the mutex. It owns the state of it. 

Litmus Test:
1) Identity - is it represented as a primitive (int, void* uintptr_t)
2) State - Does using it change state outside of program's stack/heap memory
3) Consequence - if variable vanishes, will a leak occur outside of application heap memory. 


```cpp
class DeviceStream {
    private:
        hipstream_t stream;
    public:
        DeviceStream() {
            hipStreamCreate(&stream); // acquire resource
        }

        ~DeviceStream() {
            hipStreamDestroy(stream);
        }

        hipstream_t get() {return stream;}
};

void runKernel() {
    DeviceStream myStream;

    if (some_error) {
        return; // destructor implicitly called
    }
}
```
---

## Chapter 3. Modularity

There is no module system. The preprocessor literally copies and pastes the entire `.h` file into the `.cpp` file before the compiler sees it. This creates a massive Translation Unit. C++20 introduced modules.

### Namespaces
Namespaces are a compile-time and linking construct. Oftentimes, libraries will have functions or structs with the same names. The compiler enforces uniqueness via **Name Mangling** which renames functions to make them unique.

### Invariants
An invariant is a mathematical or logical condition that must hold true for a system to be in a valid state. For example, for Cannon's algorithm, the invariant is that the grid must be a perfect square. If invalid, you must terminate. These are enforced with asserts.

```cpp
assert(condition);
```

Do not use `try`/`catch` for invariants. You use those for runtime anomalies outside CPU control like a dropped network packet or a failed disk read.

### Static Assertions
Catching errors at runtime is expensive, but doing so at compile time is free. `static_assert` can validate hardware constraints before deployment:

```cpp
static_assert(constexpr_condition, message);
```

This is critical for hardware alignment. For example:

```cpp
static_assert(alignof(MyStruct) == 32, "Struct breaks AVX alignment requirements");
```

### Classes
Concrete Types - its representation is known to the compiler at instantiation. Because the compiler knows the exact memory footprint, it can be allocated on the stack.

Value semantics - when you initialize or assign a concrete object. You are copying the underlying bytes.

Constructors
Default Constructor - constructor invoked w/o arguments. The compiler falls back on this for implicit initializations. 

Containers and Reallocation Penalties
std::vector is concrete - lives on the stack. But it manages a dynamically sized array on the heap. Just like in other languages, when you hit capacity, new memory block, and copy each one over. You can use reserve() if you know how much you need to pre-allocate. 

### Ownership, Copying, Move Semantics
1) Danger of Naked Pointers. If a function allocates a massive array on heap and returns a float*, the compiler won't know what to delete. Solution: return a std::unique_ptr<float[]>. This stack object owns the heap pointer so the destructor will be called out of scope. 

```cpp
float* allocateTensorRaw(size_t size) {
    return new float[size];
}
void processPipelineRaw() {
    float* data = allocateTensorRaw(1024);

    if (data[0] < 0.0f) {
        return; // memory leak here
    }
    delete[] data;
}
```

Do this instead:

```cpp
#include <memory>

std::unique_ptr<float[]> allocateTensorModern(size_t size) {
    return std::make_unique<float[]>(size);
}

void processPipelineModern() {
    std::unique_ptr<float[]> data = allocateTensorModern(1024);
    if (data[0] < 0.0f) {
        return; // this is safe now
    }
}
```
2) Copy Semantics 

With a custom Vector class, doing something like v2 = v1 is a shallow copy and copies the 8-byte pointer. When they go out of scope, destructors will try to delete the same address causing a double free segfault. In this case, you need to write a custom copy constructor that does a deep copy.

3) Move semantics
Although deep copies are safe, they are not performant. If you are returning a 4GB tensor, you don't want to have another buffer. You want to just pass it to the caller. You can make a move constructor. 

```cpp
class TensorBuffer {
    float* data;
    size_t size; 

    public: 
        // this is a move constructor
        TensorBuffer(TensorBuffer&& other) {
            this->data = other.data;
            this->size = other.size;

            other.data = nullptr;
            other.size = 0;
        }
};
```

Implementing the rule of 5: destructor, copy, copy assignment, move, move assignment
```cpp
class Vector {
    private:
        double*elem;
        int size;
    
    public:
        Vector() : elem(nullptr), size(0) {}
        Vector(int s) : elem{new double[s]}, size(s) {}

        // destructor
        ~Vector() {
            delete[] elem;
        }

        // copy constructor
        // Vector() -> I am building new vector from scratch
        // const Vector& other take in a const reference
        Vector(const Vector& other) : elem{new double[other.size]}, size{other.size} {
            for (int i = 0; i < other.size; i++) {
                elem[i] = other.elem[i];
            }
        } 

        // copy assignment. replace state of one with another
        // operator= -> this object already exists. I am replacing internal state. 
        // const Vector& other -> I am taking other as a reference
        // The return type is Vector& 
        Vector& operator= (const Vector& other) {
            if (this == &other) {
                return *this; 
            }

            double* arr = new double[other.size];
            for (int i = 0; i < other.size; ++i) {
                arr[i] = other.elem[i];
            }

            delete[] elem;
            elem = arr;
            size = other.size;

            return *this;
        }

        // move 
        // Vector() -> building a brand new object. Take other as a reference
        Vector(Vector&& other) : elem(other.elem), size(other.size) {
            other.elem = nullptr;
            other.size = 0; 
        }
        
        // Move assignment
        // replacing operator= -> same as above
        // Vector&& other -> You are taking a rvalue so it is dying 
        Vector& operator=(Vector&& other) {
            if (this == &other) {
                return *this; 
            }

            delete[] elem;
            elem = other.elem;
            size = other.size;

            other.elem = nullptr;
            other.size = 0;
            return *this; 
        }
};
```

However normally, always design classes by the rule of zero. Design them so they never manage resources directly. Instead of double*, use std::vector<double> or std::unique_ptr; Those types already have built in rule of 5. 

```cpp
#include <vector>
#include <string>

class Layer_Weights {
    public:
        std::string layer_name;
        std::vector<float> weights;
};

void run() {
    Layer_Weights layer;
    layer.layer_name = "dense_1";
    layer.weights.resize(100);

    // this will work because compiler uses the move constructor from std::vector
    Layer_Weights layer2 = std::move(layer);
}
```
### Functors (Function Objects)
Normal functions cannot hold state. If you have a vector of 1 million floats and want to use std::count_if to count the number of numbers < 42, how would you do this?

A functor overloads the operator() and allows you to construct an object with a state then call the object like a function.

note: operator() literally just means like when you pass in (something here).

and so operator= means you are overriding the =. 
```cpp
template<typename T>
class Less_than {
    const T val; // have this hold the 42

    public:
        Less_than(const T& v) : val(v) {}

        bool operator()(const T& x) const {
            return x < val;
        }
};

void runPipeline(std::vector<int>& data) {
    // create the object
    Less_than<int> check_42(42);

    bool result = check_42(10);
}
```

### Lambdas
```cpp
int threshold = 42; 
auto check_val = [&](int a) {return a < threshold; };
```

[] is the capture list - defines member variables of the hidden class. [&] means capture everything in the current scope by reference. [=] means copy.

() - parameters
{} - the code
The issue in the previous response was that the outer Markdown wrapper used triple backticks (`````), which collided with the C++ code blocks inside it and prematurely closed the formatting, breaking the UI's copy utility.

Here is the exact text wrapped in a 4-backtick master block so the copy button works correctly:

```markdown
### Variadic Templates
Sometimes you don't know how many arguments a function will receive or what hardware software runs on. 

Variadic templates accept an unknown number and type of arguments. 

Type Aliases solve the problem of compiling the same code on different hardware where bytes of types change. 

Sometimes you want to have a for-loop that iterates over different types. This won't work normally because for loops only work over homogeneous data. Variadic templates use recursion to process arguments at compile time:

```cpp
void f() {
    // base case
}

template<typename T, typename... Tail>
void f(T head, Tail... tail) {
    G(head); // process the head value
    f(tail...); // recursively call f with the rest of the tail values
}
```

Therefore, if you run something like:
```cpp
f(1, 2.3, "hello_world");
```

the compiler automatically generates the chain of strictly typed hardcoded functions:

```cpp
void f_generated_1(int head, double tail1, const char* tail2) {
    G(1);
    f_generated2(2.3, "hello_world");
}
```

The compiler then goes on to build all of these functions recursively and finally it links:
```cpp
f_generated_base();
```
to the empty `void f() {}` so the recursion terminates. 

### Type Aliases

Use the `using` keyword. 

```cpp
// this:
std::unordered_map<std::string, std::vector<float*>> mem_pool; 

// vs:
using DeviceBufferMap = std::unordered_map<std::string, std::vector<float*>>;

DeviceBufferMap mem_pool;
``` 

Fun fact: C uses `typedef` but `typedef` can't handle template parameters. `using` does. 

`size_t` is great because it works as an alias to select the optimal byte width for the host system. 

```cpp
unsigned int matrix_bytes = width * height * sizeof(float);

// vs
// this is more robust
size_t matrix_bytes = width * height * sizeof(float);
```

### Containers

#### 1) Sequence Containers
`std::array`
- Size is known at compile time and put on the stack.
- No overhead.
- In assembly, it is literally just a base + offset.
- If too large, will cause stack overflow errors.
- Used for fixed-size lookup tables, storing top N levels, mapping enum states.

`std::vector`
- 3 pointers living on the stack (`begin`, `end`, `capacity_end`) = 24 bytes total. Actual data lives on the heap.
- Almost always need to call `reserve(N)` before filling.

To do something like `vec[2] = 5`:
1. Read the start pointer from the stack.
2. Jump to the address on the heap.
3. Add the offset.
4. Write the value.

The data on the heap is contiguous. 

`push_back()` -> Creates a temporary object on the stack, gives it to the vector, copies it into the heap buffer, and destroys the temporary object.

`emplace_back()` -> Passes raw constructor arguments into the vector and the object is constructed directly inside the heap buffer. Skips the temporary object.

#### 2) Sequence Containers
`std::list` (doubly linked) and `std::forward_list` (singly linked)
- Has 2 or 1 pointers (previous/next) and the payload.
- Every time you traverse, you jump to a random heap address, guaranteeing an L1/L2 cache miss on almost every node. 
- If you need something like this, build an intrusive list, an array-backed node pool, or use `std::vector`. 

Array-Backed Node Pool - Allocate a massive `std::vector` of nodes and use 32-bit ints to link. 

```cpp
struct PoolNode {
    double payload;
    int32_t next_idx;
    int32_t prev_idx;
};

class NodePool {
    std::vector<PoolNode> memory_pool;
    int32_t free_head;
    int32_t active_head;

    // custom logic
};
```

Because all nodes live inside the vector, there are fewer cache misses. `int32_t` instead of 64-bit pointers means more nodes fit in a cache line. 

Intrusive List
Normally, containers wrap data in their own structure which requires memory allocation and pointer indirection. In intrusive lists, the data is the node.

```cpp
struct IntrusiveNode {
    IntrusiveNode* prev;
    IntrusiveNode* next;
};

// entry automatically takes prev and next
struct Entry : public IntrusiveNode {
    double price;
    int size; 
};

class IntrusiveList {
    IntrusiveNode* head;
    IntrusiveNode* tail; 
};
```

#### 3) Tree-Based Containers
`std::map` and `std::set`
- These are implemented with Red-Black Trees. Every node has left, right, parent, color, and payload. 
- O(log n) for search, insert, and delete.
- If you already have a `std::vector`, do not just insert elements one by one. Sort the vector, then pass `begin()` and `end()` into the `std::set`. It will construct in O(N) time:
```cpp
std::vector<int> data = {};
std::sort(data.begin(), data.end());

std::set<int> my_set(data.begin(), data.end());
```

- Usually replace with sorted `std::vector`s using binary search if not written to and read frequently.
```cpp
std::vector<std::pair<int, double>> flat_map;
std::sort(flat_map.begin(), flat_map.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

// binary search
auto it = std::lower_bound(flat_map.begin(), flat_map.end(), target_key, [](const auto& pair, int key) { return pair.first < key; });

// lower_bound is just a binary search.
```

Compared to a tree, there is less memory overhead. Also, the prefetcher is able to get data much more efficiently. 

However, this only works if you don't modify frequently. Needing to add an element in the middle is O(n). In those cases, use a flat map implementation instead.

#### 4) Hash Tables
`std::unordered_map` & `std::unordered_set`
- An array of buckets. Each bucket has pointers to singly linked lists.
- Use `reserve()` or else the container will guess a starting capacity. As you insert, it may rehash, which is performance-heavy. 
- Separate chaining causes a cache miss on bucket lookup and another cache miss for the linked list. Usually, open-addressing hashmaps like `absl::flat_hash_map` are preferred instead.

#### 5) Container Adapters
`std::queue` and `std::stack`
- Wraps a `std::deque` by default. 
- Works well.

`std::priority_queue`
- Wraps a `std::vector` and maintains a max-heap structure using `std::make_heap`, `std::push_heap`, and `std::pop_heap`.
- Used for event scheduling where you always need the highest/lowest priority elements.


### std:: Algorithms

#### 1) Numeric Reduction Algorithms

##### std::accumulate

Used for serial folding/summation. Evaluates left to right.

```cpp
#include <numeric>
#include <vector>

std::vector<double> prices = {10.5, 11.2, 10.0};
double sum = std::accumulate(prices.begin(), prices.end(), 0.0); // use float so no arithmetic truncation. Else, it will resolve to an int at every step.
```

Under the hood, it is a simple for loop. 

The sum is deterministic but not associative due to rounding errors.

##### std::reduce

Out of order folding. Designed for multithreading and vectorization via execution policies. 

```cpp
#include <numeric>
#include <vector>
#include <execution>

std::vector<double> numbers = {1.5, -0.2, 3.4, -1.1};

double total = std::reduce(std::execution::par_unseq, numbers.begin(), numbers.end(), 0.0); // execution policy is std::execution::par_unseq in this case
``` 

Reduce operations must be commutative and associative. 

Floating point addition is commutative -> for any 2 numbers, a + b = b + a. However, it is not associative -> after grouping, there is a rounding step on intermediate results that may/may not be thrown out if numbers were in a different order. 

###### std::execution
Namespace that contains execution policy tasks. Used in algorithms like `std::reduce`, `std::for_each`, `std::transform`. It tells the compiler structural guarantees of lambda/functor which allows optimization like concurrency and vectorization. 

1) `std::execution::seq` - strict sequential operations. Use this when operations depend on order or you don't need multithreading. 

2) `std::execution::par` - par is parallel. Used across multiple threads and each thread may grab chunks out of order. Within each thread, operations are done sequentially. In the cases of shared state, must use mutexes or atomics. 

3) `std::execution::unseq` - executes on a single thread but allows vectorization. Operations may be interleaved on the instruction level. Cannot use any synchronizations because if a lane waits, the entire core is deadlocked.

4) `std::execution::par_unseq` - allows multithreading and vectorization. No data races and no locks, mutexes, or blocking calls. 

In cases of multithreading: `std::execution::par` and `std::execution::par_unseq`.

These libraries do not call `std::thread` because spawning OS threads is too slow. They rely on optimized thread pools and work-stealing schedulers. GCC links with Intel TBB (Thread Building Blocks) or OpenMP.

They also do work-stealing:
1) Recursive divide into an optimal chunk size.
2) Queued -> chunks are put into a lock-free queue for each cpu core.
3) If core i finishes its queue, steal work from the bottom of another core.
4) As chunks finish, intermediate results are joined via tree reduction until the total is finished.

In cases of vectorization: `std::execution::unseq`, `std::execution::par_unseq`.

Compiler auto-vectorizer generates SIMD instructions. Instead of something like:

```asm
ADD x[0], y[0]
ADD x[1], y[1]
```

Compiler does:
```asm
VAPPD zmm0, zmm1, zmm2 // which adds 8 doubles in a single clock cycle
```

For vectorization, you need to promise that loop iterations do not depend on each other. The `unseq` tag is that promise. 


Also, using `std::execution::par_unseq` with `std::for_each` and multiple threads writing to nearby atomic variables will invalidate each other's L1 cache lines (false sharing).

What does this actually mean:
When CPU reads/writes from RAM, you fetch cache blocks from RAM to L1 cache. Through MESI, if 2 cores read the same block, it is in a SHARED state. If a core writes, it broadcasts the invalidate to all other cores. All other cores must cache miss on the next fetch and wait for the writing core to flush back to L3 shared cache or RAM. 

False Sharing - multiple threads operating on different cores modify independent variables that are on the same cache line. 

Solution: memory alignment. Force variables into separate cache lines. 

```cpp
#include <new>
struct alignas(std::hardware_destructive_interference_size) PaddedCounter {
    std::atomic<int> value;
};

// using the PaddedCounter struct allows for every value to live on a different block
std::vector<PaddedCounter> counters(4);
```

##### std::inner_product

It is the dot product of 2 ranges. This has been replaced by `transform_reduce`.

```cpp
#include <numeric>
#include <vector>

std::vector<double> v1 = {1.0, 2.0, 3.0};
std::vector<double> v2 = {4.0, 5.0, 6.0};

double dot = std::inner_product(v1.begin(), v1.end(), v2.begin(), 0.0);
```

Under the hood, it looks like this:

```cpp
template<class InputIt1, class InputIt2, class T>
T inner_product(InputIt1 first1, InputIt1 last1, InputIt2 first2, T value) {
    while (first1 != last1) {
        value = std::move(value) + (*first1) * (*first2);
        ++first1;
        ++first2;
    }

    return value; 
}
```
The problem is that it has an execution order left to right which blocks parallelization from the loop itself.

##### std::transform_reduce
Fuses a transformation (map) and reduction into a single pass. 

```cpp
#include <numeric>
#include <vector>
#include <execution>

std::vector<double> weights = {0.5, 0.5};
std::vector<double> returns = {0.02, 0.04};

double port_return = std::transform_reduce(
    std::execution::par_unseq,
    weights.begin(), weights.end(),
    returns.begin(),
    0.0
);
```

Instead of iterating through an array, mutating, storing into RAM, and then summing, `transform_reduce` loads the element and immediately adds to the local accumulator.

Allows for Cache locality + bandwidth -> avoids intermediate allocations and round trips. Prevents L1/L2 cache evictions. It is the replacement for `inner_product()`.

Replacing inner product:
1) Transform (multiplication). Pairs 2 iterators and does the transformation. a0 * b0, a1 * b1, etc. This is parallelized. 
2) Reduce. Do an out of order tree reduction similar to `std::reduce`. 

##### std::inclusive_scan and std::exclusive_scan
Parallel prefix sums.

```cpp
#include <numeric>
#include <vector>
#include <execution>

std::vector<int> numbers = {10, 20, 30};
std::vector<int> running_total(3);

// outputs {10, 30, 60}
std::inclusive_scan(std::execution::par_unseq, numbers.begin(), numbers.end(), running_total.begin()); 

std::exclusive_scan(std::execution::par_unseq, numbers.begin(), numbers.end(), running_total.begin());
```

If you don't know this you don't deserve the degree. 

#### Sorting and Selection Algorithms
##### std::sort

General purpose sort.

```cpp 
#include <algorithm>
#include <vector>

std::vector<double> latencies = {14.2, 11.1, 19.5, 12.0};

std::sort(latencies.begin(), latencies.end());
```

Implemented via introsort. 
1) Begins at quicksort.
2) If the recursion depth exceeds 2 * log(n), it assumes quicksort will degrade towards O(n^2) and switches to heapsort.
3) For small partitions (16 elements), it doesn't do recursion and does insertion sort because it has less overhead and it's only 16 elements. 

This is done in place. 

If you provide a custom comparator, it must return false for equal elements otherwise it will read out of bounds on seg faults. 

This is because inside quicksort, the partition looks like this:

```cpp
while (comp(*ptr, pivot)) { ++ptr; }
```
If `comp` returns true for equal values and the whole array is the same element, pointer `ptr` will go through the memory and walk into unmapped memory pages causing seg faults. 

##### std::stable_sort
Maintains relative order of elements that compare as equivalent.

```cpp
#include <algorithm>
#include <vector>

struct item {
    double price;
    int time_ns;
};

// sort by the price and maintains chronological order
std::vector<item> vec = {{32, 1}, {100, 3}, {100, 2}};
std::stable_sort(vec.begin(), vec.end(), [](const item& a, const item& b) { return a.price < b.price; });
```

Implemented as adaptive merge sort. If system can allocate memory, it runs NlogN. If memory allocation fails, falls back to in-place merge sort O(Nlog^2N). That malloc is punishing. 

##### std::partial_sort
Sorts only the top k elements and leaves the remaining N - K elements in an unspecified state. 

```cpp
#include <algorithm>
#include <vector>

std::vector<double> volumes = {500, 100, 900, 300};

// only sort the first 2 items but look at all the elements till end
std::partial_sort(volumes.begin(), volumes.begin() + 2, volumes.end());
```

Key ideas:
Sometimes `std::sort()` is better than `std::partial_sort` because heap operations jump around memory causing cache misses. Partial sort hurts when K is relatively large. 

Better than just heapify + pop(k). Heapify() is O(N). Popping from the heap is O(K). Thus, O(n + klog(n)).

In the standard implementation:
1) Build heap on the k elements: O(K)
2) Linear scan. Go through the remaining elements and look for smaller one.
2.5) If element is smaller, swap with root and reheapify. O((N - K)log(K)). 
3) Run `std::sort_heap()` on k elements which takes O(KlogK).

Total complexity: O(K + (N - K)log(K))

At the memory level: When you heapify with size N, it's much larger and if it spans cache blocks, it will miss. With K, it will be smaller so fewer cache misses. 

Also, in terms of branch prediction, heapify(N) is K heap pops which will always cause pointer jumps. When you iterate through N-K elements, many of those evaluate to false and if branch predictor starts to assume false, it predicts correctly more often. 


##### std::nth_element
Finds the N-th element and partitions the array around it.

```cpp
#include <algorithm>
#include <vector>

std::vector<double> returns = {0.05, -0.01, 0.02, 0.1};
auto median_it = returns.begin() + returns.size() / 2;

// find the median
std::nth_element(returns.begin(), median_it, returns.end());
```

Under the hood, this is implemented as Introselect (which is a hybrid of Quickselect and Median of Medians or Heapselect). 

1) Partitions array around a pivot.
2) Recurse only into the half that contains the Nth iterator (NlogN -> O(N)).

This is the optimal way to find nth percentile value such as a median. If you use `std::sort`, it is NlogN vs O(N) answer. 

However, while Nth element is perfect, the elements before/after are not sorted. 

##### std::is_sorted / std::is_sorted_until
Linear check to determine if array is already sorted. Very friendly for the branch predictor because most likely true in cases of network packets where it SHOULD be sorted. 

```cpp
#include <algorithm>
#include <vector>

std::vector<int> stream = {1, 0, 2, 3};
bool sorted = std::is_sorted(stream.begin(), stream.end()); // false
auto unsorted_it = std::is_sorted_until(stream.begin(), stream.end()); // returns 1
```

Will come back to these
#### Binary Search Algorithms


All built on top of binary search. 

Goal: standard library implementation of binary search to find the first iterator that is `>= value`. 

This is only done because it must support all forward iterators like linkedlists. 
```cpp
while (count > 0) {
    step = count / 2; // or step = count >> 1. get midpoint

    // get to the middle
    it = first; 
    std::advance(it, step); // if this is a vector, jump. if ll, walk. 

    if (*it < value) { // if this is 50% branch predictor will struggle
        first = ++it; // discard the half
        count -= step + 1; // shrink the number of elements by skipped
    } else {
        count = step; // it is on the left. shrink the window so it goes up to current iterator (left half)
    }
}
```
##### std::lower_bound
Returns iterator to first element that is `>= value`. 

##### std::upper_bound
Returns iterator to the first element that is `> value`.
Implementation is exactly the same except in the boolean condition.

`if (value < *it)`

Strict weak ordering - for any relation that represents `a < b`:

1) `f(a, a)` must equal `false`. This means it cannot be `==` or `<=`. 

If this is false, it guarantees that `*it` is `<=` to the target value. This means that this element and everything to the left is invalid. Upper bound must be to the right. 
##### std::equal_range
Returns the range of iterators of all elements equal to value. Literally does `{std::lower_bound(), std::upper_bound()}`.

This is further optimized where when you find the element equal to value, run `std::lower_bound` on left sub-segment and `std::upper_bound` on right sub-segment. 

##### std::binary_search
Return a bool indicating whether an element equivalent to value exists. 


#### Partitioning Algorithms
Used for separating data without being Nlog(N).

##### std::partition
Rearranges elements so all elements where predicate returns true precede all elements for which it returns false. Does not preserve relative order. 

Implemented via a bidirectional linear scan or in cases with only forward iterators, fast/slow pointers. 

1) 2 pointers, one in beginning and one at end. 
2) left pointer advances forward until it hits element where predicate is false
3) right pointer advances backward until it hits true
4) swap first and last elements
5) repeat until the 2 pointers cross each other

Runs in O(N) time and at most, N/2 swaps. Cache friendly because of the linear access pattern.

```cpp
template <class BidirectionalIterator, class Predicate>
BidirectionalIterator partition_bidirectional(BidirectionalIterator first, BidirectionalIterator last, Predicate pred) {
    while (true) {
        // advance left pointer until failure
        while (true) {
            if (first == last) {
                return first; // everything is true
            }

            if (!pred(*first)) {
                break;
            }

            ++first;
        }

        // the do-while loop is very slightly faster because the right iterator may be out of bounds. so you start by decrementing last with do while and continue while it's true. 
        do {
            if (first == --last) {
                return first;
            }
        } while (!pred(*last));

        std::iter_swap(first, last);

        ++first; // increment first because after swap, is definitely correct. last is decremented in the do while loop. 
    }
}
```

Using it to partition odd and even numbers. 
```cpp
#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> data = {1, 7, 4, 9, 2, 8, 3, 6, 5};

    auto partition_point = std::partition(data.begin(), data.end(), [](int x){
        return x % 2 == 0;
    });

    // from idx 0 -> partition point - 1, even numbers
    // partition point -> end, odd numbers
}
```
##### std::stable_partition
Same as `std::partition`, but guarantees relative order. The implementation changes depending on the memory state.

1) Sufficient memory - `std::get_temporary_buffer()` to get a buffer equal to the size of the array. Then start a linear scan. If iterator is true, write directly into left side of original array. If false, write to temporary buffer. Then copy the buffer to the original array and free the buffer.

```cpp
template <class ForwardIt, class Predicate>
ForwardIt stable_partition_buffer(ForwardIt first, ForwardIt last, Predicate pred, auto buffer_start) {
    auto read_ptr = first;
    auto write_true = first;
    auto write_false = buffer_start; 

    for (; read_ptr != last; ++read_ptr) {
        if (pred(*read_ptr)) {
            // write straight into the original memory only if the pointers are not equal.
            // if the pointers are equal, all true so far so leave as is. 
            if (read_ptr != write_true) {
                *write_true = std::move(*read_ptr);
            }

            ++write_true;
        } else {
            // write into the buffer
            *write_false = std::move(*read_ptr);
            ++write_false; 
        }
    }

    // move the false elements back into the original array
    std::move(buffer_start, write_false, write_true);

    return write_true; // return the partition point
}


```
Note: when you use iterators, you are typically dereferencing and incrementing the iterator due to flexibility. You don't do something like `arr[i]` because that requires `operator[]` random access which something like a linkedlist doesn't have. Iterators are just glorified pointers. 

```cpp 
// typically you do this
*it = data;
++it;

// or 
*it++ = data;
```

2) Insufficient memory - in-place divide and conquer. Chop array into smaller chunks, partition them, then use `std::rotate` to swap true and false elements back and forth until merged. Rotate is also really bad on CPU cache as it requires swapping memory blocks. 

I don't feel like doing the implementation for this. 

Notes:

Because of the `malloc()` in cases of sufficient memory, it *may* cause a global heap lock. Other cases, threads have thread-local caches (TLC) so the heap allocation is typically in the thread local arena. The lock only happens if there is heavy memory pressure or embedded system. 
##### std::partition_point
Returns the iterator to the exact boundary line between the true elements and the false elements. The array MUST be already partitioned. This is literally just a binary search done with `std::lower_bound`. 

As stated before, this causes cache misses for large arrays due to large jumps across memory. 


#### Heap Algorithms
Heaps are not based on nodes or pointers. They are implicit data structures mapped onto contiguous memory. 

Any element index `i`:
1) `left = 2i + 1`
2) `right = 2i + 2`
3) `parent = (i - 1)/2`

By default, heaps are max heaps using `operator <`.

**Key Point:** Not using `std::swap`.

`std::swap` requires a temporary register with 3 memory writes:
```cpp
tmp = std::move(A);
A = std::move(B);
B = std::move(tmp);
```

**Solution:** "Hole" optimization. If you are going to move the element multiple times, don't keep putting the item back in the list until you know where it logically should go.

```cpp
auto val = std::move(array[0]);        // 1) array[0] is now a "hole".
array[hole] = std::move(array[child]); // 2) Every sift, you are pulling into this hole. Every element move is a write into the hole.
array[final_hole] = std::move(val);    // 3) When the hole is in the right place, write the value back into the hole.
```

Given something like `[A, B, C, D]`:

The hole can first hold `A`, then hold `B`, then `C`. Then perhaps it stops at `C` and you replace `C` with something else.

* `std::swap` writes = `3k` (where `k` is a memory step in the heap).
* Using hole = `k + 2` writes.
##### std::make_heap
Converts an unsorted range into a valid max-heap. 

Implemented via Floyd's algorithm. 
1) Calculates index of the last non-leaf node: `(N / 2) - 1`. Elements after this index are leaves and single leaves are valid heaps. 
2) Iterate backward from index to 0.
3) from node, do a `down_heap()`. The optimized version is called `sift_down()`;

```cpp

/*
Compare left and right child to find the larger one. Essentially swap that child ptr with the hole. Do this while hole hits the very bottom leaf later.
*/

// note: holes are just an empty slot in the array for holding variables. 
// RandomIt is random access iterator
// ptrdiff_t stores the difference between 2 pointers/iterators
template<class RandomIt>
void sift_down(RandomIt first, ptrdiff_t hole, ptrdiff_t len, auto value) {
    ptrdiff_t top = hole;
    ptrdiff_t child = 2 * hole + 1; 

    while (child < len) {
        
        // first points to beginning of sequence. child is the integer offset. to get data, do first + offset. 
        // child + 1 < len -> checks if the child exist (len is the boundary of real vs garbage data)
        // *(first + child) < *(first + (child + 1)) -> if right value is > left value
        if (child + 1 < len && *(first + child) < *(first + (child + 1))) {
            ++child; // point to the bigger and in this case right child
        }

        // move larger child up
        *(first + hole) = std::move(*(first + child));

        // the hole becomes the old child as the child and hole basically swap
        hole = child;
        child = 2*hole + 1; 
    }

    sift_up(first, hole, top, std::move(value));
}

/*
sift_up is basically push_heap. Take a value at the bottom of the heap and bubble it upward to its correct resting place. 
*/
template <class RandomIt>
void sift_up(RandomIt first, ptrdiff_t hole, ptrdiff_t top, auto value) {
    while (hole > top) {
        ptrdiff_t parent = (hole - 1) / 2; 
        if (*(first + parent) < value) {
            *(first + hole) = std::move(*(first + parent));
            hole = parent;
        } else {
            break; 
        }
    }

    *(first + hole) = std::move(value); // move the value back into the hole. 
}

template <class RandomIt>
void make_heap(RandomIt first, RandomIt last) {
    ptrdiff_t len = last - first; // total number of elements in the array
    if (len < 2) {
        return;
    }

    // get last non-leaf node
    ptrdiff_t parent = (len - 2) / 2;

    // take out the parent, sift_down to its correct place. (aka downheap)
    while (true) {
        auto val = std::move(*(first + parent));
        sift_down(first, parent, len, std::move(val));
        if (parent == 0) {
            break;
        }
        --parent; // process the next value.
    }
}

```
##### std::push_heap
Already a valid heap. the item to push up is already there sitting at last - 1. It's typically placed there via `std::vector::push_back`.

```cpp
template <class RandomIt>
void push_heap(RandomIt first, RandomIt last) {
    ptrdiff_t hole = (last - first) - 1; // new added element in the end
    ptrdiff_t parent = (hole - 1) / 2;
    auto val = std::move(*(first + hole)); // take the placed value and store into this temp variable. this means that *(first + hole) data can be manipulated. 

    // push that new value up to where it should be
    while (hole > 0 && *(first + parent) < val) {
        *(first + hole) = std::move(*(first + parent)); // data from parent is put into the hole
        hole = parent; // space where the parent is is now empty. it's the new hole
        parent = (hole - 1) / 2; // new parent for next iteration
    }
    *(first + hole) = std::move(val); // drop the value into the hole
}
```

##### std::pop_heap
The max element (normally at the front) swaps places with the last element. Then you sift down that element.

```cpp
template <class RandomIt>
void pop_heap(RandomIt first, RandomIt last) {
    if (last - first > 1) {
        --last;

        // Without std::move, you would call the copy constructor.
        // If each item was a string, it would require malloc() and heap copying. Move just steals the pointers because it tells them that you will fill in *last anyway.
        auto bottom_val = std::move(*last);

        *last = std::move(*first);
        
        ptrdiff_t len = last - first;

        // Sift down the hole and then drop bottom val in.
        sift_down(first, 0, len, std::move(bottom_val));
    }
}
```
##### std::sort_heap
Converts a max-heap into a fully sorted array in ascending order.

You literally just loop `std::pop_heap` because `std::pop_heap` takes the largest element and puts it at the end of the array. Runs in O(N log N).
```cpp
template <class RandomIt>
void sort_heap(RandomIt first, RandomIt last) {
    while (last - first > 1) {
        --last;
        std::pop_heap(first, last + 1);
    }
}
```

#### Sequence Modification
Focus on the difference between memory-bound operations (eager evaluation) and CPU-bound operations (lazy evaluation). 

##### std::transform
Applies a function to every element in a range and writes to a destination range (ranges can be the same so it can be in-place). This is typically implemented as a linear loop.
```cpp
template <class InputIt, class OutputIt, class UnaryOperation>
OutputIt transform(InputIt first1, InputIt last1, OutputIt d_first, UnaryOperation unary_op) {
    while (first1 != last1) {
        *d_first++ = unary_op(*first1++);
    }
    return d_first; 
}
```

Using `std::transform`:
```cpp
#include <algorithm>
#include <vector>
#include <cmath>

std::vector<double> prices = {100.0, 105.0, 102.0};
std::vector<double> log_prices(prices.size()); // make sure it is pre allocated

std::transform(prices.begin(), prices.end(), log_prices.begin(), [](double p) {return std::log(p);});
```

Great for compiler auto-vectorization. In simple cases of unary_op, the loop can be unrolled and use AVX registers to process 8 or 16 elements per clock cycle. 

Quick refresher on loop unrolling:
```cpp
for (int i = 0; i < 1000; ++i) {
    sum += a[i] * b[i];
}
```

At the basic level:
1) load `a[i]`, `b[i]`, multiply, add to `sum`
2) increment `i`
3) compare `i` to 1000
4) if true, back to the top of the loop

Steps 2, 3, and 4 are loop overhead. If they run over and over, half of the clock cycles are spent managing the loop counter rather than the actual computation. To lower this, the compiler can do something like this to be more efficient:

```cpp
int i = 0;

for (; i <= 1000 - 4; i += 4) {
    sum += a[i] + b[i];
    sum += a[i + 1] + b[i + 1];
    sum += a[i + 2] + b[i + 2];
    sum += a[i + 3] + b[i + 3];
}

// handle remainders
for (; i < 1000; ++i) {
    sum += a[i] * b[i];
}
```
Thus, the number of branches goes from 1K to 250. For instruction-level parallelism, out-of-order execution means that multiple ALUs can function independently. 
**Trap:** If `d_first` overlaps with `first1` in any way that isn't in-place, the SIMD write might overwrite data, so the function silently won't vectorize. 

##### std::views::transform
The modern way of std::transform. Instead of eagerly computing results and writing, create a lazy view. 

a) It does not allocate memory.
b) It does not iterate over the created array.

Instead, construct an iterator wrapper with a pointer to underlying data and copy of the lambda. Transformation only happens when the iterator is dereferenced. 

```cpp
// kinda what the assembly looks like
decltype(auto) operator*() const {
    return unary_op(*underlying_iterator);
}
```

In other words. It only becomes the transform after you iterate over it. 

Example of use:
```cpp
#include <ranges>
#include <vector>
#include <iostream>

std::vector<double> prices = {100.0, 105.0, 102.0};

auto lazy_prices_in_log = prices | std::views::transform([](double p) {return std::log(p);});

// every read is the log version of the price
for (auto log_p : lazy_prices_in_log) {
    /* ... */
}
```

When is `std::views::transform` better compared to `std::transform`?
1) Single passes over memory-bound operations -> if you are iterating over the data once and the array is giant, `std::transform` will cause memory problems with buffer allocations and cause cache misses. Lazy views keeps all data inside the L1 cache.
2) Early exits -> finding the first transformed element that meets a condition, you don't need to precompute and save the rest of the elements. 
3) Chaining operations:
```cpp
data | std::views::filter | std::views::transform | std::views::take(10)
```

When is `std::transform` better than `std::views::transform`?
1) Multiple passes -> you don't want to run the computation multiple times.
2) If you have older libraries that can't use C++20.
3) Guaranteed SIMD Vectorization.


##### std::remove/std::remove_if
Removes the elements that match a value or predicate. Does not change the size of the container. 

2 pointer approach. 
1) read and write ptr
2) u know the rest
```cpp
template<class ForwardIt, class UnaryPredicate> 
// first is the read_ptr
ForwardIt remove_if(ForwardIt first, ForwardIt last, UnaryPredicate p) {
    ForwardIt write_ptr = first; 
    for (; first != last; ++first) {
        if (!p(*first)) { // if it should be removed
            // only write in if they are not the same anymore
            if (first != write_ptr) {
                *write_ptr = std::move(*first);
            }
            ++write_ptr; 
        }
    }
    return write_ptr; 
}
```

Because you return the iterator that is the number of good items + 1, you can easily combine this with `std::erase()` to reclaim memory.
```cpp
// C++17 and older
vec.erase(std::remove_if(vec.begin(), vec.end(), is_expired), vec.end());

// C++20 and newer
std::erase_if(vec, is_expired);
```
##### std::generate/std::generate_n
A generator is a function or lambda that takes no arguments and returns a value. 

```cpp
[]() {return 5;}
[]() {return get_system_time();}
```

std::generate calls the 0 argument function over and over, taking the return value, and putting it into an array.

Suppose you have an std::vector with 5 slots you want to fill
```cpp

// old C way
std::vector<int> vec(5);
for (size_t i = 0; i < vec.size(); ++i) {
    vec[i] = 42; 
}

// modern way
std::vector<int> vec(5);
std::generate(vec.begin(), vec.end(), [](){return 42;});
```

Now turn to stateful generators - when the generator remembers what it did last time. You can generate an ascending sequence with a lambda by providing a reference or using the mutable keyword. 

```cpp
std::vector<int> vec(5);
int counter = 10; 

std::generate(vec.begin(), vec.end(), [&counter]() {
    int current = counter;
    counter++;
    return current; 
});
```

`std::generate_n` fills exactly `n` slots. Calling `std::generate` on an empty array does nothing because it requires a `vec.begin()` and a `vec.end()`. 


```cpp
std::vector<int> vec(10, 0); // 10 slots with value = 0 
int counter = 1;

std::generate_n(vec.begin(), 3, [&counter]() {
    return counter++;
}); // vec is now [1, 2, 3, 0, ...]

// you can also grow the vector by combining std::generate_n with std::back_inserter

std::vector<int> empty_vec;
counter = 100;

std::generate_n(std::back_inserter(empty_vec), 4, [&counter]() {
    return counter++;
}); // empty_vec is now [100, 101, 102, 103]
```
##### std::replace/std::replace_if
Scans a sequence and replaces elements that match the value with a new value. 

```cpp
template<class ForwardIt, class UnaryPredicate, class T>
void replace_if(ForwardIt first, ForwardIt last, UnaryPredicate p, const T& new_value) {
    for (; first != last; ++first) {
        if (p(*first)) {
            *first = new_value;
        }
    }
}
```

If needed, sometimes you can just write a branchless loop:

```cpp
for (auto& val : data) {
    bool mask = p(val);
    val = mask * new_value + (!mask) * val;
}
```
#### Non-Modifying Sequence Operations
##### std::find, std::find_if, std::find_if_not
##### std::anyof, std::all_of, std:: none_of
##### std::mismatch

#### Permutation and Merging
##### std::next_permutation
##### std::merge
##### std::inplace_merge

### std Library Utilities
#### Smart Pointers and RAII (again)

Manual memory management is dangerous.

```cpp
void f(int i, int j) {
    X* p = new X;
    std::unique_ptr<X> sp {new X};

    // in both of these cases, p leaks because it is a raw pointer
    if (i < 99) {
        throw Z{};
    }
    if (j < 77) {
        return;
    }

    p->function1();
    p->function2();

    delete p;
}
```

`sp` is a `std::unique_ptr` on the stack and has its own `delete` call. It will not leak. `std::unique_ptr` is 0 cost because the compiler optimizes it with no memory overhead. It is literally just putting the pointer in a struct and the struct knows to delete itself.

`std::shared_ptr` is different. `std::shared_ptr` is used when an object has non-deterministic, shared lifetime requirements. Use it when multiple parts of the system hold onto the object. For example, in a game engine, the audio thread, physics thread, and render thread hold a pointer to the player object. If rendering thread deletes the player, the physics thread will seg fault when it calculates collisions. With `std::shared_ptr`, the last thread to drop silently executes destruction. 

It requires a dynamically allocated control block with the object. This block has:

a) strong reference count - number of `std::shared_ptr` instances that own the data. As long as this is > 0, the managed object is kept in memory. 
b) weak reference count - number of `std::weak_ptr` instances observing the data. These do not own the data. If this is > 0, it will not prevent object from being destroyed. To use the object, must convert to a `std::shared_ptr` using a lock. 
Note: you have weak pointers in cases where A holds a `std::shared_ptr` to B and B holds one for A, strong counts are 1. If a program drops all counters, they keep each other alive which is a memory leak. The fix is to have B hold a `std::weak_ptr` to object A. 

c) custom deleters/allocators

During multithreading, incrementing/decrementing reference counts requires atomic operations. This causes cache line bouncing, memory bus traffic. With shared ownership, use `std::make_shared<T>()` which makes the allocation of object and control block into a contiguous heap block. This improves cache locality and reduces overhead. 

#### Arrays, pointer decay, polymorphic problems

```cpp
void h() {
    Circle a1[10]; 
    std::array<Circle, 10> a2;

    Shape* p1 = a1; // Point 1: this will compile because an array will decay to a pointer to its first element
    Shape* p2 = a2; // fails to compile enforcing type safety. 
    p1[3].draw();
}
```

Problem with Point 1: Happens at `p1[3]`. From compiler POV, pointer arithmetic is determined by type of the pointer. It thinks `p1` is `Shape`. `p1[3]`'s address = Base + (3 x `sizeof(Shape)`). However, in `a1`, they're circles. If `sizeof(Circle) != sizeof(Shape)`, it will corrupt the bounds and cause segfault. 

The solution is to use `std::array`. This is because `std::array<T, N>` does not decay. 

#### Bit Manipulation via std::bitset
`std::bitset<N>` is an abstraction over an array of integers. Ex: tracking 128 boolean states can be done via 
```cpp
bool flags[128]; // This is 128 bytes
std::bitset<128> flags; // 16 bytes
```

Bitsets are more optimized and operations like `.count()` map directly to hardware instructions which execute in a single clock cycle. 

Don't ever do `std::vector<bool>`.

### Grouping Data: std::pair and std::tuple

## Concurrency
### std::thread and .join()
### std::defer_lock and mutexes
### Asynchronous State with std::promise and std::future
### std::packaged_task
### std::async