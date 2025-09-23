#include "../include/threadpool.h"
#include <iostream>

const int maxTaskSize = 1024;

/*Task类*/
// 构造函数
Task::Task(Priority &p, std::function<void()> &f) : prio(p), func(f) {}
// 操作符号重构
bool Task::operator<(const Task &tsk) const // 大顶堆
{
    return static_cast<int>(this->prio) < static_cast<int>(tsk.prio);
}
bool Task::operator>(const Task &tsk) const // 小顶堆
{
    return static_cast<int>(this->prio) > static_cast<int>(tsk.prio);
}

/*threadpool类*/
// 构造函数
Threadpool::Threadpool(int initSize = std::thread::hardware_concurrency(), int upperSize = std::thread::hardware_concurrency() * 2)
    : initThreadSize(initSize),
      threadUpperThresh(upperSize),
      curThreadSize(0),
      idleThreadSize(0),
      curTaskSize(0),
      taskUpperThresh(maxTaskSize),
      poolMode(PoolMode::MODE_FIXED),
      stop(false)
{
    for (int i = 0; i < initSize; ++i)
    {
        ++idleThreadSize;
        std::unique_ptr<std::thread> t = std::make_unique<std::thread>(workThread);
        tasks.emplace(t->get_id(), t);
        ++curThreadSize;
    }
}
Threadpool::~Threadpool()
{
    if (!stop)
        stop = true;
    this->threadCv.notify_all();
    for (auto i = this->threads.begin(); i != this->threads.end(); ++i)
    {
        if (i->second->joinable())
            i->second->join();
    }
}
int Threadpool::getCurThdSize() const
{
    return this->curThreadSize;
}

int Threadpool::getCurTskSize() const
{
    return this->curTaskSize;
}

int Threadpool::getIdleThdSize() const
{
    return this->idleThreadSize;
}
void Threadpool::shutDown()
{
    this->stop = true;
}
void Threadpool::setPoolMode(PoolMode mode)
{
    if (!stop)
        return;
    this->poolMode = mode;
}
