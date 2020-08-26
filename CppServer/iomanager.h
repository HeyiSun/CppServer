#ifndef __CPPSERVER_IOManager_H__
#define __CPPSERVER_IOManager_H__

#include  "scheduler.h"
#include "timer.h"

namespace CppServer {

class IOManager : public Scheduler, public TimerManager {
 public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event {
        NONE    = 0x0,
        READ    = 0x1,
        WRITE   = 0x4
    };
 private:
    struct FdContext {
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler* scheduler = nullptr;       //事件待执行的scheduler
            Fiber::ptr fiber;           //事件协程
            std::function<void()> cb;   //事件回调
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(IOManager::Event event);

        EventContext read;       // 读事件
        EventContext write;      // 写事件
        int fd = 0;              // 事件关联的描述符
        Event events = NONE;   // 已经注册的事件
        MutexType mutex;
    };
 public:
    IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
    ~IOManager();
    // 1 success, -1 error
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);  // 取消事件: 删除事件并强制触发事件  ????
    bool cancelAll(int fd);            // 取消一个描述符下的所有事件

    static IOManager* GetThis();

 protected:
    void tickle() override;   // 有协程需要执行的时候触发
    bool stopping() override; // 协程调度模块是否应该终止
    void idle() override;     // 陷入epoll_wait
    void onTimerInsertedAtFront() override;

    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);
 private:
    int m_epfd = 0;
    int m_tickleFds[2];

    std::atomic<size_t> m_pendingEventCount = {0}; // 现在要等待执行的事件数量
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;
};

};































#endif // __CPPSERVER_IOManager_H__
