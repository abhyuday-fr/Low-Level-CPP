#pragma once

#include <iostream>
#include <vector>
#include <unordered_set>
#include <string>

//
// ---------- Base GC Object ----------
//
struct GCObject {
    bool marked = false;
    std::string name;
    std::vector<GCObject*> references;

    explicit GCObject(const std::string& n) : name(n) {}

    virtual ~GCObject() {
        std::cout << "Destroying " << name << "\n";
    }
};

//
// ---------- Garbage Collector ----------
//
class GarbageCollector {
public:
    std::unordered_set<GCObject*> objects;
    std::vector<GCObject*> roots;

    // Track newly allocated objects
    inline void track(GCObject* obj) {
        objects.insert(obj);
        std::cout << "Tracking object: " << obj->name << "\n";
    }

    // Register roots
    inline void addRoot(GCObject* obj) {
        roots.push_back(obj);
        std::cout << "Added root: " << obj->name << "\n";
    }

    // ---------- MARK ----------
    inline void mark(GCObject* obj) {
        if (!obj) return;

        if (obj->marked) {
            std::cout << "Already marked: " << obj->name << "\n";
            return;
        }

        obj->marked = true;
        std::cout << "Marking: " << obj->name << "\n";

        for (auto ref : obj->references) {
            std::cout << "  " << obj->name << " -> " << ref->name << "\n";
            mark(ref);
        }
    }

    inline void markAll() {
        std::cout << "\nMARK PHASE START\n";
        for (auto root : roots)
            mark(root);
        std::cout << "MARK PHASE END\n";
    }

    // ---------- SWEEP ----------
    inline void sweep() {
        std::cout << "\nSWEEP PHASE START\n";
        for (auto it = objects.begin(); it != objects.end();) {
            if (!(*it)->marked) {
                std::cout << "Unreachable: " << (*it)->name << "\n";
                delete *it;
                it = objects.erase(it);
            } else {
                std::cout << "Keeping: " << (*it)->name << "\n";
                (*it)->marked = false; // reset for next GC cycle
                ++it;
            }
        }
        std::cout << "SWEEP PHASE END\n";
    }

    // ---------- FULL COLLECTION ----------
    inline void collect() {
        printGraph();
        markAll();
        sweep();
        printGraph();
    }

    // ---------- MEMORY GRAPH ----------
    inline void printGraph() {
        std::cout << "\nMEMORY GRAPH\n";

        std::cout << "Roots:\n";
        if (roots.empty()) {
            std::cout << "  (none)\n";
        } else {
            for (auto r : roots)
                std::cout << "  ROOT -> " << r->name << "\n";
        }

        std::cout << "\nObjects:\n";
        if (objects.empty()) {
            std::cout << "  (heap empty)\n";
        } else {
            for (auto obj : objects) {
                std::cout << "  [" << obj->name << "]";
                if (obj->marked) std::cout << " (marked)";
                std::cout << " -> ";

                if (obj->references.empty()) {
                    std::cout << "(no references)";
                } else {
                    for (auto ref : obj->references)
                        std::cout << ref->name << " ";
                }
                std::cout << "\n";
            }
        }

        std::cout << "--------------------------\n";
    }
};

//
// ---------- Example GC-managed Object ----------
//
struct Node : GCObject {
    explicit Node(const std::string& name)
        : GCObject(name) {
        std::cout << "Created " << name << "\n";
    }
};
