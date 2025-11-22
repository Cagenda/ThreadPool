#include<ThreadPool.h>

ThreadPool::ThreadPool(int min, int max) :m_maxThread(max),m_minThread(min),m_stop(false),m_idleThread(min),m_curThread(min)
// 在构造函数里，主要对初始的成员进行初始化
{
    //创建管理者线程
    m_manager = std::thread(&ThreadPool::manager, this);

    //创建工作线程
    for (int i = 0; i < min;i++)
    {
        //这里的t仅仅只是临时变量
        std::thread t(&ThreadPool::worker, this);
        //把线程对象放在容器里
        m_workers.emplace_back(t);
    }
}

//添加任务函数  往任务队列中插入任务
void ThreadPool::addTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex>lock(m_queueMutex);//锁住任务队列
        m_tasks.emplace(task);
    }
    m_condition.notify_one();
}

void ThreadPool::worker()
{
    while (!m_stop)//判断线程池是否关闭，如果线程池开启，则一直运行
    {
        std::function<void()> task = nullptr;
        std::unique_lock<std::mutex> lock(m_queueMutex);
        while (m_tasks.empty())// 队列不空，才取任务,队列为空时阻塞
        {
            m_condition.wait(lock);
        }
    }
}
