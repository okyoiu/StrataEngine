#pragma once
#include <iostream>
#include <cstdlib>

#ifndef NDEBUG
    #define TESSERA_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                std::cerr << "Assertion Failed: " << #condition << "\n" \
                            << "Message: " << message << "\n" \
                            << "File: " << __FILE__ << ":" << __LINE__ << std::endl; \
                std::abort(); \
            } \
        } while(0)
    #define TESSERA_UNREACHABLE() \
    do { \
        std::cerr << "Unreachable code executed at " \
                << __FILE__ << ":" << __LINE__ << std::endl; \
        std::abort(); \
    } while(0)
#else 
    #define TESSERA_ASSERT(condition, message) do { (void)(condition); } while(0)
    #define TESSERA_UNREACHABLE() __builtin_unreachable()
#endif