#include "util.h"


namespace CppServer {

pid_t GetThreadId() {
    return syscall(SYS_gettid);
    // A more portable way: https://medium.com/hackernoon/c-telltales-pt-1-human-readable-thread-id-92caa554a35f
}

uint32_t GetFiberId() {
    return 0;
}


}  // CppServer
