#include "CppServer/CppServer.h"
#include "CppServer/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

int sock = 0;

void test_fiber() {
    CPPSERVER_LOG_INFO(g_logger) << "test_fiber";
    sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "104.193.88.123", &addr.sin_addr.s_addr);

    int rt = connect(sock, (const sockaddr*) &addr, sizeof(addr));
    if (rt == 0) {
    } else if (errno == EINPROGRESS) {
        CppServer::IOManager::GetThis()->addEvent(sock, CppServer::IOManager::READ, []() {
            CPPSERVER_LOG_INFO(g_logger) << "read callback";
        });
        CppServer::IOManager::GetThis()->addEvent(sock, CppServer::IOManager::WRITE, []() {
            CPPSERVER_LOG_INFO(g_logger) << "write callback.... sock=" << sock;
            CppServer::IOManager::GetThis()->cancelEvent(sock, CppServer::IOManager::READ);
            close(sock);
        });
    } else {
        CPPSERVER_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}


void test1() {
    CppServer::IOManager iom(1, false);
    iom.schedule(&test_fiber);

}


CppServer::Timer::ptr s_timer;
void test_timer() {
    CppServer::IOManager iom(2);
    s_timer = iom.addTimer(1000, [](){
        CPPSERVER_LOG_INFO(g_logger) << "hello timer";
        static int i = 0;
        if (++i == 3) {
            // s_timer->cancel();
            s_timer->reset(2000, true);
        }
    }, true);
}
 

// 这种方式的问题：
// 因为use_caller是false, 主线程中的主协程不参与调度，那么主线程的t_scheduler是nullptr的，所以在主线程注册事件会导致注册的事件的schduler是null（因为addEvent用GetThis()为事件设置调度器)。如此，在triggerEvent时，用schduler调用函数的时候会段错误
// void test1() {
//     CppServer::IOManager iom(1, false);
//     iom.schedule(&test_fiber);
// 
//     sock = socket(AF_INET, SOCK_STREAM, 0);
//     fcntl(sock, F_SETFL, O_NONBLOCK);
// 
//     sockaddr_in addr;
//     memset(&addr, 0, sizeof(addr));
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(80);
//     inet_pton(AF_INET, "104.193.88.123", &addr.sin_addr.s_addr);
// 
//     int rt = connect(sock, (const sockaddr*) &addr, sizeof(addr));
//     std::cout << 1 << std::endl;
//     if (rt == 0) {
//     } else if (errno == EINPROGRESS) {
//         iom.addEvent(sock, CppServer::IOManager::READ, []() {
//             CPPSERVER_LOG_INFO(g_logger) << "read callback";
//         });
//         std::cout << 2 << std::endl;
//         iom.addEvent(sock, CppServer::IOManager::WRITE, [&iom]() {
//             CPPSERVER_LOG_INFO(g_logger) << "write callback.... sock=" << sock;
//             iom.cancelEvent(sock, CppServer::IOManager::READ);
//             close(sock);
//         });
//         std::cout << 3 << std::endl;
//     } else {
//         CPPSERVER_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
//     }
// }
// 

int main(int argc, char** argv) {
    // test1();
    test_timer();
    return 0;
}
