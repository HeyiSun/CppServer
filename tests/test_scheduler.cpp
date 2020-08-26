#include "CppServer/CppServer.h"

static CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

void test_fiber() {
    static int s_count = 5;
    CPPSERVER_LOG_INFO(g_logger) << "test in fiber, s_count="
                                 << s_count << " !!!!!!!";

    sleep(1);
    if (--s_count >= 0) {
       CppServer::Scheduler::GetThis()->schedule(&test_fiber);
    }
}

int main(int argc, char** argv) {
    // 这样会被parse成一个函数
    // CppServer::Scheduler sc();
    {
        CPPSERVER_LOG_INFO(g_logger) << "main";
        CppServer::Scheduler sc(3, false, "test");
        sc.start();
        sleep(2);
        CPPSERVER_LOG_INFO(g_logger) << "schedule";
        sc.schedule(&test_fiber);
        sc.stop();
        CPPSERVER_LOG_INFO(g_logger) << "over";
    }
    CPPSERVER_LOG_INFO(g_logger) << "return";
    return 0;
}




