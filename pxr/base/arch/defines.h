//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_DEFINES_H
#define PXR_BASE_ARCH_DEFINES_H

//
// OS
//

#if defined(__linux__)
#define ARCH_OS_LINUX
#elif defined(__APPLE__)
#include "TargetConditionals.h"
#define ARCH_OS_DARWIN
#if TARGET_OS_IPHONE
// TARGET_OS_IPHONE refers to all iOS derivative platforms
// TARGET_OS_IOS refers to iPhone/iPad
// For now, we specialize for the umbrella TARGET_OS_IPHONE group
#define ARCH_OS_IPHONE
#else
#define ARCH_OS_OSX
#endif
#elif defined(_WIN32) || defined(_WIN64)
#define ARCH_OS_WINDOWS
#endif

//
// Processor
//

#if defined(i386) || defined(__i386__) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
#define ARCH_CPU_INTEL
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
#define ARCH_CPU_ARM
#endif

//
// Bits
//

#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64)
#define ARCH_BITS_64
#else
#error "Unsupported architecture.  x86_64 or ARM64 required."
#endif

//
// Compiler
//

#if defined(__clang__)
#define ARCH_COMPILER_CLANG
#define ARCH_COMPILER_CLANG_MAJOR __clang_major__
#define ARCH_COMPILER_CLANG_MINOR __clang_minor__
#define ARCH_COMPILER_CLANG_PATCHLEVEL __clang_patchlevel__
#elif defined(__GNUC__)
#define ARCH_COMPILER_GCC
#define ARCH_COMPILER_GCC_MAJOR __GNUC__
#define ARCH_COMPILER_GCC_MINOR __GNUC_MINOR__
#define ARCH_COMPILER_GCC_PATCHLEVEL __GNUC_PATCHLEVEL__
#elif defined(__ICC)
#define ARCH_COMPILER_ICC
#elif defined(_MSC_VER)
#define ARCH_COMPILER_MSVC
#define ARCH_COMPILER_MSVC_VERSION _MSC_VER
#endif

//
// Features
//

// Only use the GNU STL extensions on Linux when using gcc.
#if defined(ARCH_OS_LINUX) && defined(ARCH_COMPILER_GCC)
#define ARCH_HAS_GNU_STL_EXTENSIONS
#endif

// The current version of Apple clang does not support the thread_local
// keyword.
#if !(defined(ARCH_OS_DARWIN) && defined(ARCH_COMPILER_CLANG))
#define ARCH_HAS_THREAD_LOCAL
#endif

// The MAP_POPULATE flag for mmap calls only exists on Linux platforms.
#if defined(ARCH_OS_LINUX)
#define ARCH_HAS_MMAP_MAP_POPULATE
#endif

//
// Sanitizers
//

// For most compilers sanitizers are enabled with something similar to
// -fsanitize={address,thread,undefined}.
// But detecting if the compiler is currently tyring to make a sanitized build
// can vary depending on the compiler (or between versions of the compiler).
// The following checks will determine if the compiler is making a sanitized
// build and set a definition
//
// These definitions can be used to conditionally compile code where
// instrumentation from the sanitizer needs augmentation; for instance
// building a test for bad memory allocations when using address
// sanitizers. Such a test would produce a false-positive from
// address sanitizer at run-time resulting in a failed test
//
// The definitions will only be defined if the compiler is actually
// building with a specific sanitizer. The absense of a definition
// means the compiler is not building with that sanitizer.
#if defined(ARCH_COMPILER_CLANG)
    #if defined(__has_feature)
        #if __has_feature(address_sanitizer)
            #define ARCH_SANITIZE_ADDRESS
        #endif

        // Definitions for other sanitizers intentionally
        // omitted here

    #endif
#elif defined(ARCH_COMPILER_GCC)
    #if defined(__has_feature)
        #if __has_feature(address_sanitizer)
            #define ARCH_SANITIZE_ADDRESS
        #endif

        // Definitions for other sanitizers intentionally
        // omitted here

    #else
        #if defined(__SANITIZE_ADDRESS__)
            #define ARCH_SANITIZE_ADDRESS
        #endif

        // Definitions for other sanitizers intentionally
        // omitted here

    #endif
#elif defined(ARCH_COMPILER_MSVC)
    #if defined(__SANITIZE_ADDRESS__)
        #define ARCH_SANITIZE_ADDRESS
    #endif

    // Definitions for other sanitizers intentionally
    // omitted here
#endif

#endif // PXR_BASE_ARCH_DEFINES_H 
