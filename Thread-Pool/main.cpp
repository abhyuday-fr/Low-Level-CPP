#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <string>
#include <sstream>
#include <queue>
#include <condition_variable>

class ThreadPool{
public:
    ThreadPool(size_t numThreads) : stop_(false){
        for(size_t i = 0; i < numThreads; i++){
            workers_.emplace_back([this]{
                for(;;){
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    condition_.wait(lock, [this]{ return stop_ || !tasks_.empty(); });
                    if(stop_ && tasks_.empty()){
                        return;
                    }
                    auto task = std::move(tasks_.front()); // extract task from tasks list
                    tasks_.pop(); // remove task from list as going to execure it
                    lock.unlock(); // unlock mutex, so another thread can accept the tasks
                    task();
                }
            });
        }
    }

    template<class F>
    void enqueue(F&& task){
        std::unique_lock<std::mutex> lock(queueMutex_);
        tasks_.emplace(std::forward<F>(task));
        lock.unlock();
        condition_.notify_all();
    }

    ~ThreadPool(){
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
        lock.unlock();
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
    std::string mystr = ss.str();
    return mystr;
}

int main(){
    ThreadPool Pool(4); // create a pool with N number of worker threads

    std::cout << "Thread Pool Created\n";
    std::cout << "Enqueue(Assign) some tasks\n";

    for(int i = 0; i < 8; i++){
        Pool.enqueue([i]{
            std::cout << "Task " << i << " " << getThreadID().c_str() << " executed by thread\n";
            std::this_thread::sleep_for(std::chrono::seconds(1)); // simulate some work
        });
    }

    // main thread continues doing other things while the tasks are executed in the background

    return 0;
}