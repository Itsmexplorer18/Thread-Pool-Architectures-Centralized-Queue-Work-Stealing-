#ifndef FORKJOINPOOL_H
#define FORKJOINPOOL_H

#include <iostream>
#include <vector>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <memory>
#include <random>
#include <chrono>

class RecursiveTask {
public:
    virtual ~RecursiveTask() = default;
    virtual void compute() = 0;
    virtual bool shouldFork() const = 0;
};

class QuickSortTask : public RecursiveTask {
private:
    std::vector<int> &arr;
    ForkJoinPool &pool;
    int leftInit, rightInit;
    const int threshold;

    static int partition(std::vector<int> &a, int l, int r) {
        int pivot = a[r];
        int i = l - 1;
        for (int j = l; j < r; ++j) {
            if (a[j] <= pivot) {
                ++i;
                std::swap(a[i], a[j]);
            }
        }
        std::swap(a[i + 1], a[r]);
        return i + 1;
    }

    static void sequentialSort(std::vector<int> &a, int l, int r) {
        if (l < r) {
            int pi = partition(a, l, r);
            sequentialSort(a, l, pi - 1);
            sequentialSort(a, pi + 1, r);
        }
    }

public:
class WorkStealingDeque {
private:
    std::deque<std::unique_ptr<RecursiveTask>> tasks;
    mutable std::mutex mtx;
    int ownerID;

public:
    WorkStealingDeque(int id = -1) : ownerID(id) {}

    void push(std::unique_ptr<RecursiveTask> task) {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push_back(std::move(task));
    }

    std::unique_ptr<RecursiveTask> pop() {
        std::lock_guard<std::mutex> lock(mtx);
        if (tasks.empty()) return nullptr;
        auto task = std::move(tasks.back());
        tasks.pop_back();
        return task;
    }

    std::unique_ptr<RecursiveTask> steal(int thiefID) {
        std::lock_guard<std::mutex> lock(mtx);
        if (tasks.empty()) return nullptr;
         // {
        //     std::lock_guard<std::mutex> cout_lock(cout_mutex);
        //     std::cout << "[STEAL] Thread " << thiefID
        //               << " stealing from Thread " << ownerID
        //               << " (victim has " << tasks.size() << " tasks)\n";
        // }
        auto task = std::move(tasks.front());
        tasks.pop_front();
        return task;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return tasks.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return tasks.size();
    }
};
class ForkJoinPool {
private:
    std::vector<std::thread> workers;
    std::vector<std::unique_ptr<WorkStealingDeque>> queues;
    std::atomic<bool> shutdown{false};
    std::atomic<int> activeWorkers{0};
    std::mutex globalMtx;
    std::condition_variable globalCV;
    thread_local static int workerID;
    int numWorkers;

    void workerThread(int id) {
        workerID = id;
        
        while (!shutdown.load()) {
            std::unique_ptr<RecursiveTask> task = nullptr;
            
            // Try to get task from own queue
            task = queues[id]->pop();
            
            // If no task, try to steal from other queues
            if (!task) {
                for (int i = 0; i < numWorkers && !task; ++i) {
                    if (i != id) {
                        task = queues[i]->steal();
                    }
                }
            }
            
            if (task) {
                activeWorkers.fetch_add(1);
                task->compute();
                activeWorkers.fetch_sub(1);
                globalCV.notify_all();
            } else {
                // No work available, wait briefly
                std::unique_lock<std::mutex> lock(globalMtx);
                globalCV.wait_for(lock, std::chrono::microseconds(100));
            }
        }
    }

public:
    explicit ForkJoinPool(int threads = std::thread::hardware_concurrency()) 
        : numWorkers(threads) {
        
        for (int i = 0; i < numWorkers; ++i) {
            queues.push_back(std::make_unique<WorkStealingDeque>());
        }
        
        for (int i = 0; i < numWorkers; ++i) {
            workers.emplace_back(&ForkJoinPool::workerThread, this, i);
        }
    }

    ~ForkJoinPool() {
        shutdown.store(true);
        globalCV.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void submit(std::unique_ptr<RecursiveTask> task) {
        int id = workerID;
        if (id >= 0 && id < numWorkers) {
            queues[id]->push(std::move(task));
        } else {
            // Called from outside worker thread, distribute randomly
            static thread_local std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<> dis(0, numWorkers - 1);
            queues[dis(gen)]->push(std::move(task));
        }
        globalCV.notify_one();
    }

    void invoke(std::unique_ptr<RecursiveTask> task) {
        submit(std::move(task));
        waitForCompletion();
    }

    void waitForCompletion() {
        std::unique_lock<std::mutex> lock(globalMtx);
        globalCV.wait(lock, [this] {
            bool allEmpty = true;
            for (const auto& q : queues) {
                if (!q->empty()) {
                    allEmpty = false;
                    break;
                }
            }
            return allEmpty && activeWorkers.load() == 0;
        });
    }

    static ForkJoinPool& getCurrentPool() {
        static ForkJoinPool pool;
        return pool;
    }
};

thread_local int ForkJoinPool::workerID = -1;

    QuickSortTask(std::vector<int> &array, ForkJoinPool &poolRef, int l, int r, int thresh = 200000)
        : arr(array), pool(poolRef), leftInit(l), rightInit(r), threshold(thresh) {}

    bool shouldFork() const override { return (rightInit - leftInit) > threshold; }

    void compute() override {
        int l = leftInit;
        int r = rightInit;

        while (l < r) {
            if ((r - l) <= threshold) {
                sequentialSort(arr, l, r);
                return;
            }

            int pi = partition(arr, l, r);

            int leftL = l;
            int leftR = pi - 1;
            int rightL = pi + 1;
            int rightR = r;

            int leftSize = (leftR >= leftL) ? (leftR - leftL + 1) : 0;
            int rightSize = (rightR >= rightL) ? (rightR - rightL + 1) : 0;

            if (leftSize < rightSize) {
                if (leftSize > 0) {
                    pool.submit(std::make_unique<QuickSortTask>(arr, pool, leftL, leftR, threshold));
                }
                l = rightL;
                r = rightR;
            } else {
                if (rightSize > 0) {
                    pool.submit(std::make_unique<QuickSortTask>(arr, pool, rightL, rightR, threshold));
                }
                l = leftL;
                r = leftR;
            }
        }
    }
};


#endif
