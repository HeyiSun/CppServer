#include <CppServer/CppServer.h>

CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

void run_in_fiber() {
    CPPSERVER_LOG_INFO(g_logger) << "run_in_fiber begin";
    CppServer::Fiber::GetThis()->YieldToHold();
    CPPSERVER_LOG_INFO(g_logger) << "run_in_fiber end";
    CppServer::Fiber::GetThis()->YieldToHold();
}

void test_fiber() {
    CppServer::Fiber::GetThis();
    {
        CPPSERVER_LOG_INFO(g_logger) << "main begin";
        CppServer::Fiber::ptr fiber(new CppServer::Fiber(run_in_fiber));
        fiber->swapIn();
        CPPSERVER_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        CPPSERVER_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    CPPSERVER_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char **argv) {
    CppServer::Thread::SetName("main");
    std::vector<CppServer::Thread::ptr> thrs;
    for (int i = 0; i < 3; ++i) {
        thrs.push_back(CppServer::Thread::ptr(new CppServer::Thread(test_fiber, "name_" + std::to_string(i))));
    }
    for (int i = 0; i < 3; ++i) {
        thrs[i]->join();
    }

    return 0;
}
