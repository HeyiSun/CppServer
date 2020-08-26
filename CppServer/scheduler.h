#ifndef __CPPSERVER_SCHEDULER_H__
#define __CPPSERVER_SCHEDULER_H__

#include <memory>
#include <vector>
#include <list>
#include "fiber.h"
#include "thread.h"

namespace CppServer {

class Scheduler {
 public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads = 1, bool use_caller = true, const std::string& name =""); 
    virtual ~Scheduler();

    const std::string& getName() const { return m_name; }
    static Scheduler* GetThis();
    static Fiber* GetMainFiber();

    void start();
    void stop();

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if (need_tickle) {
            tickle();
        }
    }

    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                need_tickle = scheduleNoLock(&*begin, -1) || need_tickle; // ??? why &*, why pass by pointer ?
                ++begin;
            }
        }
        if (need_tickle) {
            tickle();
        }
    }
 protected:
    virtual void tickle();
    void run();
    virtual bool stopping();
    virtual void idle(); // 解决线程没事做的时候干的事情，让子类实现

    void setThis(); // protected?
    bool hasIdleThreads() { return m_idleThreadCount > 0; }
 private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_fibers.empty();
        FiberAndThread ft(fc, thread);
        if (ft.fiber || ft.cb) {
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
 private:
    struct FiberAndThread {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread; // 该任务/协程被指派的线程，-1代表任一线程

        FiberAndThread(Fiber::ptr f, int thr) : fiber{f}, thread{thr} {}
        FiberAndThread(Fiber::ptr* f, int thr) : thread{thr} { fiber.swap(*f); }
        FiberAndThread(std::function<void()> f, int thr) : cb{f}, thread{thr} {}
        FiberAndThread(std::function<void()> *f, int thr) : thread{thr} { cb.swap(*f); }
        FiberAndThread() : thread(-1) {}

        void reset() {
            fiber = nullptr;
            cb = nullptr;
            thread =  -1;
        }
    };

 private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<FiberAndThread> m_fibers; // 等待执行的协程任务
    Fiber::ptr m_rootFiber; // rootFiber是创建sceduler的线程里面那个run的协程
    std::string m_name;

 protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    //此处若用=0, 由于0不是atomic类型且不是其子类,所以不会考虑直接调用构造函数,此处会将右侧转换程左侧(atomic)的类型,然后拷贝构造,可惜atomic的拷贝构造是delete的
    //此处若用 ={0}列表初始化,调用构造函数 

    std::atomic<size_t> m_activeThreadCount = {0};
    std::atomic<size_t> m_idleThreadCount = {0};
    bool m_stopping = true;
    bool m_autoStop = true; // 是否主动停止???
    int m_rootThread = 0; // 启动scheduler的主线程
};


}


#endif // __CPPSERVER_SCHEDULER_H__
