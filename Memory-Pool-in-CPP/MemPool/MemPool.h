#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <cstddef>

class MemoryManager {
    void* freeListHead_;
    char* memory_;
    size_t chunkSize_;

public:
    // Constructor and Destructor
    MemoryManager(const size_t ObjectSize, const size_t Amount);
    ~MemoryManager();

    // Core pool operations
    void* Allocate();
    void Deallocate(void* memoryAddress);
};

#endif