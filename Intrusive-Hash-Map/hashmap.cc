#include "hashmap.h"
#include <cassert>
#include <cstdlib>

HashMap::HashMap(size_t n) { // n must be a power of 2
  assert(n > 0 && ((n - 1) & n) == 0);
  hmap.map = (HNode **)calloc(n, sizeof(HNode *));
  hmap.mask = n - 1;
  hmap.size = 0;
}

void HashMap::insert_(HNode *node) {
  size_t pos = node->hcode & hmap.mask;
  HNode *next = hmap.map[pos];
  node->next = next;
  hmap.map[pos] = node;
  hmap.size++;
}

HNode **HashMap::lookup_(HNode *key, bool (*eq)(HNode *, HNode *)) {
  if (!hmap.map) {
    return NULL;
  }

  size_t pos = key->hcode & hmap.mask;
  HNode **from = &hmap.map[pos]; // incoming pointer to the target

  for (HNode *cur; (cur = *from) != NULL; from = &cur->next) {
    if (cur->hcode == key->hcode && eq(cur, key)) {
      return from;
    }
  }

  return NULL;
}

HNode *HashMap::detach_(HNode **from) {
  HNode *node = *from; // the target node
  *from = node->next;  // update the incoming pointer to the target
  hmap.size--;
  return node;
}

HNode *HashMap::lookup(HNode *key, bool (*eq)(HNode *, HNode *)) {
  HNode **from = lookup_(key, eq);
  return from ? *from : NULL;
}

void HashMap::insert(HNode *node) {
  insert_(node); // lmao why did I even do that? (maybe to add new feature in
                 // future?)
}

HNode *HashMap::remove(HNode *key, bool (*eq)(HNode *, HNode *)) {
  if (HNode **from = lookup_(key, eq)) {
    return detach_(from);
  }
  return NULL;
}

void HashMap::clear() { free(hmap.map); }

size_t HashMap::size() { return hmap.size; }

HashMap::~HashMap() { clear(); }
