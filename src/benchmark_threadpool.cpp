#include "benchmark.hpp"

// --- Sample Workloads Implementation ---
int tinyTask(int a) {
    this_thread::sleep_for(chrono::microseconds(100));
    return a * a;
}

int verySmallTask(int a) {
    this_thread::sleep_for(chrono::microseconds(500));
    return a * a;
}

int lightTask(int a) {
    this_thread::sleep_for(chrono::milliseconds(5));
    return a * a;
}

int mediumTask(int a) {
    this_thread::sleep_for(chrono::milliseconds(20));
    int result = 0;
    for (int i = 0; i < 1000; i++) result += i;
    return a * a + result;
}

int heavyTask(int a) {
    this_thread::sleep_for(chrono::milliseconds(50));
    int result = 0;
    for (int i = 0; i < 10000; i++) result += i;
    return a * a + result;
}

// --- Benchmark Implementation ---
BenchmarkResult runBenchmark(const BenchmarkConfig& config) {
    double totalSeqTime = 0;
    double totalPoolTime = 0;

    for (int run = 0; run < config.iterations; run++) {
        // Sequential execution
        auto start_seq = chrono::steady_clock::now();
        for (int i = 0; i < config.numTasks; i++) {
            config.workload(i);
        }
        auto end_seq = chrono::steady_clock::now();
        auto duration_seq = chrono::duration_cast<chrono::microseconds>(end_seq - start_seq).count();

        // Thread pool execution
        ThreadPool pool(config.numThreads);
        vector<future<int>> results;
        auto start_pool = chrono::steady_clock::now();
        for (int i = 0; i < config.numTasks; i++) {
            results.push_back(pool.executetasks(config.workload, i));
        }
        for (auto &res : results) res.get();
        auto end_pool = chrono::steady_clock::now();
        auto duration_pool = chrono::duration_cast<chrono::microseconds>(end_pool - start_pool).count();

        totalSeqTime += duration_seq;
        totalPoolTime += duration_pool;
    }

    double avgSeqTime = totalSeqTime / config.iterations / 1000.0;
    double avgPoolTime = totalPoolTime / config.iterations / 1000.0;
    double speedup = avgSeqTime / avgPoolTime;
    double speedupPercent = (speedup - 1.0) * 100.0;
    double efficiency = min((speedup / config.numThreads) * 100.0, 100.0);

    return {
        config.name,
        config.numTasks,
        config.numThreads,
        avgSeqTime,
        avgPoolTime,
        speedupPercent,
        efficiency
    };
}

void printResults(const vector<BenchmarkResult>& results) {
    cout << "\n" << string(100, '=') << endl;
    cout << "THREAD POOL BENCHMARK RESULTS" << endl;
    cout << string(100, '=') << endl;

    for (const auto& r : results) {
        cout << "\nScenario: " << r.scenarioName << endl;
        cout << string(50, '-') << endl;
        cout << "Configuration: " << r.numTasks << " tasks, " << r.numThreads << " threads" << endl;
        cout << fixed << setprecision(2);
        cout << "Sequential Time:   " << setw(8) << r.avgSeqTime << " ms" << endl;
        cout << "Thread Pool Time:  " << setw(8) << r.avgPoolTime << " ms" << endl;
        cout << "Speedup:           " << setw(8) << r.speedupPercent << " %" << endl;
        cout << "Thread Efficiency: " << setw(8) << r.efficiency << " %" << endl;
    }

    cout << "\n" << string(100, '=') << endl;
    cout << string(100, '=') << endl;
    
    for (const auto& r : results) {
        cout << "• " << r.scenarioName << ": Achieved " 
             << fixed << setprecision(1) << r.speedupPercent 
             << "% performance improvement with " << r.numThreads 
             << "-thread pool (" << fixed << setprecision(0) << r.efficiency 
             << "% efficiency)" << endl;
    }
    cout << endl;
}
