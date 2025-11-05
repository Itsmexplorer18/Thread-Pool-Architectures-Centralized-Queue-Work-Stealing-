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

        std::cout << "[STEAL] Thread " << thiefID
                  << " stealing from Thread " << ownerID
                  << " (victim has " << tasks.size() << " tasks)\n";

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

            // Try to get a local task
            task = queues[id]->pop();

            // Try to steal from others
            if (!task) {
                for (int i = 0; i < numWorkers && !task; ++i) {
                    if (i != id)
                        task = queues[i]->steal(id);
                }
            }

            if (task) {
                activeWorkers.fetch_add(1);
                task->compute();
                activeWorkers.fetch_sub(1);
                globalCV.notify_all();
            } else {
                // No work, wait briefly
                std::unique_lock<std::mutex> lock(globalMtx);
                globalCV.wait_for(lock, std::chrono::microseconds(100));
            }
        }
    }

public:
    explicit ForkJoinPool(int threads = std::thread::hardware_concurrency())
        : numWorkers(threads) {
        for (int i = 0; i < numWorkers; ++i)
            queues.push_back(std::make_unique<WorkStealingDeque>(i));

        for (int i = 0; i < numWorkers; ++i)
            workers.emplace_back(&ForkJoinPool::workerThread, this, i);
    }

    ~ForkJoinPool() {
        shutdown.store(true);
        globalCV.notify_all();
        for (auto &worker : workers)
            if (worker.joinable())
                worker.join();
    }

    void submit(std::unique_ptr<RecursiveTask> task) {
        int id = workerID;
        if (id >= 0 && id < numWorkers)
            queues[id]->push(std::move(task));
        else {
            static thread_local std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<> dis(0, numWorkers - 1);
            queues[dis(gen)]->push(std::move(task));
        }
        globalCV.notify_one();
    }

    void invoke(std::unique_ptr<RecursiveTask> task) {
        submit(std::move(task));
        globalCV.notify_all();
        waitForCompletion();
    }

    void waitForCompletion() {
        std::unique_lock<std::mutex> lock(globalMtx);
        globalCV.wait(lock, [this] {
            bool allEmpty = true;
            for (const auto &q : queues) {
                if (!q->empty()) {
                    allEmpty = false;
                    break;
                }
            }
            return allEmpty && activeWorkers.load() == 0;
        });
    }
};

thread_local int ForkJoinPool::workerID = -1;

class QuickSortTask : public RecursiveTask {
private:
    std::vector<int> &arr;
    ForkJoinPool &pool;
    int left, right;
    const int threshold;

    int partition(int l, int r) {
        int pivot = arr[r];
        int i = l - 1;
        for (int j = l; j < r; ++j) {
            if (arr[j] <= pivot) {
                ++i;
                std::swap(arr[i], arr[j]);
            }
        }
        std::swap(arr[i + 1], arr[r]);
        return i + 1;
    }

    void sequentialSort(int l, int r) {
        if (l < r) {
            int pi = partition(l, r);
            sequentialSort(l, pi - 1);
            sequentialSort(pi + 1, r);
        }
    }

public:
    QuickSortTask(std::vector<int> &array, ForkJoinPool &poolRef, int l, int r, int thresh = 1000)
        : arr(array), pool(poolRef), left(l), right(r), threshold(thresh) {}

    bool shouldFork() const override { return (right - left) > threshold; }

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

std::vector<int> generateRandomArray(int size) {
    std::vector<int> arr(size);
    std::mt19937 gen(42);
    std::uniform_int_distribution<> dis(1, 1000000);
    for (int &val : arr)
        val = dis(gen);
    return arr;
}

bool isSorted(const std::vector<int> &arr) {
    for (size_t i = 1; i < arr.size(); ++i)
        if (arr[i - 1] > arr[i])
            return false;
    return true;
}

// ================= Main =================
int main() {
    const int SIZE = 10000000;
    std::cout << "Generating random array of size " << SIZE << "...\n";
    auto arr1 = generateRandomArray(SIZE);
    auto arr2 = arr1;

    // Parallel QuickSort
    std::cout << "\nParallel QuickSort with ForkJoinPool:\n";
    auto start = std::chrono::high_resolution_clock::now();

    ForkJoinPool pool(std::thread::hardware_concurrency());
    pool.invoke(std::make_unique<QuickSortTask>(arr1, pool, 0, arr1.size() - 1));

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time: " << duration.count() << " ms\n";
    std::cout << "Sorted correctly: " << (isSorted(arr1) ? "Yes" : "No") << "\n";

    // Sequential std::sort
    std::cout << "\nSequential std::sort:\n";
    start = std::chrono::high_resolution_clock::now();

    std::sort(arr2.begin(), arr2.end());

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time: " << duration.count() << " ms\n";
    std::cout << "Sorted correctly: " << (isSorted(arr2) ? "Yes" : "No") << "\n";

    return 0;
}
