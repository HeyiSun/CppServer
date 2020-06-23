//
// Created by Sunhe on 6/22/2020.
//
#include <CppServer/CppServer.h>

CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

int count = 0;
CppServer::RWMutex s_mutex;

void func1() {
    CPPSERVER_LOG_INFO(g_logger) << " name: " << CppServer::Thread::GetName()
                                 << " this.name: " << CppServer::Thread::GetThis()->getName()
                                 << " id: " << CppServer::GetThreadId()
                                 << " this.id: " << CppServer::Thread::GetThis()->getId();
    for (int i = 0; i < 100000; i++) {
        CppServer::RWMutex::ReadLock lock(s_mutex);
        ++count;
    }
}

void func2() {

}

int main(int argc, char** argv) {
    CPPSERVER_LOG_INFO(g_logger) << "thread test begin";
    std::vector<CppServer::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i) {
        CppServer::Thread::ptr thr(new CppServer::Thread(&func1, "name_" + std::to_string(i)));
        thrs.push_back(thr);
    }

    for (int i = 0; i < 5; ++i) {
        thrs[i]->join();
    }
    CPPSERVER_LOG_INFO(g_logger) << "thread test end";
    CPPSERVER_LOG_INFO(g_logger) << "count =" << count;
    return 0;
}
