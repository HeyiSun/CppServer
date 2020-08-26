#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace CppServer{

static Logger::ptr g_logger = CPPSERVER_LOG_NAME("system");

static std::atomic<uint64_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

// 代表当前协程, 第一次被赋值后，从未有被赋值为nullptr的情况
static thread_local Fiber* t_fiber = nullptr;
// 代表主协程, 每个线程的主协程一旦分配，就会一直在直到线程结束才会自动析构
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size", 1024*1024, "fiber stack size");

class MallocStatckAllocator {
 public:
    static void* Alloc(size_t size) {
        return malloc(size);
    }

    // size for mmap
    static void Dealloc(void* vp, size_t size) {
        return free(vp);
    }
};

using StackAllocator = MallocStatckAllocator;

uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

Fiber::Fiber() {
    m_state = EXEC;
    SetThis(this);

    if (getcontext(&m_ctx)) {
        CPPSERVER_ASSERT2(false, "getcontext");
    }
    ++s_fiber_count;

    CPPSERVER_LOG_DEBUG(g_logger) << "Fiber::Fiber() --- id=" << m_id;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
    : m_id(++s_fiber_id)
    , m_cb(cb) {
    ++s_fiber_count;
    m_stacksize = stacksize == 0 ? g_fiber_stack_size->getValue() : stacksize;

    m_stack = StackAllocator::Alloc(m_stacksize);
    if (getcontext(&m_ctx)) {
        CPPSERVER_ASSERT2(false, "getcontext");
    }
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    if (!use_caller) {
        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    } else {
        makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
    }

    CPPSERVER_LOG_DEBUG(g_logger) << "Fiber::Fiber() --- id=" << m_id;
}

Fiber::~Fiber() {
    --s_fiber_count;
    if (m_stack) { // 主协程不需要分配的栈空间，直接用本身的栈就可以
        CPPSERVER_ASSERT(m_state == TERM
                    || m_state == INIT
                    || m_state == EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    } else { // main fiber
        CPPSERVER_ASSERT(!m_cb); // no callback
        CPPSERVER_ASSERT(m_state == EXEC);

        Fiber* cur = t_fiber;
        if (cur == this) {
            SetThis(nullptr);
        }
    }
    CPPSERVER_LOG_DEBUG(g_logger) << "Fiber::~Fiber() --- id=" << m_id;
}

void Fiber::reset(std::function<void()> cb) {
    CPPSERVER_ASSERT(m_stack);
    CPPSERVER_ASSERT(m_state == TERM ||
                     m_state == INIT ||
                     m_state == EXCEPT);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        CPPSERVER_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = INIT;
}

void Fiber::call() {
    // 当前协程从主协程设为本协程
    SetThis(this); // 曾经由于这里没SetThis，导致MainFunc里cur=GetThis()取的不是当前协程，cur->cb=nullptr以致bad_function_call
    CPPSERVER_ASSERT(m_state != EXEC);
    m_state = EXEC; // ?? change with next line
    if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
        CPPSERVER_ASSERT2(false, "swapcontext");
    }
}

void Fiber::back() {
    // 当前协程从主协程设为本协程
    SetThis(t_threadFiber.get());
    if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        CPPSERVER_ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapIn() {  // 从调度协程swap到此，调度协程不一定是主协程
    // 当前协程从run协程设为本协程
    SetThis(this);
    CPPSERVER_ASSERT(m_state != EXEC);
    m_state = EXEC; // ?? change with next line
    if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
        CPPSERVER_ASSERT2(false, "swapcontext");
    }
}

void Fiber::swapOut() {
    // 当前协程从本协程设为run协程
    SetThis(Scheduler::GetMainFiber());
    if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
        CPPSERVER_ASSERT2(false, "swapcontext");
    }
}

void Fiber::SetThis(Fiber* f) {
    t_fiber = f;
}

Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }
    Fiber::ptr main_fiber(new Fiber);
    CPPSERVER_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

void Fiber::YieldToReady() {
    Fiber::ptr cur = GetThis();
    cur->m_state = READY;
    cur->swapOut();
}

void Fiber::YieldToHold() {
    Fiber::ptr cur = GetThis();
    cur->m_state = HOLD;
    cur->swapOut();
}

uint64_t Fiber::TotalFibers() {
    return s_fiber_count;
}

void Fiber::MainFunc() {
    Fiber::ptr cur = GetThis();
    CPPSERVER_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        CPPSERVER_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
           << " fiber_id =" << cur->m_id
           << std::endl
           << CppServer::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        CPPSERVER_LOG_ERROR(g_logger) << "Fiber Except";
    }

    // 因为该函数不会返回，所以这里特殊手法释放智能指针
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();

    CPPSERVER_ASSERT2(false, "Never Reach, fiber_id=" + std::to_string(raw_ptr->m_id));
}

void Fiber::CallerMainFunc() {
    Fiber::ptr cur = GetThis();
    CPPSERVER_ASSERT(cur);
    try {
        cur->m_cb();
        cur->m_cb = nullptr;
        cur->m_state = TERM;
    } catch (std::exception& ex) {
        cur->m_state = EXCEPT;
        CPPSERVER_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
           << " fiber_id =" << cur->m_id
           << std::endl
           << CppServer::BacktraceToString();
    } catch (...) {
        cur->m_state = EXCEPT;
        CPPSERVER_LOG_ERROR(g_logger) << "Fiber Except";
    }

    // 因为该函数不会返回，所以这里特殊手法释放智能指针
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->back();
    /*
        如何防止cur.reset析构掉当前对象？
        t_fiber一直都是指向一个被智能指针托管的对象，所以这里GetThis()相当于让cur共享了这个对象，所以cur一定不是这个对象的唯一owner
        如果cur是唯一owner, 意味着GetThis()里的t_fiber一定指向一个未被智能指针托管的对象，那其中的shared_from_this()肯定出问题
    */

    CPPSERVER_ASSERT2(false, "Never Reach, fiber_id=" + std::to_string(raw_ptr->m_id));
}

} // CppServer




















