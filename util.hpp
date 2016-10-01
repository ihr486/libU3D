#pragma once

#include <iostream>
#include <exception>
#include <sstream>
#include <string>

namespace U3D
{

class Error : public std::exception
{
    std::string msg;
    std::ostringstream stream;
public:
    Error(const char *filename, int line) {
        stream << filename << ":" << line << ":";
    }
    Error(const Error& old) : msg(old.stream.str()) {
    }
    virtual const char *what() const throw() {
        return msg.c_str();
    }
    template<typename T> Error& operator<<(const T& obj) {
        stream << obj;
        return *this;
    }
    virtual ~Error() throw() {}
};

template<typename T> class AutoHandle
{
    T ptr;
    void (*deleter)(T);
public:
    AutoHandle(T ptr, void (*deleter)(T)) : ptr(ptr), deleter(deleter) {}
    ~AutoHandle() {
        deleter(ptr);
    }
    operator T() { return ptr; }
    operator bool() { return ptr != NULL; }
    T& operator*() { return *ptr; }
    T operator->() { return ptr; }
private:
    AutoHandle(const AutoHandle& old);
    void operator=(const AutoHandle& old);
};

#define U3D_LOG (std::cout << __FILE__ ":" << __LINE__ << ":")
#define U3D_WARNING (std::cerr << __FILE__ ":" << __LINE__ << ":")
#define U3D_ERROR (U3D::Error(__FILE__, __LINE__))

}
