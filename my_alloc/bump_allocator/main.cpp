#include <cstdlib>

static unsigned char heap[1024 * 1024];
static unsigned char *heap_ptr = heap;

// simple bump allocator

void *my_alloc(size_t size) {

  if (size + heap_ptr > heap + sizeof(heap))
    return NULL;
  void *p = heap_ptr;
  heap_ptr += size;
  return p;
}

int main() {
  void *p = my_alloc(10);
  return EXIT_SUCCESS;
}
