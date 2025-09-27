#include "../include/threadpool.h"
#include <iostream>

const size_t maxTaskSize = 1024;      // 最大任务数量
const size_t destroyTime = 60 * 1000; // 任务销毁等待时间(ms)
const size_t subTime = 5 * 1000;      // 提交失败等待时间(ms)

/*Task类*/
// 构造函数
Task::Task(Priority &p, std::function<void()> &f) : prio(p), func(f) {}
// 操作符号重构
bool Task::operator<(const Task &tsk) const // 大顶堆
{
    return static_cast<int>(this->prio) > static_cast<int>(tsk.prio);
}

Priority &Task::getPrio() // 返回优先级
{
    return prio;
};
const std::function<void()> &Task::getFunc() const
{
    return func;
}

/*threadpool类*/
// 构造函数
Threadpool::Threadpool(size_t initSize = std::thread::hardware_concurrency(), size_t upperSize = std::thread::hardware_concurrency() * 2)
    : initThreadSize(initSize),
      threadUpperThresh(upperSize),
      idleThreadSize(0),
      taskUpperThresh(maxTaskSize),
      poolMode(PoolMode::MODE_FIXED),
      stopFlag(false),
      destroyWaitTime(destroyTime),
      submitWaitTime(subTime)

{
}
Threadpool::~Threadpool()
{
    if (!stopFlag)
        stopFlag = true;
    this->threadCv.notify_all();

    for (auto i = this->threads.begin(); i != this->threads.end(); ++i)
    {
        if (i->second->joinable())
            i->second->join();
    }
}

void Threadpool::start() // 启动线程池
{
    for (size_t i = 0; i < initThreadSize; ++i)
        addThread();
}

size_t Threadpool::getCurThdSize() const // 获取当前线程数量
{
    return threads.size();
}

size_t Threadpool::getCurTskSize() const // 获取当前任务数量
{

    return tasks.size();
}

size_t Threadpool::getIdleThdSize() const // 获取空闲线程数量
{

    return this->idleThreadSize;
}
void Threadpool::shutDown() // 关闭线程
{
    std::lock_guard<std::mutex> lock(taskQueMtx);
    this->stopFlag = true;
    while (tasks.size())
        tasks.pop();
}
void Threadpool::setPoolMode(PoolMode mode) // 设置线程模式
{
    if (stopFlag)
        return;
    poolMode = mode;
}

void Threadpool::setInitThreadSize(const size_t &size) // 设置初始线程数量
{
    if (stopFlag)
        return;
    initThreadSize = size;
}
void Threadpool::setThreadUpperThresh(const size_t &size) // 设置cache模式下线程上限阈值
{
    if (stopFlag)
        return;
    threadUpperThresh = size;
}
void Threadpool::setTaskUpperThresh(const size_t &size) // 设置任务上限阀值
{
    if (stopFlag)
        return;
    taskUpperThresh = size;
}
void Threadpool::setDestroyWaitTime(const size_t &time) // 空闲线程销毁等待时间
{
    if (stopFlag)
        return;
    destroyWaitTime = std::chrono::milliseconds(time);
}
void Threadpool::setSubmitWaitTime(const size_t &time) // 设置提交等待时间
{
    if (stopFlag)
        return;
    submitWaitTime = std::chrono::milliseconds(time);
}

void Threadpool::addThread() // 增加线程函数，在非start()函数中需要在锁中
{
    ++idleThreadSize;
    std::unique_ptr<std::thread> t = std::make_unique<std::thread>(&Threadpool::workThread, this);
    threads.emplace(t->get_id(), std::move(t));
}
void Threadpool::workThread() // 线程函数
{

    while (true)
    {
        std::unique_lock<std::mutex> lock(taskQueMtx);
        if (poolMode == PoolMode::MODE_CACHED && tasks.empty() && getCurThdSize() > initThreadSize && idleThreadSize > 0 || stopFlag)
        {
            if (!threadCv.wait_for(lock, destroyWaitTime, [this]() -> bool
                                   { return !this->tasks.empty() || this->stopFlag; }) ||
                stopFlag)
            {
                --idleThreadSize;
                threads.erase(std::this_thread::get_id());
                return;
            }
        }
        else
        {
            threadCv.wait(lock, [this]() -> bool
                          { return !this->tasks.empty() || this->stopFlag; });
        }
        --idleThreadSize;
        if (tasks.empty())
        {
            ++idleThreadSize;
            continue;
        }

        std::function<void()> taskFunc = tasks.top().getFunc();
        tasks.pop();
        lock.unlock();
        taskFunc();
        lock.lock();
        ++idleThreadSize;
        lock.unlock();
        taskCv.notify_one();
    }
}
