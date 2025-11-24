#include "ThreadPool.h"
ThreadPool::ThreadPool(int min, int max) : m_maxThread(max), m_minThread(min), m_stop(false), m_idleThread(min), m_curThread(min), m_exitThread(0)
// 在构造函数里，主要对初始的成员进行初始化
{
    // 创建管理者线程
    m_manager = std::thread(&ThreadPool::manager, this);
    // 创建工作线程
    for (int i = 0; i < min; i++)
    {
        // 这里的t仅仅只是临时变量
        // std::thread t(&ThreadPool::worker, this);
        // // 把线程对象放在容器里
        // m_workers.emplace_back(t);
        // 上述代码这是复制构造 thread 对象！（不安全）std::thread不能复制，只能移动&ThreadPool::worker, this
        std::thread t(&ThreadPool::worker, this);
        m_workers.insert(std::make_pair(t.get_id(), std::move(t))); // map中插入元素的方法
    }
}
// 添加任务函数  往任务队列中插入任务
void ThreadPool::addTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex); // 锁住任务队列
        m_tasks.emplace(task);
    }
    m_condition.notify_one();
}
// 线程池析构函数
ThreadPool::~ThreadPool()
{
    m_stop = true;
    //------------------------释放消费者线程
    m_condition.notify_all();
    if (m_manager.joinable())
    {
        std::cout << "------（析构函数）管理者线程退出" << std::endl;
        m_manager.join();
    }

    for (auto &it : m_workers)
    {
        std::cout << "------（析构函数）线程退出" << it.second.get_id() << std::endl;
        it.second.join();
    }
    //-------------------------释放管理者线程
}

void ThreadPool::manager()
{
    while (!m_stop)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3)); // 让管理者线程睡3秒，每隔3秒检测一次
        // 【改进】不管扩容还是缩容，每次醒来先看看有没有“尸体”需要处理
        // 这样可以避免 Worker 刚退出，Manager 刚好睡着，导致要等很久才回收
        {
            std::lock_guard<std::mutex> lock(m_idsMutex);
            for (auto id : m_ids)
            {
                auto it = m_workers.find(id);
                if (it != m_workers.end())
                {
                    if (it->second.joinable()) // 严谨一点
                        it->second.join();
                    std::cout << "--------(Manager) 线程销毁,ID::" << it->first << std::endl;
                    m_workers.erase(it);
                }
            }
            m_ids.clear();
        }

        int idel = m_idleThread;
        int cur = m_curThread;
        //-----------------------有一半的线程啥也没干,销毁线程，每次销毁两个线程
        if (idel > cur / 2 && cur > m_minThread)
        {
            m_exitThread = 2;
            m_condition.notify_all(); //
        }
        //-----------------------进行线程添加，往容器中添加线程，注意线程只能移动不能copy
        else if (idel == 0 && cur < m_maxThread)
        {
            {
                std::lock_guard<std::mutex> lock(m_managerMutex); // 锁住
                std::thread t(&ThreadPool::worker, this);
                std::cout << "添加了一个线程,ID:" << t.get_id() << std::endl;
                m_workers.insert(std::make_pair(t.get_id(), std::move(t))); //
            }
            m_curThread++;
            m_idleThread++;
        }
    }
}

void ThreadPool::worker()
{
    while (!m_stop) // 判断线程池是否关闭，如果线程池开启，则一直运行
    {
        std::function<void()> task = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex); // 这个锁主要是为了保证取出任务是线程安全的
            // 【修改点 1】使用 lambda 表达式，醒来的条件是：
            // 1. 线程池停止了 (m_stop)
            // 2. 或者 有任务了 (!empty)
            // 3. 或者 需要裁员了 (m_exitThread > 0)
            m_condition.wait(lock, [this]
                             { return m_stop || !m_tasks.empty() || m_exitThread > 0; });

            // 销毁线程，只有在决定缩容的时候，才去回收死掉的线程？
            if (m_exitThread > 0) // 阻塞的线程  唤醒之后进行判断，如果>0则线程退出，如果<=0则继续往下执行
            {
                m_curThread--;
                m_idleThread--;
                m_exitThread--;
                std::cout << "---------线程准备退出了,ID:  " << std::this_thread::get_id() << std::endl;
                std::lock_guard<std::mutex> lock(m_idsMutex);
                m_ids.emplace_back(std::this_thread::get_id()); // 把要退出的线程id进行存储，这里有问题：什么时候释放线程号所对应的对象？（在管理者线程释放）
                return;                                         // 结束当前线程的执行，结束线程函数的运行（worker是线程函数）
            }

            // 任务队列不空
            if (!m_tasks.empty()) // 为了更严谨，在判断一下队伍是不是为空
            {
                std::cout << "取出一个任务" << std::endl;
                task = std::move(m_tasks.front()); // 如果是不使用move，从对头取出元素（实际上说拷贝）
                m_tasks.pop();
            }
        }

        if (task) // 为了严谨，判断这个task对象是不是空对象
        {
            m_idleThread--;
            task();
            m_idleThread++;
        }
    }
}

void cal()
{
    std::cout << "10" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
}
int main()
{
    ThreadPool pool;
    // 添加任务
    for (size_t i = 0; i < 10; i++)
    {
        std::cout << "添加了任务 " << i << std::endl;
        pool.addTask(cal);
    }
    getchar();
    return 0;
}