
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>
#include <future>
#include <chrono>

using namespace std;

class ThreadPool {
private:
    int m_threads;
    vector<thread> threads;
    queue<function<void()>> tasks;
    mutex mtx;
    condition_variable cv;
    bool stop;

public:
    explicit ThreadPool(int numthreads) : m_threads{numthreads}, stop{false} {
        for (int i = 0; i < m_threads; i++) {
            threads.emplace_back([this]() { //lambda function for thread-->each thread executes the follwing lambda function
                while (true) { //keep the thread running forever 
                    function<void()> currtask;
                    unique_lock<mutex> lock(mtx);
                    cv.wait(lock, [this] {
                        return !tasks.empty() || stop;
                    });
                    if (stop) return;
                    currtask = move(tasks.front());
                    tasks.pop();
                    lock.unlock(); // Unlock before running the task to allow other threads to pick tasks
                    currtask();
                }
            });
        }
    }

    ~ThreadPool() {
        {
            unique_lock<mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();
        for (auto &th : threads) {
            th.join();
        }
    }

    template<class F, class... Args> //any callable dunction with any number of argument
    auto executetasks(F&& f, Args&&... args) -> future<decltype(f(args...))> {//1 & 2
        using return_type = decltype(f(args...));
        auto task = make_shared<packaged_task<return_type()>>(bind(forward<F>(f), forward<Args>(args)...)); //3
        future<return_type> res = task->get_future();
        {
            unique_lock<mutex> lock(mtx);
            tasks.emplace([task]() {
                (*task)();
            });
        }
        cv.notify_one();
        return res;
    }
       //INSTEAD OF BIND ITS BETTER TO USE LAMDA ESPECIALLY N MORDERN C++
     /*  template<class F, class... Args>
       auto executetasks(F&& f, Args&&... args) -> future<decltype(f(args...))> {
           using return_type = decltype(f(args...));
           auto task = make_shared<packaged_task<return_type()>>(
               [func = forward<F>(f), tup = make_tuple(forward<Args>(args)...)]() mutable {
                   return apply(func, tup);
               });
       
           future<return_type> res = task->get_future();
           {
               unique_lock<mutex> lock(mtx);
               tasks.emplace([task]() { (*task)(); });
           }
           cv.notify_one();
           return res;
       }
       */

};

// Function that takes time to execute
int func(int a) {
    this_thread::sleep_for(chrono::seconds(1));
    cout << "Task executed by thread ID: " << this_thread::get_id() << endl;
    return a * a;
}

int main() {
    int numTasks =8;
    cout << "Number of tasks: " << numTasks << endl;
    
    // Sequential execution without thread pool
    auto start_seq = chrono::steady_clock::now();
    for (int i = 0; i < numTasks; i++) {
        func(i);
    }
    auto end_seq = chrono::steady_clock::now();
    cout << "Time taken without thread pool: " 
         << chrono::duration_cast<chrono::milliseconds>(end_seq - start_seq).count() 
         << " milliseconds" << endl;

    // Execution using thread pool
    ThreadPool pool(4);  // Using 4 threads
    vector<future<int>> results;

    auto start_pool = chrono::steady_clock::now();
    for (int i = 0; i < numTasks; i++) {
        results.push_back(pool.executetasks(func, i));
    }
    
    for (auto &res : results) {
        res.get();  // Wait for all tasks to complete
    }
    
    auto end_pool = chrono::steady_clock::now();
    cout << "Time taken with thread pool: " 
         << chrono::duration_cast<chrono::milliseconds>(end_pool - start_pool).count() 
         << " milliseconds" << endl;

    return 0;
}
