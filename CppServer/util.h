#ifndef __CPPSERVER_UTIL_H__
#define __CPPSERVER_UTIL_H__

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdint>


namespace CppServer {

pid_t GetThreadId();

uint32_t GetFiberId();


}  // CppServer

#endif  // __CPPSERVER_UTIL_H__
