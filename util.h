#ifndef UTIL_H
#define UTIL_H

#include <functional>
#include <stdexcept>
#include <string>

class scope_guard
{
public:
    typedef std::function<void()> handler;

    scope_guard(const handler& f): _f(f) {}
    ~scope_guard() { if (_f) _f(); }

    void cancel() { _f = nullptr; }

    scope_guard(const scope_guard&) = delete;
    void operator = (const scope_guard&) = delete;

private:
    handler _f;
};


class io_exception: public std::exception
{
    std::string _msg;

public:
    io_exception(const char* msg): _msg(msg)
    {}

    io_exception(const std::string& msg): _msg(msg)
    {}

    const char* what() const noexcept override {
        return _msg.c_str();
    }
};

#endif // UTIL_H
