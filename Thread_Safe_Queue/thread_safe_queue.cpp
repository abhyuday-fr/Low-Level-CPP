#include "thread_safe_queue.h"

#include <stdexcept>
#include <string>

template <typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(int max_size)
    : max_size_(max_size), done_(false)
{
    if (max_size <= 0)
        throw std::invalid_argument("max_size must be greater than 0");
}

template <typename T>
bool ThreadSafeQueue<T>::push(T value)
{
    std::unique_lock<std::mutex> lock(mutex_);

    not_full_.wait(lock, [this] {
        return done_ || has_space();
        });

    if (done_)
        return false;

    queue_.push_back(value);
    not_empty_.notify_one();
    return true;
}

template <typename T>
bool ThreadSafeQueue<T>::try_push(T value)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (done_ || !has_space())
        return false;

    queue_.push_back(value);
    not_empty_.notify_one();
    return true;
}

template <typename T>
std::optional<T> ThreadSafeQueue<T>::pop()
{
    std::unique_lock<std::mutex> lock(mutex_);

    not_empty_.wait(lock, [this] {
        return done_ || has_item();
        });

    if (!has_item())
        return std::nullopt;

    T value = queue_.front();
    queue_.pop_front();
    not_full_.notify_one();
    return value;
}

template <typename T>
std::optional<T> ThreadSafeQueue<T>::try_pop()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!has_item())
        return std::nullopt;

    T value = queue_.front();
    queue_.pop_front();
    not_full_.notify_one();
    return value;
}

template <typename T>
void ThreadSafeQueue<T>::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);
    done_ = true;
    not_full_.notify_all();
    not_empty_.notify_all();
}

template <typename T>
int ThreadSafeQueue<T>::size() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return (int)queue_.size();
}

template <typename T>
bool ThreadSafeQueue<T>::empty() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

template <typename T>
int ThreadSafeQueue<T>::capacity() const
{
    return max_size_;
}

template <typename T>
bool ThreadSafeQueue<T>::is_shutdown() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return done_;
}

template class ThreadSafeQueue<int>;
template class ThreadSafeQueue<std::string>;