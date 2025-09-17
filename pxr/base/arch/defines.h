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
#if defined(__EMSCRIPTEN__)
#define ARCH_OS_WASM_VM
#elif defined(__linux__)
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
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || \
    defined(_M_ARM64)
#define ARCH_CPU_ARM
#endif

//
// Bits
//

#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64) || \
    defined(_M_ARM64) || defined(__wasm64__)
#define ARCH_BITS_64
#elif defined(__wasm32__)
#define ARCH_BITS_32
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

// The MAP_POPULATE flag for mmap calls only exists on Linux platforms.
#if defined(ARCH_OS_LINUX)
#define ARCH_HAS_MMAP_MAP_POPULATE
#endif

// When using MSVC, provide an easy way to detect whether the older
// "traditional" preprocessor is being used as opposed to the newer, more
// standards-conforming preprocessor. The traditional preprocessor may require
// custom versions of macros.
// See here for more detail about MSVC's preprocessors:
// https://learn.microsoft.com/en-us/cpp/preprocessor/preprocessor-experimental-overview
#if defined(ARCH_COMPILER_MSVC)
    #if !defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL
    #define ARCH_PREPROCESSOR_MSVC_TRADITIONAL
    #endif
#endif

#endif // PXR_BASE_ARCH_DEFINES_H 
