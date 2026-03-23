#ifndef THRD_SAFE_Q_H
#define THRD_SAFE_Q_H

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>

template<typename T>
class ThreadSafeQueue {
public:
	explicit ThreadSafeQueue(std::size_t max_size);

	~ThreadSafeQueue() = default;

	ThreadSafeQueue(const ThreadSafeQueue&) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
	ThreadSafeQueue(ThreadSafeQueue&&) = delete;
	ThreadSafeQueue& operator=(ThreadSafeQueue&&) = delete;

	//----------------------
	// Producer
	//----------------------

	// push: blocking. If the queue is full, the calling thread sleeps on a condition variable until a slot opens up.
	// It will always eventually enqueue the item (or return false on shutdown).
	bool push(T value);

	// try_push: non-blocking: If the queue is full right now, it returns false immediately and walks away.
	// The caller keeps control and decides what to do
	bool try_push(T value);

	//-----------------------
	// Consumer
	//-----------------------

	// pop: sleeps until an item is available.
	std::optional<T> pop();

	// try_pop: return The front item, or std::nullopt if the queue is empty.
	std::optional<T> try_pop();


	// Control
	void shutdown();

	// Observers
	[[nodiscard]] std::size_t size() const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] std::size_t capacity() const noexcept;
	[[nodiscard]] bool is_shutdown() const;
private:
	//helpers
	[[nodiscard]] bool has_space() const noexcept;
	[[nodiscard]] bool has_item() const noexcept;

	//data members
	const std::size_t max_size_;
	mutable std::mutex mutex_;
	std::condition_variable not_full_;
	std::condition_variable not_empty_;
	std::deque<T> queue;
	bool done_; 
};

#endif