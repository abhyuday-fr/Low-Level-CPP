# 🧹 Garbage Collector in C++ (Mark-and-Sweep)

A simple educational garbage collector implemented in modern C++ to demonstrate how mark-and-sweep GC works internally.

This project is meant for learning systems programming concepts such as:

- Heap memory management

- Object graphs

- Reachability analysis

- Garbage collection algorithms

⚠️ This is not a production GC.
It is intentionally explicit, verbose, and debuggable.

--- 

# ✨ Features

- Mark-and-sweep garbage collection

- Manual root management

- Object graph traversal (DFS)

- Cycle-safe collection

- Runtime debug tracing

- ASCII memory graph visualization

- Header-only implementation (easy to include)

---

# ⚙️ How the Garbage Collector Works
### 1. Track Objects

  All GC-managed objects must be registered with the collector.

`gc.track(obj);`

### 2. Register Roots

Roots represent entry points into the object graph.

`gc.addRoot(obj);`

### 3. Mark Phase

Starting from roots, the GC:

- Traverses references recursively

- Marks all reachable objects

### 4. Sweep Phase

The GC:

- Deletes unmarked (unreachable) objects

- Resets mark flags for the next cycle

---

# 🔮 Future Improvements

Possible extensions:

- Automatic object registration (operator new)

- Stack root simulation

- Graphviz .dot export

- Incremental or generational GC

- Thread-safe marking

- Integration into a scripting language runtime

---

# 📜 License

MIT License, do whatever you want with it.
