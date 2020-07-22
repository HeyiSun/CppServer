#ifndef __CPPSERVER_UTIL_H__
#define __CPPSERVER_UTIL_H__

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstdint>
#include <vector>
#include <string>


namespace CppServer {

pid_t GetThreadId();

uint32_t GetFiberId();

// skip: ignore some layers, for example, self
void Backtrace(std::vector<std::string>& bt, int size, int skip = 1);
std::string BacktraceToString(int size, int skip = 2, const std::string& prefix = "");
}  // CppServer

#endif  // __CPPSERVER_UTIL_H__
