
#include "ForkJoin.hpp"
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>
#include <iomanip>
#include <numeric>

using namespace std;
vector<int> generateRandomArray(int size, int seed) {
    vector<int> arr(size);
    mt19937 gen(seed);
    uniform_int_distribution<> dis(1, 1000000);
    for (int &val : arr) {
        val = dis(gen);
    }
    return arr;
}
bool isSorted(const vector<int> &arr) {
    return is_sorted(arr.begin(), arr.end());
}
void quicksort(vector<int>& arr, int left, int right) {
    if (left >= right) return;
    int pivot = arr[left + (right - left) / 2];
    int i = left, j = right;
    while (i <= j) {
        while (arr[i] < pivot) i++;
        while (arr[j] > pivot) j--;
        
        if (i <= j) {
            swap(arr[i], arr[j]);
            i++;
            j--;
        }
    }
    
    if (left < j) quicksort(arr, left, j);
    if (i < right) quicksort(arr, i, right);
}

// Task function that performs quicksort on a vector
vector<int> sortTask(int taskId, int size, const vector<int>& arr) {
    vector<int> copy = arr;
    quicksort(copy, 0, copy.size() - 1);
    return copy;
}

int main() {
    const int SIZE = 10'000'000;
    const int NUM_TASKS = 1;
    const int NUM_THREADS = 8;
    const int NUM_RUNS = 50;  // Number of runs for averaging
        cout << "Array size: " << SIZE << endl;
    cout << "Number of threads: " << NUM_THREADS << endl;
    cout << "Number of runs: " << NUM_RUNS << endl;

    vector<long long> seq_times, tp_times, fj_times;

    // Run multiple times
    for (int run = 0; run < NUM_RUNS; run++) {
        cout << "Run " << (run + 1) << "/" << NUM_RUNS << "..." << flush;
        
        // Generate test data with different seed each run
        auto originalArray = generateRandomArray(SIZE, 42 + run);

        // Sequential
        {
            auto arr = originalArray;
            auto start = chrono::high_resolution_clock::now();
            quicksort(arr, 0, arr.size() - 1);
            auto end = chrono::high_resolution_clock::now();
            seq_times.push_back(chrono::duration_cast<chrono::milliseconds>(end - start).count());
        }

        // ForkJoinPool
        {
            auto arr = originalArray;
            auto start = chrono::high_resolution_clock::now();
            {
                ForkJoinPool fjPool(NUM_THREADS);
                fjPool.invoke(std::make_unique<QuickSortTask>(arr, fjPool, 0, static_cast<int>(arr.size()) - 1));
            }
            auto end = chrono::high_resolution_clock::now();
            fj_times.push_back(chrono::duration_cast<chrono::milliseconds>(end - start).count());
        }
        
        cout << " Done\n";
    }

    // Calculate averages
    double avg_seq = accumulate(seq_times.begin(), seq_times.end(), 0.0) / NUM_RUNS;
    double avg_fj = accumulate(fj_times.begin(), fj_times.end(), 0.0) / NUM_RUNS;
    // Results
    cout << "AVERAGE RESULTS (over " << NUM_RUNS << " runs)\n";
    cout << "Sequential:       " << fixed << setprecision(2) << avg_seq << " ms\n";
    //cout << "ThreadPool:       " << avg_tp << " ms  (speedup: " 
      //   << (avg_seq / avg_tp) << "x)\n";
    cout << "ForkJoinPool:     " << avg_fj << " ms  (speedup: " 
         << (avg_seq / avg_fj) << "x)\n";
    return 0;
}
