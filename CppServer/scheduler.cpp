#include "scheduler.h"
#include "log.h"
#include "macro.h"
#include "hook.h"

namespace CppServer {

static CppServer::Logger::ptr g_logger = CPPSERVER_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;    // 当前线程对应的调度器
static thread_local Fiber* t_scheduler_fiber = nullptr;  // 线程中执行run的的协程
// 其它线程的run协程就是主协程，schedule本身的线程的run协程不是主协程

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name) 
    : m_name(name) {
    CPPSERVER_ASSERT(threads > 0);

    if (use_caller) {
        CppServer::Fiber::GetThis();
        --threads;

        CPPSERVER_ASSERT(GetThis() == nullptr);
        t_scheduler = this;

        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        CppServer::Thread::SetName(m_name);

        t_scheduler_fiber = m_rootFiber.get(); //因为主线程要作为线程池的一员，需要跑run方法，但是作为主线程没法跑run方法，所以另开一个协程跑run方法
        m_rootThread = CppServer::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1;
    }
    m_threadCount = threads;
} 

Scheduler::~Scheduler() {
    CPPSERVER_ASSERT(m_stopping);
    if (GetThis() == this) {
        t_scheduler = nullptr;
    }
}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if (!m_stopping) { // m_stopping = false 意味着调度器已启动
        return;
    }
    m_stopping = false;
    CPPSERVER_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for (size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), 
                                      m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock(); // 下面rootFiber还是要跑run，还要再锁一次
    // if (m_rootFiber) {
    //     m_rootFiber->call();
    //     CPPSERVER_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    // }
}

void Scheduler::stop() {
    m_autoStop = true;
    if (m_rootFiber &&
        m_threadCount == 0 &&
        (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT)) {
        // INIT -> 没跑起来
        CPPSERVER_LOG_INFO(g_logger) << this << " stopped";
        m_stopping = true;

        if (stopping()) { // stopping函数让scheduler子类有清理任务的机会
            return;
        }
    }
    // bool exit_on_this_fiber = false;
    if (m_rootThread != -1) { // use_caller
        // 当use_caller时， stop一定要在创建线程执行
        CPPSERVER_ASSERT(GetThis() == this);
    } else {
        CPPSERVER_ASSERT(GetThis() != this);
    }

    m_stopping = true;
    for (size_t i = 0; i < m_threadCount; ++i) {
        // 唤醒线程，让他们自己结束
        tickle(); // wake up
    }

    if (m_rootFiber) {
        tickle();
    }

    if (m_rootFiber) {
        // while (!stopping()) {  //等待其它线程完成
        //     if (m_rootFiber->getState() == Fiber::TERM ||
        //             m_rootFiber->getState() == Fiber::EXCEPT) {
        //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //         t_scheduler_fiber = m_rootFiber.get();
        //         CPPSERVER_LOG_INFO(g_logger) << " root fiber is term, reset";
        //     }
        //     m_rootFiber->call();
        // }
        if (!stopping()) {
            m_rootFiber->call();
        }
    }

    if (stopping()) { //?
        return;
    }


    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for (auto&& i : thrs) {
        i->join();
    }
    // if (exit_on_this_fiber) {
    // }
}

void Scheduler::run() {
    // Fiber::GetThis(); // 初始化主协程
    CPPSERVER_LOG_INFO(g_logger) << "run";
    set_hook_enable(true);
    setThis();
    if (CppServer::GetThreadId() != m_rootThread) { // rootThread在use_caller的情况下已经在构造函数里创建主协程了
        t_scheduler_fiber = Fiber::GetThis().get();    
    }
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber; // callback

    FiberAndThread ft;
    while (true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        {
            // 从协程队列取出协程
            MutexType::Lock lock(m_mutex); 
            auto it = m_fibers.begin();
            while (it != m_fibers.end()) {
                if (it->thread != -1 && it->thread != CppServer::GetThreadId()) {
                    ++it;
                    tickle_me = true; //该线程不能处理下一个协程，tickle_me让信号量驱使下一个新的线程来找任务
                    continue;
                }
                CPPSERVER_ASSERT(it->fiber || it->cb);
                if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
                    ++it;
                    continue;
                }

                ft = *it;
                m_fibers.erase(it++);
                ++m_activeThreadCount;
                is_active = true;
                break;
            }
        }


        if (tickle_me) {
            tickle();
        }

        if (ft.fiber && ft.fiber->getState() != Fiber::TERM
                     && ft.fiber->getState() != Fiber::EXCEPT) {
            ft.fiber->swapIn();
            --m_activeThreadCount;

            if (ft.fiber->getState() == Fiber::READY) {
                schedule(ft.fiber); // 还需要执行
            } else if (ft.fiber->getState() != Fiber::TERM &&
                       ft.fiber->getState() != Fiber::EXCEPT) {
                ft.fiber->m_state = Fiber::HOLD;
            }
            ft.reset();
        } else if (ft.cb) {
            if (cb_fiber) {
                cb_fiber->reset(ft.cb);
            } else {
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();  // 智能指针置空, 释放ft
            cb_fiber->swapIn();
            --m_activeThreadCount;

            if (cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset(); // 智能指针置空, 释放cb_fiber, 
                                  // 如果不释放，cb_fiber的智能指针将会跟新schedule的fiber指针共享资源，若之后的好几个任务没有ft.cb类型的，在其他线程跑完新schedule的fiber后无法成功释放资源

            } else if (cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr); // cb_fiber中的执行函数置空, 不会析构cb_fiber
                                          // fiber已经跑完，不会出现上述问题
            } else {
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {
            if (is_active) {
                --m_activeThreadCount;
                continue;
            }
            if (idle_fiber->getState() == Fiber::TERM) {
                CPPSERVER_LOG_INFO(g_logger) << "idle fiber term";
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if (idle_fiber->getState() != Fiber::TERM
                     && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }

    }
}

void Scheduler::tickle() {
    CPPSERVER_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex); // for m_fibers(it is a list)
    return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
}

void Scheduler::idle() {
    CPPSERVER_LOG_INFO(g_logger) << "idle!!";
    while (!stopping()) {
        Fiber::YieldToHold();
    }
}

void Scheduler::setThis() {
    t_scheduler = this;
}


} // CppServer
