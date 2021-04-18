#include "CppServer/socket.h"
#include "CppServer/CppServer.h"

static CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

void test_socket() {
    CppServer::IPAddress::ptr addr = CppServer::Address::LookupAnyIPAddress("www.google.com");
    if (addr) {
        CPPSERVER_LOG_INFO(g_logger) << "get address: " << addr->toString();
    } else {
        CPPSERVER_LOG_ERROR(g_logger) << "get address fail";
    }

    CppServer::Socket::ptr sock = CppServer::Socket::CreateTCP(addr);
    addr->setPort(80);
    if (!sock->connect(addr)) {
        CPPSERVER_LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    } else {
        CPPSERVER_LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if (rt <= 0) {
        CPPSERVER_LOG_INFO(g_logger) << "send fail rt=" << rt;
        return;
    }
    CPPSERVER_LOG_INFO(g_logger) << "send rt=" << rt;
    
    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());
    if (rt <= 0) {
        CPPSERVER_LOG_INFO(g_logger) << "recv fail rt=" << rt;
        return;
    }
    CPPSERVER_LOG_INFO(g_logger) << rt << "    !!!";
    buffs.resize(rt);
    CPPSERVER_LOG_INFO(g_logger) << buffs;

}

int main(int argc, char** argv) {
    CppServer::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}
