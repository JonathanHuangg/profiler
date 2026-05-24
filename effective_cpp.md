# Effective C++ Notes

// option k then v to view
## 1) Template type deduction

```cpp
template<typename T>
void f(ParamType param);

f(expr);
```

The compiler evaluates based on different cases. `T` is just a parameter placeholder. `ParamType` is the final type of the finished variable and uses `T`.

### a) ParamType is normal -> `T&` or `T*`
From the compiler's POV, the function wants to share the caller's memory. Do not modify read-only memory. `T&` means "do not make copy. direct reference to original variable".

If you pass in `const int cx` into `f(T& param)`, if `T` is deduced as an `int`, the function can use it as a reference to overwrite the memory. So `T` must be `const int`. `ParamType` is thus `const int&`.

If the signature is already `const T&`, you already know you can't edit, so just get the base type.

### b) ParamType is universal reference `T&&`
Depends on whether the argument is an lvalue or rvalue. If `expr` is an lvalue, `T` and `ParamType` are lvalue references. Make it `type&`.

If `expr` is an rvalue, look above.

**Example:**
```cpp
template<typename T> 
void f(T&& param);

int x = 0;
f(x); // x is an lvalue. T and ParamType is int&

const int y = 0;
f(y); // y is an lvalue. T and ParamType is const int&
```

### c) ParamType is passed by value. So no addresses. Just adding to the stack like normal.
```cpp
template<typename T>
void f(T param);
```

When you pass by value, the function receives a copy of the argument from the stack. You can modify it since it is a copy.

If reference, ignore it. If `const` or `volatile`, ignore it.

* `const int& rx` -> `T` is an `int`. `ParamType` is an `int`.

```cpp
const char* const ptr = "hello";
f(ptr);
```

* `const char*` means that the data pointed to characters cannot be changed.
* `* const ptr` means the pointer cannot be pointed to a different address.

For the function, you copy the pointer. It is a copy of the data, so `ptr` can be changed in the function. `const char*` does not change because the data is still protected in memory.

`T` and `ParamType` are thus deduced to be `const char*`.

---

**Example:**

```cpp
template<typename T>

// Example A. param cannot be modified. T is raw type. ParamType is automatically const T&
void caseA_ref(const T& param) {}

// Example B. ParamType is a universal reference for forwarding
void innerTarget(int& lref) {}
void innerTarget(int&& rref) {}

template<typename T>
void caseB_Forwarder(T&& param) {
    innerTarget(std::forward<T>(param));
}

// Example C. Pass by value. Put on stack
template<typename T>
void caseC_Value(T param) {
    param = 99; // modify freely because it is on the stack
}
```

---

## 2) Auto type deduction

Using `auto`, it's the same idea as above where `auto` is `T`. The one exception is with braced initializers.

```cpp
int x1 = 27;
int x2(27);
int x3 = {27};
int x4{27};
```

Everything above is an `int` with value `27`.

```cpp
auto x1 = 27;   // this is an int
auto x2(27);   // this is an int
auto x3 = {27}; // this is std::initializer_list<int>
auto x4{27};    // this is std::initializer_list<int>
```

The compiler first sets it to `std::initializer_list<int>` and then looks inside the braces to deduce `T`. This is an extra hardcoded rule. If you pass in `{1, 2, 3}` into a `f(T param)`, it will lead to a compiler error. For it to work, you have to write `std::initializer_list`.

---

## 3) Decltype - compile time inspector tool

**Example:**
```cpp
const int x = 0;
auto a = x;         // not auto& or auto&&. Neither pointer nor reference. Treated as pass-by-value (stack)
decltype(x) d = x;  // Looks at x -> const int -> 'd' is const int
```

You need this because `auto` is too aggressive at stripping references. Example of the problem:

```cpp
template<typename Container, typename Index>
auto auth_and_access(Container& c, Index i) {
    authenticateUser();
    return c[i];
}
```

