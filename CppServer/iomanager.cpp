#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace CppServer {

static CppServer::Logger::ptr g_logger = CPPSERVER_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
    switch (event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            CPPSERVER_ASSERT2(false, "getContext");
    }
}

void IOManager::FdContext::resetContext(EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.cb = nullptr;
}
// 为什么不重置eventContext? schedule之后cb/fiber不一定运行，所以当然不能重置context
void IOManager::FdContext::triggerEvent(IOManager::Event event) {
    CPPSERVER_ASSERT(events & event);  // 事件存在
    events = (Event) (events & ~event);
    EventContext& ctx = getContext(event);
    if (ctx.cb) {
        ctx.scheduler->schedule(&ctx.cb);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t threads, bool use_caller, const std::string& name) 
    : Scheduler(threads, use_caller, name) {
    m_epfd = epoll_create(5000);
    CPPSERVER_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    CPPSERVER_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0 ,sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0]; // 0 for read

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK); // ???
    CPPSERVER_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event); //???
    CPPSERVER_ASSERT(!rt);

    contextResize(64);

    start(); // schedule
}

IOManager::~IOManager() {
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);
    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    // 为了安全读写这个fd_ctx, 用fd_ctx内部的锁
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (fd_ctx->events & event) {
        CPPSERVER_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                      << " event=" << event
                                      << " fd_ctx.evet" << fd_ctx->events;
        CPPSERVER_ASSERT(!(fd_ctx->events & event));
    } 
    // fd_ctx事件非0则为mod，为0则为ADD
    int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->events | event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        CPPSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << op << "," << fd << "," << "," << epevent.events << "):"
            << rt << " (" << errno << ")  (" << strerror(errno) << ")";
        return -1;
    }

    ++m_pendingEventCount;
    // 设置fd_ctx各项
    fd_ctx->events = (Event) (fd_ctx->events | event);
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    CPPSERVER_ASSERT(!event_ctx.scheduler
                    && !event_ctx.fiber
                    && !event_ctx.cb);
    event_ctx.scheduler = Scheduler::GetThis();  // 当前线程的调度器
    if (cb) {
        event_ctx.cb.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis(); // 当前的协程
        CPPSERVER_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
                          , "state=" << event_ctx.fiber->getState());
    }
    return 0;
}

bool IOManager::delEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event) (fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;
    int rt  = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        CPPSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd<<", "
            << op << ", " << fd << ", " << epevent.events << "):"
            << rt << "  (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    --m_pendingEventCount;
    fd_ctx->events = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}

bool IOManager::cancelEvent(int fd, Event event) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int) m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!(fd_ctx->events & event)) {
        return false;
    }

    Event new_events = (Event) (fd_ctx->events & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;
    int rt  = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        CPPSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd<<", "
            << op << ", " << fd << ", " << epevent.events << "):"
            << rt << "  (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }
    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if ((int) m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if (!fd_ctx->events) {
        return false;
    }

    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt  = epoll_ctl(m_epfd, op, fd, &epevent);
    if (rt) {
        CPPSERVER_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd<<", "
            << op << ", " << fd << ", " << epevent.events << "):"
            << rt << "  (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if (fd_ctx->events & READ) {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }
    if (fd_ctx->events & WRITE) {
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;

    }

    CPPSERVER_ASSERT(fd_ctx->events == 0);
    return true;
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    // 有闲置的线程才有发消息的必要
    if (hasIdleThreads()) {
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    CPPSERVER_ASSERT(rt == 1);
}

bool IOManager::stopping(uint64_t& timeout) {
    timeout = getNextTimer();
    return timeout == ~0ull  // 一定要有这个条件，因为schduler调用stopping会调用到iomanager::stopping, 导致定时器事件存在但是scheduler跳出循环了
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {
    CPPSERVER_LOG_DEBUG(g_logger) << "idle";
    const uint64_t MAX_EVENTS = 256;
    epoll_event* events = new epoll_event[MAX_EVENTS]();
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });
    while (true) {
        uint64_t next_timeout = 0;
        if (stopping(next_timeout)) {
            CPPSERVER_LOG_INFO(g_logger) << "name=" << getName()
                                         << " idle stopping exit";
            break;
        }
        int rt = 0;
        do {
            static const int MAX_TIMEOUT  = 5000; // 5s
            if (next_timeout != ~0ull) {  // 有定时器的超时存在, 注意有符号的情况下，~0ull是负数
                next_timeout = (int) next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT : next_timeout;
                // 如果next_timeout过大，还是要要及时让出CPU看看有没有新任务
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int) next_timeout);

            if (rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while (true);

        std::vector<std::function<void()>> cbs;
        listExpiredCb(cbs);
        if (!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for (int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            if (event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                // 因为是边沿出发，所以要一次消化掉所有tickle
                while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }
            FdContext* fd_ctx = (FdContext*) event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            // EPOLLHUP代表socket一端关闭，拔网线
            if (event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events; //??? 这行是在？
            }
            int real_events = NONE;
            if (event.events & EPOLLIN) {
                real_events |= READ;
            }
            if (event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            // 发生的事件并未注册
            if ((fd_ctx->events & real_events) == NONE) {
                continue;
            }
            // 剩余的事件 = 注册事件 - 发生事件
            int left_events = (fd_ctx->events & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if (rt2) {
                CPPSERVER_LOG_ERROR(g_logger) <<  "epoll_ctl(" << m_epfd << ", "
                    << op << "," << fd_ctx->fd << "," << event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            if (real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if (real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        // 当scheduler/iomanager未调用stop()的时候,idle还未执行完，只是需要让出cpu，直接返回会被设置成TERM状态而可能导致scheduler::run协程终止（无其他任务的情况下)，所以要自己swapOut, 等到时候再swap回来继续循环
        // 因为是是直接的栈切换，RTTI无效，需要手动reset智能指针
        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}


void IOManager::onTimerInsertedAtFront() {
    tickle();
}

};
