//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef ARCH_DEFINES_H
#define ARCH_DEFINES_H

/*
 * OS
 */

#if defined(__linux__)
#define ARCH_OS_LINUX
#elif defined(__APPLE__)
#include "TargetConditionals.h"
#define ARCH_OS_DARWIN
#if TARGET_OS_IPHONE
#define ARCH_OS_IOS
#else
#define ARCH_OS_OSX
#endif
#elif defined(_WIN32) || defined(_WIN64)
#define ARCH_OS_WINDOWS
#endif

/*
 * Processor
 */

#if defined(i386) || defined(__i386__) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
#define ARCH_CPU_INTEL
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM)
#define ARCH_CPU_ARM
#endif



/*
 * Bits
 */

#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64)
#define ARCH_BITS_64
#else
#error "Unsupported architecture.  x86_64 or ARM64 required."
#endif



/*
 * Compiler
 */

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
#define ARCH_COMPILER_MSVC_VERSION	_MSC_VER
#endif



/*
 * Features
 */
// XXX -- This is an interim solution during the port to C++11.  We want to
//        use pure C++11 so we don't want to use the __typeof__ extension.
//        Once we don't require backward compatibility we can find and fix
//        anyplace using this macro.
//
//        __typeof__ drops references while decltype preserves them and
//        __typeof__ returns the type of the given expression, not its
//        declared type.  We emulate that behavior by converting the
//        expression to an lvalue by adding a pair of parentheses around
//        it then stripping any reference.  Arch_TypeOfRemoveReference is
//        just std::remove_reference but without adding a #include.  Note
//        that typename outside of a template is valid C++11.
//
//        Note that this is a macro taking an argument:  you can't pass
//        an expression with a comma in it!  However, since we're going
//        to add parentheses around the expression anyway, it's okay for
//        clients to do that too to get past the preprocessor.

#ifdef __cplusplus
template <typename T> struct Arch_TypeOfRemoveReference { typedef T type; };
template <typename T> struct Arch_TypeOfRemoveReference<T&> { typedef T type; };
#define ARCH_TYPEOF(x) typename Arch_TypeOfRemoveReference<decltype((x))>::type
#elif defined(ARCH_OS_WINDOWS)
#define ARCH_TYPEOF(x) decltype(x)
#else
// This extension is widely supported so fallback to it.
#define ARCH_TYPEOF __typeof__
#endif

// The current version of Apple clang does not support the thread_local
// keyword.
#if !(defined(ARCH_OS_DARWIN) && defined(ARCH_COMPILER_CLANG))
#define ARCH_HAS_THREAD_LOCAL
#define ARCH_COMPILER_HAS_STATIC_ASSERT
#endif

// The MAP_POPULATE flag for mmap calls only exists on Linux platforms.
#if defined(ARCH_OS_LINUX)
#define ARCH_HAS_MMAP_MAP_POPULATE
#endif

// Some static asserts are not supported due to difference in size types.
#if defined(ARCH_OS_LINUX)
    #define ARCH_COMPILER_HAS_STATIC_ASSERT
#endif

#endif // ARCH_DEFINES_H 
