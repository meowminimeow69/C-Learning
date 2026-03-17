#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            // Each worker thread runs this lambda
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        // 1. Lock the queue to safely grab a task
                        std::unique_lock<std::mutex> lock(this->queueMutex);
                        
                        // 2. Wait until there is work OR the pool is shutting down
                        this->condition.wait(lock, [this] { 
                            return this->stop || !this->tasks.empty(); 
                        });

                        // 3. If stopping and no tasks left, exit thread
                        if (this->stop && this->tasks.empty()) return;

                        // 4. Get the next task from the front of the queue
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    
                    // 5. Execute the task outside the lock!
                    task();
                }
            });
        }
    }

    // Add a new job to the pool
    void enqueue(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.push(std::move(task));
        }
        condition.notify_one(); // Wake up one sleeping worker
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        condition.notify_all(); // Wake up EVERYONE to check the stop flag
        for (std::thread &worker : workers) {
            worker.join(); // Wait for threads to finish their current task
        }
    }

private:
    std::vector<std::thread> workers;        // Our "employees"
    std::queue<std::function<void()>> tasks; // The "to-do list"
    
    std::mutex queueMutex;           // Prevents threads from grabbing the same task
    std::condition_variable condition; // The "alarm clock" for workers
    bool stop;                       // The "off switch"
};