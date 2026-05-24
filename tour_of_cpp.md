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
* **Dynamic Binding:** The CPU does a chain of pointer dereferences to find the correct instruction address.

#### Vtable and Vptr
* **Vtable:** Compiler generates a static array of raw function pointers for that specific class. The array is constructed at compile time and baked into read-only data (`.rodata`). One `vtable` per class.
* **Vptr:** The pointer is injected.

#### Calling a virtual function through a base pointer:
1. Dereference the object. Read the 8-byte `vptr`.
2. Index the `vtable`.
3. Read the actual memory address of the target function.
4. Indirect call -> CPU jumps to the resolved address, passing `basePtr` as the hidden `this` pointer.

The pointer jumps cause a performance penalty due to the extra memory loads and branch prediction misses in the CPU pipeline.

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