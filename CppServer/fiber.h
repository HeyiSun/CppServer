#ifndef __CPPSERVER_FIBER_H__
#define __CPPSERVER_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace CppServer {


// Thread ->  main_fiber  <----> sub_fiber
//                ^
//                |
//                |
//                v
//             sub_fiber
class Fiber : public std::enable_shared_from_this<Fiber> {
 public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State {
        INIT,
        HOLD,
        EXEC,
        TERM,
        READY,
        EXCEPT
    };
 private:
    Fiber();

 public:
    Fiber(std::function<void()> cb, size_t stacksize = 0);
    ~Fiber();

    void reset(std::function<void()> cb);
    void swapIn();
    void swapOut();

    uint64_t getId() const { return m_id; }

 public:
    static void SetThis(Fiber* f);
    static Fiber::ptr GetThis();
    static void YieldToReady();
    static void YieldToHold();
    static uint64_t TotalFibers();

    static void MainFunc();
    static uint64_t GetFiberId();

 private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_cb;
};

}

#endif // __CPPSERVER_FIBER_H__
