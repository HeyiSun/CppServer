#include "timer.h"
#include "util.h"


namespace CppServer {

bool Timer::Comparator::operator() (const Timer::ptr& lhs
                                  ,const Timer::ptr& rhs) const {
    if (!lhs && !rhs) {
        return false;
    }
    if (!lhs) {
        return true;
    }
    if (!rhs) {
        return false;
    }
    if (lhs->m_next < rhs->m_next) {
        return true;
    }
    if (rhs->m_next < lhs->m_next) {
        return false;
    }
    return lhs.get() < rhs.get();
};

Timer::Timer(uint64_t ms, std::function<void()> cb,
             bool recurring, TimerManager* manager)
    : m_recurring{recurring}
    , m_ms{ms}
    , m_cb{cb}
    , m_manager (manager) {
    m_next = CppServer::GetCurrentMS() + m_ms;
}

// Timer::Timer(uint64_t next)
//     : m_next(next) {}
Timer::Timer(uint64_t next)
    :m_next(next) {
}


bool Timer::cancel() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh() {
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = CppServer::GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
    if (ms == m_ms && !from_now) {
        return false;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if (!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if (it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if (from_now) {
        start = CppServer::GetCurrentMS();
    } else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

TimerManager::TimerManager() {
    m_previousTime = CppServer::GetCurrentMS();
}

TimerManager::~TimerManager() {

}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb
                                  ,bool recurring) {
    Timer::ptr timer(new Timer(ms, cb, recurring, this));
    RWMutexType::WriteLock lock(m_mutex);
    addTimer(timer, lock);
    return timer;
}

static void onTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
    std::shared_ptr<void> tmp = weak_cond.lock();
    if (tmp) {
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb
                            , std::weak_ptr<void> weak_cond // why weak ptr???
                            , bool recurring) {
    return addTimer(ms, std::bind(&onTimer, weak_cond, cb), recurring);
}

uint64_t TimerManager::getNextTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false; // 清理m_tickled
    if (m_timers.empty()) {
        return ~0ull;
    }
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now_ms = CppServer::GetCurrentMS();
    if (now_ms >= next->m_next) {
        return 0;
    } else {
        return next->m_next - now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs) {
    uint64_t now_ms = CppServer::GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        RWMutexType::ReadLock lock(m_mutex);
        if (m_timers.empty()) {
            return;
        }
    }
    RWMutexType::WriteLock lock(m_mutex);

    bool rollover = detectClockRollover(now_ms);
    if (!rollover && ((*m_timers.begin())->m_next > now_ms)) {

        return;
    }

    Timer::ptr now_timer(new Timer(now_ms));
    auto it = rollover ? m_timers.end() : m_timers.upper_bound(now_timer);
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());
    for (auto&& timer: expired) {
        cbs.push_back(timer->m_cb);
        if (timer->m_recurring) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        } else {
            timer->m_cb = nullptr;
        }
    }
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
    auto it = m_timers.insert(val).first;
    // 当插入到m_timers最前端的时候，相当于插入的timer是第一个要发生的，那要唤醒陷入epoll_wait的schduler中的线程, 不然epoll_wait的超时时间都是之前的最短计时，有可能导致新的插入的更短的计时无法及时触发
    bool at_front = (it == m_timers.begin()) && !m_tickled;
    if (at_front) {
        m_tickled = true; // 防止频繁修改，导致onTimerInsertedAtFront()频繁触法????
    }
    lock.unlock();

    if (at_front) {
        onTimerInsertedAtFront();
    }
}


// 检测服务器调时间
bool TimerManager::detectClockRollover(uint64_t now_ms) {
    bool rollover= false;
    // 传进的时间小于m_previousTime 1小时
    if (now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000)) {
        rollover = true;
    }
    m_previousTime = now_ms;
    return rollover;
}


bool TimerManager::hasTimer() {
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();
}


}  // CppServer
