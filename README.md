# 线程池库文档

## 项目简介
这是一个C++实现的线程池库，支持任务优先级调度和动态线程管理。主要特性包括：
- 支持固定和动态两种线程模式
- 三级任务优先级（高、中、低）
- 自动线程扩展和收缩
- 任务队列管理
- 线程安全的任务提交和执行

## 主要组件
- `Threadpool`：核心线程池类，管理线程和任务队列
- `Task`：任务类，包含优先级和可执行函数
- `Priority`：枚举定义任务优先级
- `PoolMode`：枚举定义线程池模式

## 使用示例
```cpp
#include "threadpool.h"

// 定义一个耗时任务
long long heavy_task(int n) {
    long long sum = 0;
    for (int i = 0; i < n; ++i) {
        sum *= i;
    }
    return sum;
}

int main() {
    // 创建线程池
    Threadpool threadpool(2, 16);
    threadpool.setPoolMode(PoolMode::MODE_CACHED);
    threadpool.start();

    // 提交任务
    auto result = threadpool.submitNormalTsk(heavy_task, 1000000);
    
    // 等待任务完成
    result.get();
    
    // 关闭线程池
    threadpool.shutDown();
    
    return 0;
}
```

## API 文档

### 线程池操作
- `start()`: 启动线程池
- `shutDown()`: 关闭线程池
- `setPoolMode(PoolMode mode)`: 设置线程池模式
- `setInitThreadSize(size_t size)`: 设置初始线程数量
- `setThreadUpperThresh(size_t size)`: 设置最大线程数量
- `setTaskUpperThresh(size_t size)`: 设置最大任务数量

### 任务提交
- `submit(Priority prio, F&& func, Args&&... args)`: 提交指定优先级的任务
- `submitLowTsk(F&& func, Args&&... args)`: 提交低优先级任务
- `submitNormalTsk(F&& func, Args&&... args)`: 提交普通优先级任务
- `submitHighTsk(F&& func, Args&&... args)`: 提交高优先级任务

### 状态查询
- `getCurThdSize()`: 获取当前线程数量
- `getCurTskSize()`: 获取当前任务数量

## 配置参数
- `maxTaskSize`: 最大任务数量（默认：32867）
- `destroyTime`: 空闲线程销毁等待时间（默认：30秒）
- `submitWaitTime`: 任务提交等待时间（默认：1秒）

## 编译要求
- C++17或更高版本
- 支持标准线程库

## 许可证
本项目使用MIT许可证，请参阅LICENSE文件获取详细信息。