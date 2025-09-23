#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>

enum class PoolMode // 线程池模式
{
    MODE_FIXED,  // 固定数量的线程
    MODE_CACHED, // 线程数量可动态增长（cache模式）
};

enum class Priority // 任务优先级
{
    HIGH = 2,   // 高优先级
    NORMAL = 1, // 普通优先级
    LOW = 0     // 低优先级
};

class Task // 包装任务
{
public:
    Task(Priority &p, std::function<void()> &f);
    ~Task() = default;
    /*重载运算符，用于堆的排序*/
    bool operator<(const Task &tsk) const; // 小于返回true，优先级更高，但是大顶堆
    bool operator>(const Task &tsk) const; // 大于返回true，优先级更高，但是小顶堆

private:
    Priority prio;              // 任务优先级
    std::function<void()> func; // 用于接受std::package_task类型，std::package_task是一个无返回值无参对象
};

class Threadpool
{
public:
    Threadpool(int initSize = std::thread::hardware_concurrency(), int upperSize = std::thread::hardware_concurrency() * 2);
    ~Threadpool();
    // 拷贝函数与拷贝赋值函数删除
    Threadpool(const Threadpool &t) = delete;
    Threadpool &operator=(const Threadpool &t) = delete;

    int getCurThdSize() const;       // 获取当前线程数量
    int getIdleThdSize() const;      // 获取当前空闲线程数量
    int getCurTskSize() const;       // 获取当前任务数量
    void setPoolMode(PoolMode mode); // 设置线程模式
    void shutDown();                 // 关闭线程池

    template <typename F, typename... Args>
    auto submit(Priority prio = Priority::NORMAL, F &&func, Args &&...args) // 带优先级提交
        -> std::future<typename std::result_of_t<F(Args...)>>
    {
        std::unique_lock<mutex> lock(taskQueMtx);
        this->taskCv.
    };

    template <typename F, typename... Args>
    auto submitLowTsk(F &&func, Args &&...args) // 低优先级提交任务
        -> std::future<typename std::result_of_t<F(Args...)>>;

    template <typename F, typename... Args>
    auto submitNormalTsk(F &&func, Args &&...args) // 普通优先级提交任务
        -> std::future<typename std::result_of_t<F(Args...)>>;

    template <typename F, typename... Args>
    auto submitHighTsk(F &&func, Args &&...args) // 高优先级提交任务
        -> std::future<typename std::result_of_t<F(Args...)>>;

private:
    void workThread();

private:
    /*线程相关变量*/
    std::unordered_map<int, std::unique_ptr<std::thread>> threads; // 线程哈希表
    int initThreadSize;                                            // 初始线程数量
    int threadUpperThresh;                                         // cache模式下线程上限阈值
    std::atomic<int> curThreadSize;                                // 当前线程数量
    std::atomic<int> idleThreadSize;                               // 当前空闲线程数量

    /*任务相关变量*/
    std::priority_queue<Task> tasks; // 任务队列，默认大顶堆
    int taskUpperThresh;             // 任务上限阀值
    std::atomic<int> curTaskSize;    // 当前任务数量

    /*线程互斥锁与条件变量*/
    std::mutex taskQueMtx;            // 保证任务队列的线程安全
    std::condition_variable taskCv;   // 任务条件变量
    std::condition_variable threadCv; // 线程条件变量

    PoolMode poolMode;      // 当前线程池的工作模式
    std::atomic<bool> stop; // 线程池停止标志
};

#endif