#ifndef __CPPSERVER_NONCOPYABLE_H__
#define __CPPSERVER_NONCOPYABLE_H__

namespace CppServer {

class Noncopyable {
public:
    Noncopyable() = default;
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

}


#endif  // __CPPSERVER_NONCOPYABLE_H__
