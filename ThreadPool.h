#include <stdio.h>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <queue>
#include <functional>
#include <condition_variable>
#include <iostream>
#include <map>

class ThreadPool
{
public:
    ThreadPool(int min = 4, int max = 6);     // 构造函数
    void addTask(std::function<void()> task); // 添加任务函数->添加到任务队列
    ~ThreadPool();                            // 析构函数，资源释放

private:
    void manager(); // 管理者线程函数
    void worker();  // 工作者线程函数
private:
    std::thread m_manager;
    std::map<std::thread::id, std::thread> m_workers;
    std::vector<std::thread::id> m_ids; // 存储已经退出任务函数的线程ID

    std::atomic<int> m_minThread;
    std::atomic<int> m_maxThread;  // 线程池最大的线程数量
    std::atomic<int> m_curThread;  // 当前线程数量，实际存在的线程数量
    std::atomic<int> m_idleThread; // 当前空闲的的线程数量
    std::atomic<bool> m_stop;
    std::atomic<int> m_exitThread;             // 退出线程数量
    std::queue<std::function<void()>> m_tasks; // 任务队列(这个不会)
    std::mutex m_queueMutex;
    std::mutex m_managerMutex;
    std::mutex m_idsMutex;
    std::condition_variable m_condition; // 用来阻塞消费者线程
};