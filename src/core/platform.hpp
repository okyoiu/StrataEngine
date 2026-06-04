#pragma once
#include <iostream>

/*  
    ========================================
    DETECTS THE PLATFORM (e.g. macOS, ARM64)
    AND DEFINES PLATFORM-SPECIFIC CONSTANTS
    LIKE CACHE LINE SIZE
    ========================================
*/

namespace tessera {

    // detecting the platform
    #if defined(__APPLE__) && defined(__MACH__) 
        #define TESSERA_PLATFORM_MACOS 1
    #endif

    #if defined(__aarch64__) || defined(_M_ARM64)
        #define TESSERA_ARCH_ARM64 1
    #endif

    // cache line size (64 bytes for Silicon)
    inline constexpr std::size_t TESSERA_CACHE_LINE_SIZE = 64;

    // alignment macro
    #define TESSERA_ALIGNAS(n) alignas(n)

    // prediction hints 
    #define TESSERA_LIKELY(x)   (!!(x)) [[likely]]
    #define TESSERA_UNLIKELY(x) (!!(x)) [[unlikely]]
}