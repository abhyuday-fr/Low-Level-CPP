#include "thread_safe_queue.h"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

std::mutex g_print_mutex;

void log(std::string message)
{
    std::lock_guard<std::mutex> lk(g_print_mutex);
    std::cout << message << "\n";
}

void simulate_work(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void producer(ThreadSafeQueue<int>& q, int producer_id, int items_per_producer, std::atomic<int>& total_produced)
{
    for (int i = 0; i < items_per_producer; i++)
    {
        int item = producer_id * 1000 + i;
        simulate_work(10);

        if (!q.push(item))
        {
            log("[P" + std::to_string(producer_id) + "] shutdown signalled, stopping at item " + std::to_string(i));
            return;
        }

        log("[P" + std::to_string(producer_id) + "] pushed " + std::to_string(item) +
            "  (queue size ~" + std::to_string(q.size()) + "/" + std::to_string(q.capacity()) + ")");

        total_produced++;
    }

    log("[P" + std::to_string(producer_id) + "] finished - pushed " + std::to_string(items_per_producer) + " items.");
}

void consumer(ThreadSafeQueue<int>& q, int consumer_id, std::atomic<int>& total_consumed)
{
    int local_count = 0;

    while (true)
    {
        std::optional<int> item = q.pop();

        if (!item)
            break;

        simulate_work(20);

        log("   [C" + std::to_string(consumer_id) + "] consumed " + std::to_string(*item) +
            "  (queue size ~" + std::to_string(q.size()) + "/" + std::to_string(q.capacity()) + ")");

        total_consumed++;
        local_count++;
    }

    log("   [C" + std::to_string(consumer_id) + "] exiting - consumed " + std::to_string(local_count) + " items.");
}

int main()
{
    const int QUEUE_CAPACITY = 5;
    const int NUM_PRODUCERS = 3;
    const int NUM_CONSUMERS = 2;
    const int ITEMS_PER_PRODUCER = 8;
    const int TOTAL_ITEMS = NUM_PRODUCERS * ITEMS_PER_PRODUCER;

    log("=== ThreadSafeQueue demo ===");
    log("Capacity: " + std::to_string(QUEUE_CAPACITY) +
        "  |  Producers: " + std::to_string(NUM_PRODUCERS) +
        "  |  Consumers: " + std::to_string(NUM_CONSUMERS) +
        "  |  Total items: " + std::to_string(TOTAL_ITEMS));
    log("-------------------------------------------");

    ThreadSafeQueue<int> queue(QUEUE_CAPACITY);

    std::atomic<int> total_produced(0);
    std::atomic<int> total_consumed(0);

    std::vector<std::thread> consumers;
    for (int c = 0; c < NUM_CONSUMERS; c++)
        consumers.push_back(std::thread(consumer, std::ref(queue), c, std::ref(total_consumed)));

    std::vector<std::thread> producers;
    for (int p = 0; p < NUM_PRODUCERS; p++)
        producers.push_back(std::thread(producer, std::ref(queue), p, ITEMS_PER_PRODUCER, std::ref(total_produced)));

    for (int p = 0; p < NUM_PRODUCERS; p++)
        producers[p].join();

    log("-------------------------------------------");
    log("All producers done. Signalling shutdown...");
    queue.shutdown();

    for (int c = 0; c < NUM_CONSUMERS; c++)
        consumers[c].join();

    log("-------------------------------------------");
    log("Total produced : " + std::to_string(total_produced.load()));
    log("Total consumed : " + std::to_string(total_consumed.load()));
    log("Queue size     : " + std::to_string(queue.size()) + "  (should be 0)");

    if (total_produced.load() == TOTAL_ITEMS &&
        total_consumed.load() == TOTAL_ITEMS &&
        queue.empty())
    {
        log("PASS - all items accounted for, no leaks, no hangs.");
        return 0;
    }

    log("FAIL - item count mismatch!");
    return 1;
}