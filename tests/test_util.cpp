#include "CppServer/CppServer.h"
#include <cassert>

CppServer::Logger::ptr g_logger = CPPSERVER_LOG_ROOT();

void test_assert() {
    CPPSERVER_LOG_INFO(g_logger) << CppServer::BacktraceToString(10);
    // CPPSERVER_ASSERT(false);
    CPPSERVER_ASSERT2(0 == 1, "abcdefg");
}

int main(int argc, char** argv) {
    assert(1);
    test_assert();
    return 0;
}
