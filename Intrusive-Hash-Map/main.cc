#include "hashmap.h"
#include <cstddef> // for offsetof
#include <functional>
#include <iostream>
#include <string>

#define container_of(ptr, T, member) ((T *)((char *)ptr - offsetof(T, member)))

struct Entry {
  std::string key;
  std::string value;
  HNode node;
};

static bool entry_eq(HNode *lhs, HNode *rhs) {
  Entry *entry_a = container_of(lhs, Entry, node);
  Entry *entry_b = container_of(rhs, Entry, node);
  return entry_a->key == entry_b->key;
}

uint64_t hash_string(const std::string &s) {
  return std::hash<std::string>{}(s);
}

int main() {
  HashMap map(16); // must be a power of 2

  Entry *e1 = new Entry{"user_1", "John Doe", {NULL, hash_string("user_1")}};

  Entry *e2 = new Entry{"user_2", "Jane Doe", {NULL, hash_string("user_2")}};

  map.insert(&e1->node);
  map.insert(&e2->node);

  std::cout << "Map Size: " << map.size() << "\n";

  Entry dummy_lookup;
  dummy_lookup.key = "user_1";
  dummy_lookup.node.hcode = hash_string("user_1");

  HNode *lookup_node = map.lookup(&dummy_lookup.node, entry_eq);
  if (lookup_node) {
    Entry *entry = container_of(lookup_node, Entry, node);
    std::cout << "Lookup Successful, found key: " << entry->key
              << " value: " << entry->value << "\n";
  }

  Entry dummy_remove;
  dummy_remove.key = "user_2";
  dummy_remove.node.hcode = hash_string("user_2");

  HNode *remove_node = map.remove(&dummy_remove.node, entry_eq);
  if (remove_node) {
    Entry *entry = container_of(remove_node, Entry, node);
    std::cout << "Succesfully removed key: " << entry->key
              << " value: " << entry->value;
    delete entry;
  }

  delete e1;

  return 0;
}
