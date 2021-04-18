#ifndef __CPPSERVER_MACRO_H__
#define __CPPSERVER_MACRO_H__

#include <string>
#include <cassert>
#include "CppServer/util.h"

#if defined __GNUC__ || defined __llvm__
#define CPPSERVER_LIKELY(x)    __builtin_expect(!!(x), 1)
#define CPPSERVER_UNLIKELY(x)  __builtin_expect(!!(x), 0)
#else
#define CPPSERVER_LIKELY(x)     (x)
#define CPPSERVER_UNLIKELY(x)   (x)
#endif

#define CPPSERVER_ASSERT(x) \
    if (CPPSERVER_UNLIKELY(!(x))) { \
        CPPSERVER_LOG_ERROR(CPPSERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << CppServer::BacktraceToString(100, 2, "       "); \
        assert(x); \
    }

#define CPPSERVER_ASSERT2(x, w) \
    if (CPPSERVER_UNLIKELY(!(x))) { \
        CPPSERVER_LOG_ERROR(CPPSERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << CppServer::BacktraceToString(100, 2, "       "); \
        assert(x); \
    }


#endif
