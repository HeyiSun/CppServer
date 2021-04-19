#include "CppServer/tcp_server.h"
#include "CppServer/log.h"

#include <sys/uio.h>

static CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

class EchoServer : public CppServer::TcpServer {
 public:
    EchoServer(int type);
    virtual void handleClient(CppServer::Socket::ptr client);

 private:
    int m_type = 0;
};

EchoServer::EchoServer(int type)
    : m_type {type} {
}

void EchoServer::handleClient(CppServer::Socket::ptr client) {
    CPPSERVER_LOG_INFO(g_logger) << "handleClient " << *client;
    std::string buffer;
    while (true) {
        buffer.resize(4096);
        int rt = client->recv(&buffer[0], buffer.size());
        if (rt == 0) {
            CPPSERVER_LOG_INFO(g_logger) << "client close: " << *client;
            break;
        } else if (rt < 0) {
            CPPSERVER_LOG_ERROR(g_logger) << "client error rt=" << rt
                << " errno=" << errno << " errstr=" << strerror(errno);
            break;
        }
        if (rt > 4096) {
            CPPSERVER_LOG_ERROR(g_logger) << "Received too much";
            break;
        }
        buffer.resize(rt);

        if (m_type == 1) {
            CPPSERVER_LOG_INFO(g_logger) << buffer;
            client->send(&buffer[0], buffer.size());
        } else {
            CPPSERVER_LOG_INFO(g_logger) << "Don't support binary type";
        }
    }
}

int type = 1;

void run() {
    EchoServer::ptr es(new EchoServer(type));
    auto addr = CppServer::Address::LookupAny("0.0.0.0:8020");
    while (!es->bind(addr)) {
        sleep(2);
    }
    es->start();
}

int main(int argc, char** argv) {
    if (argc < 2) {
        CPPSERVER_LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 1;
    }
    if (!strcmp(argv[1], "-b")) {
        type = 2;
    }
    CppServer::IOManager iom(2);
    iom.schedule(run);
    return 0;
}
