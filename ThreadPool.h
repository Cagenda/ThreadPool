#include <stdio.h>
#include <thread>
#include<mutex>
#include<atomic>
#include<vector>
#include<queue>
#include<functional>
#include<condition_variable>
#include <unique>
class ThreadPool
{
public:
    ThreadPool(int min = 2, int max = 4); // 构造函数
    void addTask(std::function<void()> task); // 添加任务函数->添加到任务队列
    ~ThreadPool();     // 析构函数，资源释放

private:
    void manager();//管理者线程函数
    void worker();//工作者线程函数
private:
    std::thread m_manager;
    std::vector<std::thread> m_workers;
    std::atomic<int> m_minThread;
    std::atomic<int> m_maxThread;
    std::atomic<int> m_curThread;
    std::atomic<int> m_idleThread;//当前空闲的原子数量
    std::atomic<bool> m_stop;
    std::queue<std::function<void()>> m_tasks;//任务队列(这个不会)
    std::mutex m_queueMutex;
    std::condition_variable m_condition; // 用来阻塞消费者线程

public:
    ThreadPool(/* args */);
    ~ThreadPool();
};

