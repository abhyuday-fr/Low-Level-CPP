#include <iostream>
#include <assert.h>
#include <atomic>
#include <string>
#include <thread>
#include <chrono>
#include <cstdint>
#include <vector>

//==========================
// Data Structures
//==========================

struct Node{
    int key;
    Node* next;

    explicit Node(int key) : key(key), next(nullptr){};
};

// allign to 16 bytes to ensure std::atomic can use 128 bit
struct alignas(16) TaggedPointer{
    Node *node = nullptr;
    uint64_t tag = 0;
};

//==========================
// Lock-Free stack class
//==========================

class LockFreeStack{
private:
    std::atomic<TaggedPointer> top;
    std::atomic<uint64_t> tag_counter;

public:
    LockFreeStack(){
        top.store(TaggedPointer{nullptr, 0});
        tag_counter.store(0);
    }

    void push(int key){
        Node *new_node = new Node(key);

        TaggedPointer old_top = top.load(std::memory_order_acquire);
        TaggedPointer new_top;
        new_top.node = new_node;

        do{
            new_node->next = old_top.node;

            // inrement tag to avoid ABA
            new_top.tag = tag_counter.fetch_add(1, std::memory_order_relaxed);
        }while( !top.compare_exchange_weak(old_top, new_top, std::memory_order_release, std::memory_order_relaxed));
    }

    int pop(){
        TaggedPointer old_top = top.load(std::memory_order_acquire);
        top.load(std::memory_order_acquire);
        TaggedPointer new_top;

        do{
            // stack is empty
            if(old_top.node == nullptr){
                return 0;
            }

            new_top.node = old_top.node->next;
            new_top.tag = tag_counter.fetch_add(1, std::memory_order_relaxed);
        }while( !top.compare_exchange_weak(old_top, new_top, std::memory_order_acquire, std::memory_order_relaxed));

        int key = old_top.node->key;

        delete old_top.node;

        return key;
    }
};

//==========================
// Testing (Produce/Consume)
//==========================

constexpr int NUM_CONSUMERS = 4;
constexpr int ITEMS_PER_CONSUMER = 10000;

// producer pushes items and calculates the expected sum
void produce(LockFreeStack &stack, std::atomic<long> &producer_sum){
    long local_sum = 0;
    int total_items = NUM_CONSUMERS * ITEMS_PER_CONSUMER;

    for(int i = 1; i <= total_items; i++){
        stack.push(i);
        local_sum += i;
    }

    producer_sum.store(local_sum, std::memory_order_relaxed);
}

// consumer pop items and adds them to the total consumer sum
void consume(LockFreeStack &stack, std::atomic<long> &global_consumer_sum){
    long local_sum = 0;

    for(int i = 0; i < ITEMS_PER_CONSUMER; i++){
        int val = 0;

        while((val = stack.pop()) == 0){
            std::this_thread::yield(); // be polite to the CPU 
        }

        local_sum += val;
    }

    // safely adding this thread's subtotal to the global sum
    global_consumer_sum.fetch_add(local_sum, std::memory_order_relaxed);
}

//==========================
// Main 
//==========================

int main(){
    LockFreeStack stack;
    std::atomic<long> producer_sum{0};
    std::atomic<long> consumer_sum{0};

    auto start_time = std::chrono::high_resolution_clock::now();

    // start 1 producer thread
    std::thread prod_thread(produce, std::ref(stack), std::ref(producer_sum));

    // start N consumer threads
    std::vector<std::thread> cons_threads;
    for(int i = 0; i < NUM_CONSUMERS; i++){
        cons_threads.emplace_back(consume, std::ref(stack), std::ref(consumer_sum));
    }

    // join all threads
    prod_thread.join();
    for(auto &t : cons_threads){
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff = end_time - start_time;

    // validate the lock-free integrity
    long p_sum = producer_sum.load();
    long c_sum = consumer_sum.load();

    std::cout << "Time: " << diff.count() << "s\n";

    if (p_sum == c_sum && p_sum != 0) {
        std::cout << "PASS (Produced: " << p_sum << " | Consumed: " << c_sum << ")\n";
    } else {
        std::cout << "FAIL at validation step!\n";
    }

    return 0;
}