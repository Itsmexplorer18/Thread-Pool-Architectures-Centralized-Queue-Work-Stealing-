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
