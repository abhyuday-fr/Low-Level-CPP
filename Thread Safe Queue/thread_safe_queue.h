#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>

template <typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(int max_size);

    bool push(T value);
    bool try_push(T value);

    std::optional<T> pop();
    std::optional<T> try_pop();

    void shutdown();

    int  size() const;
    bool empty() const;
    int  capacity() const;
    bool is_shutdown() const;

private:
    bool has_space() const { return (int)queue_.size() < max_size_; }
    bool has_item()  const { return !queue_.empty(); }

    int                     max_size_;
    mutable std::mutex      mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    std::deque<T>           queue_;
    bool                    done_;
};