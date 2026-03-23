#include "thread_safe_queue.h"

#include <string>
#include <utility>

//---------------
// Construction
//---------------

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(std::size_t max_size) : max_size(max_size), done(false){
	if (max_size == 0) {
		throw std::invalid_argument("ThreadSafeQueue: max_size must be > 0");
	}
}

//----------------
// Producer
//----------------

