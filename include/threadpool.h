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
#include <type_traits>

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
    Priority &getPrio();
    const std::function<void()> &getFunc() const;

private:
    Priority prio;              // 任务优先级
    std::function<void()> func; // 用于接受std::package_task类型，std::package_task是一个无返回值无参对象
};

class Threadpool
{
public:
    Threadpool(size_t, size_t);
    ~Threadpool();
    // 拷贝函数与拷贝赋值函数删除
    Threadpool(const Threadpool &t) = delete;
    Threadpool &operator=(const Threadpool &t) = delete;

    void start(); // 启动线程

    size_t getCurThdSize() const;    // 获取当前线程数量
    size_t getIdleThdSize() const;   // 获取当前空闲线程数量
    size_t getCurTskSize() const;    // 获取当前任务数量
    void setPoolMode(PoolMode mode); // 设置线程模式
    void shutDown();                 // 关闭线程池

    template <typename F, typename... Args>
    auto submit(Priority prio, F &&func, Args &&...args)
        -> std::future<decltype(std::declval<F>()(std::declval<Args>()...))>
    {
        // 定义返回类型别名
        using ReturnType = decltype(std::declval<F>()(std::declval<Args>()...));

        // 包装任务
        // 将函数与参数用bind绑定
        auto boundFunc = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
        // 将package_task包装进shared_ptr,便于转换为function<void()>进行捕获，function类型需要函数可拷贝
        std::shared_ptr<std::packaged_task<ReturnType()>> taskPackage = std::make_shared<std::packaged_task<ReturnType()>>(boundFunc);
        // 获取future
        std::future<ReturnType> resultFuture = taskPackage->get_future();
        // 转换为function<void()>
        std::function<void()> taskFunc = [taskPackage]()
        { (*taskPackage)(); };

        // 任务入队
        std::unique_lock<std::mutex>
            lock(taskQueMtx);
        // 等待超过1s视为提交失败
        if (!this->taskCv.wait_for(lock, std::chrono::seconds(1), [this]() -> bool
                                   { return this->tasks.size() < this->taskUpperThresh && !this->stopFlag; }))
            throw std::runtime_error("Task submission failed, Please try again later.");

        tasks.emplace(prio, taskFunc); // 入队
        // 若线程池处于cache模式,且没有空闲线程，任务队列有任务，当前线程量小于最大线程数时，增加新线程
        if (poolMode == PoolMode::MODE_CACHED && idleThreadSize == 0 && 0 < getCurTskSize() && getCurThdSize() < threadUpperThresh)
            addThread();

        lock.unlock();
        threadCv.notify_one();

        return resultFuture;
    }

    template <typename F, typename... Args>
    auto submitLowTsk(F &&func, Args &&...args) // 低优先级提交任务
        -> std::future<decltype(std::declval<F>()(std::declval<Args>()...))>
    {
        return submit(Priority::LOW, func, args...);
    }

    template <typename F, typename... Args>
    auto submitNormalTsk(F &&func, Args &&...args) // 普通优先级提交任务
        -> std::future<decltype(std::declval<F>()(std::declval<Args>()...))>
    {
        return submit(Priority::NORMAL, func, args...);
    }

    template <typename F, typename... Args>
    auto submitHighTsk(F &&func, Args &&...args) // 高优先级提交任务
        -> std::future<decltype(std::declval<F>()(std::declval<Args>()...))>
    {
        return submit(Priority::HIGH, func, args...);
    }

private:
    void workThread();
    void addThread();

private:
    /*线程相关变量*/
    std::unordered_map<std::thread::id, std::unique_ptr<std::thread>> threads; // 线程哈希表
    size_t initThreadSize;                                                     // 初始线程数量
    size_t threadUpperThresh;                                                  // cache模式下线程上限阈值
    std::atomic<size_t> idleThreadSize;                                        // 当前空闲线程数量

    /*任务相关变量*/
    std::priority_queue<Task> tasks; // 任务队列，默认大顶堆
    size_t taskUpperThresh;          // 任务上限阀值

    /*线程互斥锁与条件变量*/
    mutable std::mutex taskQueMtx;    // 保证任务队列的线程安全,mutable确保可以在const内修改
    std::condition_variable taskCv;   // 任务条件变量
    std::condition_variable threadCv; // 线程条件变量

    PoolMode poolMode;          // 当前线程池的工作模式
    std::atomic<bool> stopFlag; // 线程池停止标志
};

#endif