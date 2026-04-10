#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <string>
#include <sstream>
#include <queue>
#include <condition_variable>

std::mutex coutMutex;

class ThreadPool{
public:
    ThreadPool(size_t numThreads) : stop_(false){
        for(size_t i = 0; i < numThreads; i++){
            workers_.emplace_back([this]{
                for(;;){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queueMutex_);
                        condition_.wait(lock, [this]{ return stop_ || !tasks_.empty(); });
                        if(stop_ && tasks_.empty()){
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& task){
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if(stop_) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace(std::forward<F>(task));
        }
        condition_.notify_one();
    }

    ~ThreadPool(){
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for(std::thread &worker : workers_){
            worker.join();
        }
    }
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    std::mutex queueMutex_;
    std::condition_variable condition_;
    bool stop_;
};

std::string getThreadID(){
    auto myID = std::this_thread::get_id();
    std::stringstream ss;
    ss << myID;
    return ss.str();
}

int main(){
    ThreadPool Pool(4); // create a pool with 4 worker threads

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "Thread Pool Created with 4 threads\n";
        std::cout << "Enqueueing 8 tasks...\n";
    }

    for(int i = 0; i < 8; i++){
        Pool.enqueue([i]{
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "Task " << i << " (Thread ID: " << getThreadID() << ") started\n";
            }
            std::this_thread::sleep_for(std::chrono::seconds(1)); // simulate some work
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "Task " << i << " completed\n";
            }
        });
    }

    // The destructor of ThreadPool will be called when Pool goes out of scope,
    // which will wait for all tasks to complete.
    return 0;
}