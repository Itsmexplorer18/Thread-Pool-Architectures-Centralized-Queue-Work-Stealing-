#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include "threadpool.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include <functional>

using namespace std;

// --- Sample Workloads ---
int tinyTask(int a);
int verySmallTask(int a);
int lightTask(int a);
int mediumTask(int a);
int heavyTask(int a);

// --- Benchmark Structures ---
struct BenchmarkConfig {
    string name;
    int numTasks;
    int numThreads;
    function<int(int)> workload;
    int iterations;
};

struct BenchmarkResult {
    string scenarioName;
    int numTasks;
    int numThreads;
    double avgSeqTime;
    double avgPoolTime;
    double speedupPercent;
    double efficiency;
};

// --- Benchmark Functions ---
BenchmarkResult runBenchmark(const BenchmarkConfig& config);
void printResults(const vector<BenchmarkResult>& results);

#endif // BENCHMARK_HPP
