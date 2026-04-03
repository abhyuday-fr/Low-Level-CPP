# Lock-Free Stack in C++

A high-performance, thread-safe stack built using atomic compare-and-swap (CAS) operations with no mutexes, no locks, just pure hardware-level concurrency.

---

## Build & Run

**Requirements:** GCC with C++11 or later, on a platform supporting 128-bit atomics (x86-64).

```bash
# Compile
g++ main.cpp -o stack -mcx16 -latomic

# Run
./stack
```

**Flag breakdown:**
- `-mcx16` — enables the `CMPXCHG16B` instruction, required for 128-bit atomic operations on `TaggedPointer`
- `-latomic` — links the GCC atomic support library

**Expected output:**
```
Time: 0.0XXs
PASS (Produced: 800020000 | Consumed: 800020000)
```

---

## What is CAS (Compare-And-Swap)?

Compare-And-Swap is a single atomic CPU instruction that does the following in one uninterruptible step:

```
if (current_value == expected_value) {
    current_value = new_value;
    return true;
} else {
    expected_value = current_value;  // update caller's view
    return false;
}
```

It is the foundation of lock-free programming. Instead of acquiring a mutex and blocking other threads, a CAS-based algorithm reads the current state, computes a new state, and then atomically tries to swap — retrying if another thread changed the value in between.

In this stack, both `push` and `pop` use `compare_exchange_weak` in a retry loop:

```cpp
while (!top.compare_exchange_weak(old_top, new_top, ...)) {
    // another thread changed `top`, retry with the updated value
}
```

This gives you progress without locks: threads never block each other, they just retry.

---

## The ABA Problem

CAS checks whether a value is **equal** to what we expect — but equal doesn't always mean **unchanged**.

Consider this scenario with a naive (untagged) pointer stack:

```
Initial state:   top → A → B → C

Thread 1:  reads top = A, prepares to pop (set top = B)
           ... gets preempted ...

Thread 2:  pops A  →  top = B → C
Thread 2:  pops B  →  top = C
Thread 2:  pushes A back  →  top = A → C   (same pointer, new context!)

Thread 1:  resumes, CAS sees top == A ✓ (matches its snapshot)
           sets top = B  →  but B is already freed!
```

Thread 1's CAS **succeeds incorrectly** because the pointer looks the same, even though the stack's logical state completely changed. This is the ABA problem: the value went A → B → A, and the CAS couldn't tell.

The consequence can be a **use-after-free**, a corrupted linked list, or silent data loss — all without any crashes or warnings.

---

## How This Stack Fixes It: Version Tagging

The fix used here is to replace the bare `Node*` pointer with a **tagged pointer** — a struct pairing the pointer with a monotonically increasing version counter:

```cpp
struct alignas(16) TaggedPointer {
    Node*    node = nullptr;
    uint64_t tag  = 0;        // version number — incremented on every CAS
};
```

Now every push and pop increments a global tag counter and stores the new tag alongside the pointer:

```cpp
new_top.tag = tag_counter.fetch_add(1, std::memory_order_relaxed);
```

The CAS now checks **both** the pointer and the tag:

```
old:  { node = A, tag = 5 }
new:  { node = B, tag = 6 }
```

Even if another thread pops and re-pushes node A, it will do so with a **newer tag** (e.g., tag = 8). When Thread 1 resumes and tries its CAS, it sees `{ A, tag=5 }` vs the actual `{ A, tag=8 }` — **mismatch, CAS fails**, retry happens. ABA is defeated.

The `alignas(16)` is critical: it ensures `TaggedPointer` sits on a 16-byte boundary so the CPU can atomically read/write all 128 bits in one `CMPXCHG16B` instruction.

---

## Other Ways to Solve the ABA Problem

### 1. Hazard Pointers
Each thread declares which node it is currently reading by publishing a "hazard pointer". Before freeing a node, the thread scans all hazard pointers across all threads — if any thread is still reading that node, deletion is deferred. This avoids dangling pointers entirely and doesn't rely on tag width.

**Trade-off:** More complex to implement; requires a shared hazard pointer table and a deferred-deletion queue per thread.

### 2. Epoch-Based Reclamation (EBR)
Threads register themselves in a global epoch. Memory is not freed until all threads have passed through a new epoch, guaranteeing no thread holds a stale reference. Used in production systems like Folly's lock-free structures.

**Trade-off:** Lower overhead than hazard pointers at runtime, but stalled threads can prevent memory from being reclaimed indefinitely.

### 3. Quiescent State-Based Reclamation (QSBR)
A lighter variant of EBR where threads signal "quiescent states" — moments where they hold no references to shared data. Safe reclamation happens between these signals.

**Trade-off:** Requires explicit quiescent-state annotations in application code; very fast when used correctly.

### 4. Garbage Collection
In GC-managed languages (Java, Go, C#), the runtime handles object lifetime — a node won't be freed while any thread holds a reference to it, so ABA via dangling pointers is impossible. Lock-free structures in the JVM exploit this heavily.

**Trade-off:** Not available in C++ without bringing in a full GC library; carries GC pause overhead.

### 5. Wider Tags / Pointer Packing
Extend the tag to 128 bits (as done here) or pack a tag into the unused high bits of a 64-bit pointer (pointer tagging, common on x86-64 where only 48 bits are used for addressing). A sufficiently wide tag makes counter wraparound practically impossible.

**Trade-off:** Relies on tag never wrapping around; `uint64_t` gives ~1.8 × 10¹⁹ increments before overflow — safe for virtually all real workloads.

---

## Concurrency Design Summary

| Aspect | Detail |
|---|---|
| Threads | 1 producer, 4 consumers |
| Items | 40,000 total (10,000 per consumer) |
| ABA fix | 128-bit tagged pointer with atomic version counter |
| Memory order | `acquire`/`release` on top, `relaxed` on tag counter |
| Validation | Producer sum == Consumer sum |

---

> **Note:** This stack does not implement safe memory reclamation — `delete old_top.node` in `pop()` can still race under certain patterns in a fully general setting. For production use, pair this with hazard pointers or EBR to handle reclamation safely.