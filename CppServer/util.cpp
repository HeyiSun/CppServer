#include "util.h"
#include <execinfo.h>

#include "log.h"
#include "fiber.h"


namespace CppServer {

CppServer::Logger::ptr g_logger = CPPSERVER_LOG_NAME("system");

pid_t GetThreadId() {
    return syscall(SYS_gettid);
    // A more portable way: https://medium.com/hackernoon/c-telltales-pt-1-human-readable-thread-id-92caa554a35f
}

uint32_t GetFiberId() {
    return CppServer::Fiber::GetFiberId();
}

void Backtrace(std::vector<std::string>& bt, int size, int skip) {
    void** array = (void**) malloc((sizeof(void*) * size));
    size_t s = ::backtrace(array, size);

    char** strings = backtrace_symbols(array, s);
    if (strings == NULL) {
        CPPSERVER_LOG_ERROR(g_logger) << "backtrace_symbols error";
        return;
    }

    for (size_t i = skip; i < s; ++i) {
        bt.push_back(strings[i]);
    }
    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for (size_t i = 0; i < bt.size(); ++i) {
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}


}  // CppServer
