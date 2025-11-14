//#include "threadpool.hpp"
#include "benchmark.hpp"


int main() {
    const int ITERATIONS = 100;
    vector<BenchmarkConfig> benchmarks = {
        {"Tiny Workload (0.1ms) - 4 threads", 16, 4, tinyTask, ITERATIONS},
        
        {"Very Small Workload (0.5ms) - 4 threads", 16, 4, verySmallTask, ITERATIONS},
        
        {"Light Workload (5ms) - 4 threads", 16, 4, lightTask, ITERATIONS},
        {"Light Workload (5ms) - 8 threads", 32, 8, lightTask, ITERATIONS},
        
        {"Medium Workload (20ms) - 4 threads", 16, 4, mediumTask, ITERATIONS},
        {"Medium Workload (20ms) - 8 threads", 32, 8, mediumTask, ITERATIONS},
        
        {"Heavy Workload (50ms) - 4 threads", 16, 4, heavyTask, ITERATIONS},
        {"Heavy Workload (50ms) - 8 threads", 32, 8, heavyTask, ITERATIONS},
        
        {"High Task Count (64 tasks) - 8 threads", 64, 8, mediumTask, ITERATIONS},
    };

    vector<BenchmarkResult> results;
    for (const auto& config : benchmarks) {
        cout << "Testing: " << config.name << "..." << endl;
        results.push_back(runBenchmark(config));
    }

   printResults(results);

    return 0;
}
