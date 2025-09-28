#include "../include/threadpool.h"
#include <iostream>

// 一个耗时更长的任务
long long heavy_task(int n)
{
    long long sum = 0;
    // 关键：用固定次数的循环放大任务耗时，避免溢出/编译器优化导致的“耗时不准”
    for (int i = 0; i < n; ++i)
    {
        sum *= i; // 简单计算，避免编译器优化掉循环
    }
    return sum;
}

int main()
{
    std::vector<std::future<long long>> vr;
    Threadpool threadpool(2, 16);
    threadpool.setPoolMode(PoolMode::MODE_CACHED);
    threadpool.start();

    auto start = std::chrono::system_clock::now();
    /*第一次任务提交*/
    std::cout << "第一次任务提交 : " << std::endl;

    // 提交1000个“大”任务
    for (int i = 0; i < 10000; ++i)
    {
        vr.push_back(threadpool.submitNormalTsk(heavy_task, 1000000)); // 计算一个大数的阶乘
    }

    // 等待所有任务完成
    for (auto &fut : vr)
    {
        fut.get();
    }
    vr.clear();
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "线程池总耗时: " << elapsed_seconds.count() << "s\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(60 * 1000));

    /*第二次任务提交*/
    start = std::chrono::system_clock::now();
    std::cout << "第二次任务提交 : " << std::endl;
    for (int i = 0; i < 10000; ++i)
    {
        vr.push_back(threadpool.submitNormalTsk(heavy_task, 1000000)); // 计算一个大数的阶乘
    }

    // 等待所有任务完成
    for (auto &fut : vr)
    {
        fut.get();
    }
    end = std::chrono::system_clock::now();
    elapsed_seconds = end - start;

    std::cout << "线程池总耗时: " << elapsed_seconds.count() << "s\n";

    threadpool.shutDown();

    /*--- 对比单线程 ---*/
    start = std::chrono::system_clock::now();
    for (int i = 0; i < 10000; ++i)
    {
        heavy_task(1000000);
    }
    end = std::chrono::system_clock::now();
    elapsed_seconds = end - start;
    std::cout << "单线程总耗时: " << elapsed_seconds.count() << "s\n";

    return 0;
}