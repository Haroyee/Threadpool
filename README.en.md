# Thread Pool Library Documentation

## Project Overview

This is a C++ implemented thread pool library that supports task priority scheduling and dynamic thread management. Key features include:

- Supports both fixed and dynamic thread modes
- Three-level task priority (High, Normal, Low)
- Automatic thread expansion and contraction
- Task queue management
- Thread-safe task submission and execution

## Main Components

- `Threadpool`: Core thread pool class, manages threads and task queues
- `Task`: Task class, contains priority and executable function
- `Priority`: Enumeration defining task priorities
- `PoolMode`: Enumeration defining thread pool modes

## Usage Example

```cpp
#include "threadpool.h"

// Define a heavy computing task
long long heavy_task(int n) {
    long long sum = 0;
    for (int i = 0; i < n; ++i) {
        sum *= i;
    }
    return sum;
}

int main() {
    // Create thread pool
    Threadpool threadpool(2, 16);
    threadpool.setPoolMode(PoolMode::MODE_CACHED);
    threadpool.start();

    // Submit task
    auto result = threadpool.submitNormalTask(heavy_task, 1000000);

    // Wait for task completion
    result.get();

    // Shutdown thread pool
    threadpool.shutDown();

    return 0;
}
```

## API Documentation

### Thread Pool Operations

- `start()`: Start the thread pool
- `shutDown()`: Shutdown the thread pool
- `setPoolMode(PoolMode mode)`: Set thread pool mode
- `setInitThreadSize(size_t size)`: Set initial thread count
- `setThreadUpperThresh(size_t size)`: Set maximum thread count
- `setTaskUpperThresh(size_t size)`: Set maximum task count

### Task Submission

- `submit(Priority prio, F&& func, Args&&... args)`: Submit task with specified priority
- `submitLowTask(F&& func, Args&&... args)`: Submit low priority task
- `submitNormalTask(F&& func, Args&&... args)`: Submit normal priority task
- `submitHighTask(F&& func, Args&&... args)`: Submit high priority task

### Status Queries

- `getCurThdSize()`: Get current thread count
- `getCurTskSize()`: Get current task count

## Configuration Parameters

- `maxTaskSize`: Maximum task count (default: 32867)
- `destroyTime`: Idle thread destruction wait time (default: 30 seconds)
- `submitWaitTime`: Task submission wait time (default: 1 second)

## Compilation Requirements

- C++11 or higher
- Standard thread library support

## License

This project uses the MIT License. Please refer to the LICENSE file for details.
