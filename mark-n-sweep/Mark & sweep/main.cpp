#include "garbage_collector.h"

int main() {
    GarbageCollector gc;

    Node* a = new Node("A");
    Node* b = new Node("B");
    Node* c = new Node("C");

    a->references.push_back(b);
    b->references.push_back(c);

    gc.track(a);
    gc.track(b);
    gc.track(c);

    gc.addRoot(a);

    std::cout << "\n====== FIRST GC ======\n";
    gc.collect();

    gc.roots.clear();

    std::cout << "\n====== SECOND GC ======\n";
    gc.collect();

    return 0;
}
