# Thread-Safe Bounded Queue in C++

A multi-producer / multi-consumer (MPMC) bounded blocking queue implemented in C++ using mutexes and condition variables. Built with correctness as the primary goal — no deadlocks, no missed wakeups, no spurious wakeup bugs.

---

## What it does

- Multiple producer threads can push items concurrently
- Multiple consumer threads can pop items concurrently
- The queue has a fixed capacity — producers block when it is full and unblock as soon as a slot opens
- Consumers block when the queue is empty and unblock as soon as an item arrives
- A clean shutdown mechanism lets producers finish, then consumers drain all remaining items before exiting

---

## Files

| File | Purpose |
|---|---|
| `thread_safe_queue.h` | Class declaration — members, method signatures |
| `thread_safe_queue.cpp` | Method implementations + explicit template instantiations |
| `main.cpp` | Demo: 3 producers, 2 consumers, queue capacity 5, 24 total items |

---

## How to build

**Visual Studio (Windows)**

1. Create a new Console App project
2. Add all three files via *Add -> Existing Item*
3. Set C++ language standard to C++17:
   - Right-click project -> Properties
   - C/C++ -> Language -> C++ Language Standard -> ISO C++17
4. Press `Ctrl+F5` to build and run

**GCC / Clang (Linux or WSL)**

```bash
g++ -std=c++17 -O2 -pthread thread_safe_queue.cpp main.cpp -o demo
./demo
```

---

## Sample output

```
=== ThreadSafeQueue demo ===
Capacity: 5  |  Producers: 3  |  Consumers: 2  |  Total items: 24
-------------------------------------------
[P0] pushed       0  (queue size ~1/5)
[P1] pushed    1000  (queue size ~2/5)
[P2] pushed    2000  (queue size ~3/5)
   [C0] consumed     0  (queue size ~2/5)
...
-------------------------------------------
All producers done. Signalling shutdown...
   [C0] exiting - consumed 12 items.
   [C1] exiting - consumed 12 items.
-------------------------------------------
Total produced : 24
Total consumed : 24
Queue size     : 0  (should be 0)
PASS - all items accounted for, no leaks, no hangs.
```

---

## Key concepts used

**`std::mutex`**
A mutual exclusion lock. Only one thread can hold it at a time. Every read or write to shared state (the queue, the `done_` flag) happens while holding this lock.

**`std::condition_variable`**
A signalling primitive. A thread can sleep on it (`wait`) and another thread can wake it up (`notify_one` or `notify_all`). Two condition variables are used:
- `not_full_` — producers sleep here when the queue is full; consumers signal it after a pop
- `not_empty_` — consumers sleep here when the queue is empty; producers signal it after a push

**`std::unique_lock`**
A lock wrapper that can temporarily release the mutex. Required for `condition_variable::wait()` because the thread must release the lock while sleeping, then reacquire it on wakeup.

**`std::lock_guard`**
A simpler lock wrapper that just locks on construction and unlocks on destruction. Used everywhere a condition variable is not needed.

**`std::optional<T>`**
A wrapper that holds either a value of type `T` or nothing (`std::nullopt`). Used as the return type of `pop()` — returns an item when one is available, or `std::nullopt` when the queue is shut down and empty. Lets the consumer loop exit naturally with no sentinel values.

**`std::atomic<int>`**
An integer that is safe to increment from multiple threads simultaneously without a mutex. Used for the `total_produced` and `total_consumed` counters in the demo.

**`std::deque<T>`**
The underlying storage. Supports efficient `push_back` and `pop_front`, which is the FIFO access pattern the queue needs.

**Explicit template instantiation**
Because the method bodies live in the `.cpp` file rather than the header, the compiler needs to be told upfront which concrete types to generate. The bottom of `thread_safe_queue.cpp` lists them:
```cpp
template class ThreadSafeQueue<int>;
template class ThreadSafeQueue<std::string>;
```
Add a new line here for any other type you want to use.

---

## Safety guarantees

**No missed wakeups**
The condition predicate is always checked while holding the mutex. A `notify_one()` call from another thread cannot slip in between the predicate returning false and the thread going to sleep, because both happen under the same lock.

**No spurious wakeup bugs**
`condition_variable::wait(lock, predicate)` re-checks the predicate every time the thread wakes up. If it wakes for no real reason (spurious wakeup), the predicate returns false and it goes back to sleep automatically.

**No deadlocks**
Only one mutex exists in the entire design. No thread ever tries to acquire a second lock while holding the first. All locks are RAII — they are always released when the variable goes out of scope, even if an exception is thrown.

---

## Using a different type

To use the queue with a type other than `int` or `std::string`, add an explicit instantiation at the bottom of `thread_safe_queue.cpp`:

```cpp
template class ThreadSafeQueue<float>;
template class ThreadSafeQueue<MyStruct>;
```

Then include the header and use it normally:

```cpp
ThreadSafeQueue<float> q(10);
q.push(3.14f);
std::optional<float> val = q.pop();
```

---

## Requirements

- C++17 or later
- Compiler: MSVC (Visual Studio 2019+), GCC 7+, or Clang 5+
- No third-party dependencies
