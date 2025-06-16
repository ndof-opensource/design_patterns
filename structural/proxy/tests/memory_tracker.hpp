#ifndef PROXY_TESTS_MEMORY_TRACKER_HPP
#define PROXY_TESTS_MEMORY_TRACKER_HPP

#include <any>
#include <new>
#include <iostream>

template<typename T=void>
struct Tracker {
    static int allocs;
    void* operator new(std::size_t size) {
        ++allocs;
        return ::operator new(size);
    }
    void operator delete(void* p) noexcept {
        --allocs;
        ::operator delete(p);
    }
    int x;
};

template<typename T>
int Tracker<T>::allocs = 0;
#endif
