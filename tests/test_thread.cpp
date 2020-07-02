//
// Created by Sunhe on 6/22/2020.
//
#include <CppServer/CppServer.h>

CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

int count = 0;
// CppServer::RWMutex s_mutex;
CppServer::Mutex s_mutex;

void func1() {
    CPPSERVER_LOG_INFO(g_logger) << " name: " << CppServer::Thread::GetName()
                                 << " this.name: " << CppServer::Thread::GetThis()->getName()
                                 << " id: " << CppServer::GetThreadId()
                                 << " this.id: " << CppServer::Thread::GetThis()->getId();
    for (int i = 0; i < 100000; i++) {
        CppServer::Mutex::Lock lock(s_mutex);
        ++count;
    }
}

void func2() {
    while (true)
        CPPSERVER_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
}

void func3() {
    while (true)
        CPPSERVER_LOG_INFO(g_logger) << "===========================================================================================================";
}

int main(int argc, char** argv) {
    CPPSERVER_LOG_INFO(g_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/heyisun/Documents/playground/cpp/CppServer/bin/conf/log2.yml");
    // YAML::Node root = YAML::LoadFile("/root/log2.yml");
    CppServer::Config::LoadFromYaml(root);

    std::vector<CppServer::Thread::ptr> thrs;
    for (int i = 0; i < 5; ++i) {
        CppServer::Thread::ptr thr(new CppServer::Thread(&func2, "name_" + std::to_string(i * 2)));
        CppServer::Thread::ptr thr2(new CppServer::Thread(&func3, "name_" + std::to_string(i * 2 + 1)));
        thrs.push_back(thr);
        thrs.push_back(thr2);
    }

    for (size_t i = 0; i < thrs.size(); ++i) {
        thrs[i]->join();
    }
    CPPSERVER_LOG_INFO(g_logger) << "thread test end";
    CPPSERVER_LOG_INFO(g_logger) << "count =" << count;
    return 0;
}
