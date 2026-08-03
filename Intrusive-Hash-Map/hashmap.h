#ifndef HASHMAP_H
#define HASHMAP_H

#include <cstddef>
#include <cstdint>

// hashtable node, should be embedded into the payload
struct HNode {
  HNode *next = NULL;
  uint64_t hcode = 0;
};

class HashMap {
private:
  // simple fixed-sized hashtable
  struct HMap {
    HNode **map = NULL; // array of slots
    size_t size = 0;
    size_t mask = 0;
  };

  HMap hmap;

  HNode **lookup_(HNode *key, bool (*eq)(HNode *, HNode *));
  HNode *detach_(HNode **from);
  void insert_(HNode *node);

public:
  HashMap(size_t n);
  HNode *lookup(HNode *key, bool (*eq)(HNode *, HNode *));
  void insert(HNode *node);
  HNode *remove(HNode *key, bool (*eq)(HNode *, HNode *));
  void clear();
  size_t size();
  ~HashMap();
};

#endif
