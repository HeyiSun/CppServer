#include "CppServer/address.h"
#include "CppServer/log.h"

CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

void test() {
    std::vector<CppServer::Address::ptr> addrs;
    bool v = CppServer::Address::Lookup(addrs, "www.baidu.com");
    if (!v) {
        CPPSERVER_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }
    for (size_t i = 0; i < addrs.size(); ++i) {
        CPPSERVER_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_iface() {
    std::multimap<std::string, std::pair<CppServer::Address::ptr, uint32_t>> results;
    bool y = CppServer::Address::GetInterfaceAddresses(results);
    if (!y) {
        CPPSERVER_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
    }
    for (auto& it : results) {
        CPPSERVER_LOG_INFO(g_logger) << it.first << " - " << it.second.first->toString() << " - " << it.second.second;
    }
}

void test_ipv4() {
    // CppServer::Address::ptr addr = CppServer::IPv4Address::Create("www.sylar.top");
    CppServer::Address::ptr addr = CppServer::IPv4Address::Create("127.0.0.8");
    if (addr) {
        CPPSERVER_LOG_INFO(g_logger) << addr->toString();
    }
}

int main(int argc, char** argv) {
    // test();
    // test_iface();
    test_ipv4();
    return 0;
}
