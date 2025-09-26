#include "../include/threadpool.h"
#include <iostream>

const size_t maxTaskSize = 1024;

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
      stopFlag(false)

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

void Threadpool::start()
{
    for (size_t i = 0; i < initThreadSize; ++i)
        addThread();
}

size_t Threadpool::getCurThdSize() const
{
    return threads.size();
}

size_t Threadpool::getCurTskSize() const
{

    return tasks.size();
}

size_t Threadpool::getIdleThdSize() const
{

    return this->idleThreadSize;
}
void Threadpool::shutDown()
{
    std::lock_guard<std::mutex> lock(taskQueMtx);
    this->stopFlag = true;
    while (tasks.size())
        tasks.pop();
}
void Threadpool::setPoolMode(PoolMode mode)
{
    if (stopFlag)
        return;
    this->poolMode = mode;
}

void Threadpool::addThread()
{
    ++idleThreadSize;
    std::unique_ptr<std::thread> t = std::make_unique<std::thread>(&Threadpool::workThread, this);
    threads.emplace(t->get_id(), std::move(t));
}
void Threadpool::workThread()
{

    while (true)
    {
        std::unique_lock<std::mutex> lock(taskQueMtx);
        if (poolMode == PoolMode::MODE_CACHED && tasks.empty() && getCurThdSize() > initThreadSize && idleThreadSize > 0 || stopFlag)
        {
            if (!threadCv.wait_for(lock, std::chrono::seconds(60), [this]() -> bool
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
                          { return !tasks.empty() || stopFlag; });
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
