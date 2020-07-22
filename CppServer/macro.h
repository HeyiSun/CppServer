#ifndef __CPPSERVER_MACRO_H__
#define __CPPSERVER_MACRO_H__

#include <string>
#include <cassert>
#include "CppServer/util.h"

#define CPPSERVER_ASSERT(x) \
    if (!(x)) { \
        CPPSERVER_LOG_ERROR(CPPSERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" \
            << CppServer::BacktraceToString(100, 2, "       "); \
        assert(x); \
    }

#define CPPSERVER_ASSERT2(x, w) \
    if (!(x)) { \
        CPPSERVER_LOG_ERROR(CPPSERVER_LOG_ROOT()) << "ASSERTION: " #x \
            << "\n" << w \
            << "\nbacktrace:\n" \
            << CppServer::BacktraceToString(100, 2, "       "); \
        assert(x); \
    }


#endif