Assuming you pass in `std::vector<int> v = {10, 20, 30}`.

Assume `v` starts at `0x100` and integer `10` is at `0x104`. At the `return c[i]` line, assume you are returning `c[0]` -> you directly return `int&`. You should be returning `int&` pointing to address `0x104`.

However, it is an `auto` declaration, so the pass-by-value template is used. It makes the function return an `int` instead of `int&`. It makes a new memory slot for the return value. When the code now runs, compile failure.

Instead of `auto`, you must use `decltype(auto)`.

**Quick note:**
```cpp
int x = 0;
decltype(x)   // this is an int
decltype((x)) // this is an expression that evaluates to lvalue so int&
```

This is dangerous for `decltype(auto)` obviously because it depends on what you are returning.

---

## 4) Deducing Types

### a) Use the compiler to print exact type deductions
#### i) Create incomplete type
```cpp
template<typename T>
class TD;
```
Compiler knows `TD` exists, but does not know how to instantiate.

#### ii) Then pass in something that fails
```cpp
TD<decltype(x)> xType;
```
The compiler sees `decltype(x)`. It is `int`. Now it is `TD<int> xType`. Tries to allocate space. Looks at `TD` to see how much memory but finds nothing. Causes halt and emits error message.

When the compiler complains, it prints out the exact type name it was trying to instantiate.

### b) Runtime
You can try to use `typeid(x).name()`. But it means that a `type_info` object exists and produces a C-style string. However, it can sometimes just be wrong.

```cpp
template<typename T>
void f(const T& param) {
    using std::cout;
    cout << "T = " << typeid(T).name() << '\n';
    cout << "T = " << typeid(param).name() << '\n';
}
```

3 different compilers say that `T = class Widget const *`, `param = class Widget const *`.

But if you think about it, `T` and `param` should not have the same type. If `T` is `int`, `param` should be `const int&`. The bottom line is that `typeid` is not reliable.

This is because `type_info::name` mandates the type to be treated as a pass-by-value parameter. `const` is stripped, references are stripped.

`param` which should be `const Widget * const &` is stripped to `const Widget*`.

---

## 5) Prefer auto to explicit type declarations

If you type something like `int x`, the compiler does not spend CPU cycles zeroing out the memory. Just reserves it. Because `auto` relies on type deduction, the compiler **MUST** see an expression on the right side to figure out the type. You can't read garbage data by accident.

Given something like:
```cpp
template<typename It>
void dwim(It b, It e) {
    while (b != e) {
        typename std::iterator_traits<It>::value_type currValue = *b;
        ...
    }
}
```

This code is an iterator. The problem is that if this is a generic type, it won't always have a `value_type`. If you pass a raw pointer, it becomes `int*`. `int*::value_type` is a syntax error.

`iterator_traits` is a template-routing struct. Hand it a class iterator, extracts the nested `value_type`. Give it raw `int*`, uses template specialization where it strips `*` and spits out `int`. You had to use this which was 45 characters of heavy template metaprogramming:

```cpp
typename std::iterator_traits<It>::value_type currValue = *b;
```

Instead, the `auto` solution is:
```cpp
auto currValue = *b;
```

The compiler already knows what `*b` is. `auto` just piggybacks on it and takes the type.

```cpp
template<typename It>
void dwim(It b, It e) {
    while (b != e) {
        auto currValue = *b;
        ...
    }
}
```

The third point focuses on closures and lambdas.

```cpp
[](int a, int b) { return a < b; }
```

When lambdas are written, it is not a function pointer. It generates an anonymous class (functor) under the hood.

```cpp
auto myLambda = [](int a, int b) { return a < b; };
```

Compiler does this:
```cpp
class __CompilerGeneratedName_89ab3f {
public:
    inline bool operator()(int a, int b) const {
        return a < b;
    }
};

__CompilerGeneratedName_89ab3f myLambda = __CompilerGeneratedName_89ab3f();
```

