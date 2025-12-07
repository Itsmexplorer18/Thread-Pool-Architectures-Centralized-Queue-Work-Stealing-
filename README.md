# Thread-Pool-Architectures And Centralized-Queue-Work-Stealing

##  Components

### 1. ThreadPool
- thread pool for task parallelism
- Best for: Independent tasks, I/O operations, variable workloads
- **Performance**: Up to 8X speedup on 8 cores

### 2. ForkJoinPool : Workstealing
- Divide-and-conquer parallel execution framework
- Best for: Recursive algorithms, merge sort, tree operations
- **Performance**: Up to 2X speedup on recursive workloads

##  Quick Comparison

| Feature | ThreadPool | ForkJoin |
|---------|-----------|----------|
| Use Case | Independent tasks | Recursive divide-and-conquer |
| Overhead | Low | Medium (task spawning) |
| Best For | I/O, variable tasks | CPU-intensive recursion |
| Examples | Image processing | Merge sort, tree traversal |


## When to Use What?
**Use ThreadPool when:**
- Tasks are independent
- Variable execution times
- I/O-bound operations

**Use ForkJoin when:**
- Recursive algorithms
- Divide-and-conquer problems
- ## to-do-optimizations and theoritical understanding
- false sharing
- child stealing vs continuation stealing
- lock free implementation 


## Requirements

- C++17 or later
- pthread support
