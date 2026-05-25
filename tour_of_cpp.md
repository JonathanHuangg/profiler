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

