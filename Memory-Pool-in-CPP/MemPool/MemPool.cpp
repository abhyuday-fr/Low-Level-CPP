#include "MemPool.h"
#include <algorithm>

MemoryManager::MemoryManager(const size_t ObjectSize, const size_t Amount) {
	chunkSize_ = std::max(ObjectSize, sizeof(void*)); // the chunk should be large enough to hold the pointer

	memory_ = new char[chunkSize_ * Amount]; // allocated the pool

	freeListHead_ = memory_; // pointing to the start
	char* current = memory_; // self explanatory

	for (size_t i = 0; i < Amount - 1; i++) {
		char* next = current + chunkSize_;
		// Cast the current block to a void** and store the address of the next block inside it
		*reinterpret_cast<void**>(current) = next;
		current = next;
	}

	// The last block points to nullptr, marking the end of the free list
	*reinterpret_cast<void**>(current) = nullptr;

}

MemoryManager::~MemoryManager() {
	delete[] memory_; // emptying pool
}

void* MemoryManager::Allocate() {

	if (!freeListHead_) {
		return nullptr; // Out of memory (I will try to allocate a new pool in upcoming update)
	}

	void* memoryChunk = freeListHead_;
	freeListHead_ = *reinterpret_cast<void**>(memoryChunk);
	return memoryChunk;
}

void MemoryManager::Deallocate(void* memoryAddress) {

	if (!memoryAddress) return;

	*reinterpret_cast<void**>(memoryAddress) = freeListHead_;

	freeListHead_ = memoryAddress;
}