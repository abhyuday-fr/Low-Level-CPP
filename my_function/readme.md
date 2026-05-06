# my::function

A lightweight, header-only C++17 implementation of a type-erased callable wrapper, inspired by `std::function`. 

This library allows you to store and invoke any callable target—including function pointers, lambdas, and functors—while leveraging **Small Buffer Optimization (SBO)** to strictly avoid expensive heap allocations for small objects.

## Features

* **Drop-in Concept:** Works similarly to `std::function` for storing general callables.
* **Small Buffer Optimization (SBO):** Callables smaller than 32 bytes (like raw function pointers and captureless or small-capture lambdas) are instantiated directly on the stack via placement `new`.
* **Type Erasure:** Utilizes the Concept-Model idiom to safely hide the underlying type of the callable while maintaining a uniform interface.
* **Header-Only:** No complex build systems required. Just drop `my_function.hpp` into your project and `#include` it.
* **Zero Dependencies:** Relies purely on the C++ Standard Library (`<memory>`, `<utility>`, `<functional>`, `<type_traits>`).

## How It Works Under the Hood

`my::function` solves the problem of storing varying types with a unified interface using two primary techniques:

1. **The Concept-Model Idiom:** The class holds a pointer to an abstract base class (`CallableBase`). When a specific callable (like a lambda) is assigned, a templated subclass (`CallableModel<F>`) is generated to wrap that exact type and override the virtual invocation methods.
2. **Local Byte Buffer:** The class contains an aligned `std::byte` array of 32 bytes. At compile time, if the decayed type of the assigned callable fits within this buffer and meets alignment requirements, memory is allocated locally inside the object footprint rather than on the heap. Custom destruction logic ensures `delete` is never erroneously called on stack memory.

## Usage

Because it is header-only, simply include the file in your source code.
```cpp
#include <iostream>
#include "my_function.hpp"

// 1. Free Functions
void print_hello() {
    std::cout << "Hello, World!\n";
}

// 2. Large Functors
struct HeavyFunctor {
    int payload[100]; // Will force a heap allocation (bypasses SBO)
    void operator()(int x) const {
        std::cout << "Heavy functor processed: " << x << "\n";
    }
};

int main() {
    // Stores a function pointer (Uses SBO)
    my::function<void()> f1 = print_hello;
    f1();

    // Stores a lambda with a small capture (Uses SBO)
    int multiplier = 10;
    my::function<int(int)> f2 = [multiplier](int x) {
        return x * multiplier;
    };
    std::cout << "Lambda returned: " << f2(5) << "\n";

    // Stores a large functor (Allocates on Heap)
    my::function<void(int)> f3 = HeavyFunctor{};
    f3(42);

    return 0;
}
```

## Compilation

Ensure your compiler is set to C++17 or higher.

```
  g++ -std=c++17 -Wall -Wextra main.cpp -o my_function
  ./my_function
```

## Memory Management

The class correctly handles the Rule of Five, ensuring deep copies of the underlying
callable (whether stored via SBO or on the heap) and safely moving resources without
leaking memory or triggering double-frees on stack-allocated placement buffers.
