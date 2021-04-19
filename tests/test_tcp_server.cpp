#include "CppServer/tcp_server.h"
#include "CppServer/log.h"

static CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

void run() {
    CppServer::Address::ptr addr = CppServer::Address::LookupAny("0.0.0.0:8033");
    CppServer::Address::ptr addr2 = std::make_shared<CppServer::UnixAddress>("/tmp/unix_addr");
    CPPSERVER_LOG_INFO(g_logger) << *addr << " - " << *addr2;

    std::vector<CppServer::Address::ptr> addrs;
    addrs.push_back(addr);
    addrs.push_back(addr2);

    CppServer::TcpServer::ptr tcp_server(new CppServer::TcpServer);
    std::vector<CppServer::Address::ptr> fails;
    while (!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
}



int main(int argc, char** argv) {
    CppServer::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
