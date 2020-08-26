#ifndef __CPPSERVER_TIMER_H__
#define __CPPSERVER_TIMER_H__


#include <set>
#include <memory>
#include <vector>
#include "thread.h"

namespace CppServer {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
 friend class TimerManager;
 public:
    typedef std::shared_ptr<Timer> ptr;
    bool cancel();  // 取消定时器
    bool refresh(); // 从现在开始重新计时该定时器
    bool reset(uint64_t ms, bool from_now); //重新设置时钟的周期，可以选择从现在开始使用新周期计时，或者在下次触发后再使用新周期计时
 private:
    Timer(uint64_t ms, std::function<void()> cb,
          bool recurring, TimerManager* manager);
    Timer(uint64_t next); // 用来构造一个临时的timer，从m_timers中筛选timer

 private:
    bool m_recurring = false;  // 是否循环定时器
    uint64_t m_ms = 0;         // 执行周期
    uint64_t m_next = 0;       // 精确的执行时间
    std::function<void()> m_cb;
    TimerManager* m_manager = nullptr;
 private:
    struct Comparator {
        bool operator() (const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };

};

class TimerManager {
 friend class Timer;
 public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                        , bool recurring = false);
    Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                                , std::weak_ptr<void> weak_cond // why weak ptr???
                                , bool recurring = false);
    uint64_t getNextTimer();
    void listExpiredCb(std::vector<std::function<void()>>& cbs);
    bool hasTimer();
 protected:
    virtual void onTimerInsertedAtFront() = 0;
    void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);
 private:
    // 检测服务器调时间
    bool detectClockRollover(uint64_t now_ms);
 private:
    RWMutexType m_mutex;
    std::set<Timer::ptr, Timer::Comparator> m_timers;
    bool m_tickled = false;
    uint64_t m_previousTime = 0; // 上一次的执行时间
};

}

#endif // __CPPSERVER_TIMER_H__