The name of the class is `__CompilerGeneratedName_89ab3f` which is basically complete gibberish. Without `auto`, you can't declare the variable `lambda`. `auto` deduces the type.

```cpp
auto lambda = [](const auto& p1, const auto& p2) { return *p1 < *p2>; };
```

You also can't try to store the lambda using `std::function`:

```cpp
auto myLambda = [](int a) { return a + 1; };
std::function<int(int)> myFunc = [](int a) { return a + 1; };
```

These are not equivalent at the machine level. Using `auto`, it is a lightweight object on the stack and calling it is a CPU jump which is almost always inlined. `std::function` is a polymorphic wrapper with a virtual table pointer and hidden heap allocations if the internal lambda has large variables.

Another thing to note, using `auto` also ensures perfect sizing.

```cpp
std::vector<int> v;
unsigned sz = v.size();
```

If `v` is massive, the compiler will truncate to the size of `unsigned`. Use `auto` instead:

```cpp
auto sz = v.size();
```

Another thing with `auto` is hashmap copies.

```cpp
std::unordered_map<std::string, int> m;

for (const std::pair<std::string, int>& p : m) {
    ...
}
```

Hashmaps cannot allow modification of keys. Every node stored has a `const` key. If you reference `std::pair<std::string, int>` missing the constant, the compiler will allocate a temporary `std::pair` on the stack, do a full copy, bind the reference, and then destroy it temporarily. Do this instead:

```cpp
for (const auto& p : m)
```
as `auto` deduces the `const` key and the reference binds to memory directly.

---

## Current Summary of Items 1-5

### Compiler and Deduction
* **i) Pass-By-Reference (`T&`):** Compiler preserves `const` to make sure the function cannot modify read-only caller memory.
* **ii) Pass-by-value (`T`):** Compiler creates an isolated stack clone.
* **iii) Universal reference (`T&&`):** Compiler uses lvalue-deduction and reference collapsing to preserve lvalue or rvalue so it can be forwarded without copying.

### Auto vs. Decltype
* **i) `auto`:** A type stripper; drops references and `const` to create pass-by-values. *EXCEPT:* `auto x = {1, 2}` -> forces `std::initializer_list`.
* **ii) `decltype`:** Does no stripping. Reflects the declared type. *EXCEPT:* `decltype((expression))` yields an lvalue reference, so `decltype(auto)` with something like `return (x)` will return a dangling reference to a destroyed stack frame.

* Use `auto` when you want a fresh, independent copy. Make the compiler have it be a pass-by-value. Use it for creating local primitives, iterators, and lambdas. Use `auto&` or `const auto&` for a safe reference/pointer to avoid making copies of expensive objects.
* Use `decltype` when you want the exact type without declaring a variable. It does not instantiate memory.
  ```cpp
  const int x = 42;
  std::vector<decltype(x)> vec; // Compiler will build this as a std::vector<const int>
  ```
* Use `decltype(auto)` when you write forwarders or wrappers. You want to automatically deduce the type but not strip references. Good for generic functions.
  ```cpp
  template<typename Container, typename Index>
  decltype(auto) accessElement(Container& c, Index i) {
      return c[i];
  }
  ```
  ```cpp
  decltype(auto) val = intermediate_function();
  process(std::forward<decltype(val)>(val));
  ```

### Hidden Costs of Explicit Typing
* **i) Vtable/Heap penalty:** Storing a lambda in `std::function` forces the compiler to use type erasure, adding virtual table pointers and hidden heap allocations. `auto` is just a stack functor (faster).
* **ii) `unsigned` vs. `size_t`:** Adding `unsigned` does truncation. `auto` ensures bits match.
* **iii) `std::unordered_map`:** Hash nodes are `const`. If you are using `std::pair<K, V>`, you are making stack variables + deep copy. `auto&` binds directly to memory.
