#include "../include/threadpool.h"
#include <iostream>
#include <iomanip> //输出格式化

const size_t maxTaskSize = 32867;     // 最大任务数量
const size_t destroyTime = 30 * 1000; // 任务销毁等待时间(ms)
const size_t subTime = 1 * 1000;      // 提交失败等待时间(ms)

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
    addThreadCv.notify_all();
    subThreadCv.notify_all();
    executeTaskCv.notify_all();

    if (addThreadManager->joinable())
        addThreadManager->join();
    if (subThreadManager->joinable())
        subThreadManager->join();

    for (auto i = this->threads.begin(); i != this->threads.end(); ++i)
    {
        if (i->second.joinable())
            i->second.join();
    }
}

void Threadpool::start() // 启动线程池
{
    for (size_t i = 0; i < initThreadSize; ++i)
        addThread();
    if (poolMode == PoolMode::MODE_CACHED)
    {
        addThreadManager = std::unique_ptr<std::thread>(new std::thread(&Threadpool::addManagerThread, this));
        subThreadManager = std::unique_ptr<std::thread>(new std::thread(&Threadpool::subManagerThread, this));
    }
}

const size_t Threadpool::getCurThdSize() const // 获取当前线程数量
{
    return threads.size();
}

const size_t Threadpool::getCurTskSize() const // 获取当前任务数量
{

    return tasks.size();
}

const size_t Threadpool::getIdleThdSize() const // 获取空闲线程数量
{

    return this->idleThreadSize;
}
void Threadpool::shutDown() // 关闭线程
{
    std::lock_guard<std::mutex> lock(mtx);
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

void Threadpool::setInitThreadSize(size_t &size) // 设置初始线程数量
{
    if (stopFlag)
        return;
    initThreadSize = size;
}
void Threadpool::setThreadUpperThresh(size_t &size) // 设置cache模式下线程上限阈值
{
    if (stopFlag)
        return;
    threadUpperThresh = size;
}
void Threadpool::setTaskUpperThresh(size_t &size) // 设置任务上限阀值
{
    if (stopFlag)
        return;
    taskUpperThresh = size;
}
void Threadpool::setDestroyWaitTime(size_t &time) // 空闲线程销毁等待时间
{
    if (stopFlag)
        return;
    destroyWaitTime = std::chrono::milliseconds(time);
}
void Threadpool::setSubmitWaitTime(size_t &time) // 设置提交等待时间
{
    if (stopFlag)
        return;
    submitWaitTime = std::chrono::milliseconds(time);
}

void Threadpool::addThread() // 增加线程函数，需在锁中运行
{
    if (getCurThdSize() < threadUpperThresh)
    {
        std::thread t(&Threadpool::workThread, this);
        std::thread::id thread_id = t.get_id();
        threads.emplace(thread_id, std::move(t));
        std::cout << "新增线程 : " << std::setw(2) << thread_id << " 当前线程数 : " << std::setw(2) << getCurThdSize() << std::endl;
    }
}

void Threadpool::subThread() // 销毁线程函数，需在锁中运行
{
    if (getCurThdSize() - idleThreadSize >= initThreadSize) // 注意判断条件为>=
    {
        for (auto id : idleThreadId)
        {
            auto &t = threads.at(id);
            if (t.joinable())
                t.join();
            threads.erase(id);

            std::cout << "销毁线程 : " << std::setw(2) << id << " 当前线程数 : " << std::setw(2) << getCurThdSize() << std::endl;
        }
        idleThreadId.clear();
    }
}
void Threadpool::workThread() // 线程函数
{

    while (!stopFlag)
    {
        std::function<void()> taskFunc;
        {
            std::unique_lock<std::mutex> lock(mtx);

            bool flag = executeTaskCv.wait_for(lock, destroyWaitTime, [this]() -> bool
                                               { return !(this->tasks.empty()) || this->stopFlag; }); // 超时标志
            bool destoryFlag = !flag && (getCurThdSize() - idleThreadSize) > initThreadSize && tasks.empty();

            if ((poolMode == PoolMode::MODE_CACHED && destoryFlag) || stopFlag)
            {
                idleThreadId.emplace_back(std::this_thread::get_id()); // 将空闲线程id入队
                ++idleThreadSize;
                lock.unlock();
                subThreadCv.notify_one();
                return;
            }
            if (tasks.empty())
            {
                lock.unlock();
                continue;
            }
            else
            {
                taskFunc = tasks.top().getFunc();
                tasks.pop();
            }
        }
        if (taskFunc)
            taskFunc();
        submitTaskCv.notify_one();
        subThreadCv.notify_one();
    }
}

void Threadpool::addManagerThread() // 新增线程的线程管理函数
{
    while (!stopFlag)
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            addThreadCv.wait(lock, [this]()
                             { return this->getCurThdSize() < this->threadUpperThresh || stopFlag; });
            if (stopFlag)
                return;
            addThread();
        }
        executeTaskCv.notify_one();
    }
}
void Threadpool::subManagerThread() // 销毁线程的线程管理函数
{
    while (!stopFlag)
    {
        {
            std::unique_lock<std::mutex> lock(mtx);
            subThreadCv.wait(lock, [this]()
                             { return this->tasks.empty() || stopFlag; });

            if (stopFlag)
                return;
            subThread();
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}
