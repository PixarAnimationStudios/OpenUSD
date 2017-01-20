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
#ifndef ARCH_ATTRIBUTES_H
#define ARCH_ATTRIBUTES_H

/// \file arch/attributes.h
/// Define function attributes.
///
/// This file allows you to define architecture-specific or compiler-specific
/// options to be used outside lib/arch.

#include "pxr/pxr.h"
#include "pxr/base/arch/export.h"

PXR_NAMESPACE_OPEN_SCOPE

#if defined(doxygen)

/// Macro used to indicate a function takes a printf-like specification.
///
/// This attribute is used as follows:
/// \code
///    void PrintFunc(T1 arg1, T2 arg2, const char* fmt, ...)
///        ARCH_PRINTF_FUNCTION(3, 4)
/// \endcode
/// This indicates that the third argument is the format string, and that the
/// fourth argument is where the var-args corresponding to the format string begin.
///
/// \hideinitializer
#   define ARCH_PRINTF_FUNCTION(_fmt, _firstArg)

/// Macro used to indicate a function takes a scanf-like specification.
///
/// This attribute is used as follows:
/// \code
///    void ScanFunc(T1 arg1, T2 arg2, const char* fmt, ...)
///        ARCH_PRINTF_FUNCTION(3, 4)
/// \endcode
/// This indicates that the third argument is the format string, and
/// that the fourth argument is where the var-args corresponding to the
/// format string begin.
///
/// \hideinitializer
#   define ARCH_SCANF_FUNCTION(_fmt, _firstArg)

/// Macro used to indicate that a function should never be inlined.
///
/// This attribute is used as follows:
/// \code
///    void Func(T1 arg1, T2 arg2) ARCH_NOINLINE;
/// \endcode
///
/// \hideinitializer
#   define ARCH_NOINLINE

/// Macro used to indicate a function parameter may be unused.
///
/// In general, avoid this attribute if possible.  Mostly this attribute
/// should be used when the set of arguments to a function is described
/// as part of a macro.  The usage is:
/// \code
///    void Func(T1 arg1, ARCH_UNUSED_ARG T2 arg2, ARCH_UNUSED_ARG T3 arg3, T4 arg4) {
///        ...
///    }
/// \endcode
///
/// \hideinitializer
#   define ARCH_UNUSED_ARG

/// Macro used to indicate that a function's code must always be emitted even
/// if not required.
///
/// This attribute is especially useful with templated registration functions,
/// which might not be present in the linked binary if they are not used (or
/// the compiler optimizes away their use.)
///
/// The usage is:
/// \code
/// template <typename T>
/// struct TraitsClass {
///    static void RegistryFunction() ARCH_USED_FUNCTION {
///        ...
///    }
/// };
/// \endcode
///
/// \hideinitializer
#   define ARCH_USED_FUNCTION

/// Macro to begin the definition of a function that should be executed by
/// the dynamic loader when the dynamic object (library or program) is
/// loaded.
///
/// \p _priority is used to order the execution of constructors.  Valid
/// values are integers in the range [0,255].  Constructors with lower
/// numbers are run first.  It is unspecified if these functions are run
/// before or after dynamic initialization of non-local variables.
///
/// \p _name is the name of the function and the remaining arguments should
/// be types for the signature of the function.  The types are only to make
/// the name unique (when mangled);  the function will be called with no
/// arguments so the arguments must not be used.  If you don't need any
/// arguments you must use void.
///
/// \hideinitializer
#   define ARCH_CONSTRUCTOR(_name, _priority, ...)

/// Macro to begin the definition of a function that should be executed by
/// the dynamic loader when the dynamic object (library or program) is
/// unloaded.
///
/// \p _priority is used to order the execution of destructors.  Valid
/// values are integers in the range [0,255].  Destructors with higher
/// numbers are run first.  It is unspecified if these functions are run
/// before or after dynamically initialized non-local variables.
///
/// \p _name is the name of the function and the remaining arguments should
/// be types for the signature of the function.  The types are only to make
/// the name unique (when mangled);  the function will be called with no
/// arguments so the arguments must not be used.  If you don't need any
/// arguments you must use void.
///
/// \hideinitializer
#   define ARCH_DESTRUCTOR(_name, _priority, ...)

#elif defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)

#   define ARCH_PRINTF_FUNCTION(_fmt, _firstArg) \
        __attribute__((format(printf, _fmt, _firstArg)))
#   define ARCH_SCANF_FUNCTION(_fmt, _firstArg)	\
        __attribute__((format(scanf, _fmt, _firstArg)))
#   define ARCH_NOINLINE __attribute__((noinline))
#   define ARCH_UNUSED_ARG   __attribute__ ((unused))
#   define ARCH_USED_FUNCTION __attribute__((used))

#elif defined(ARCH_COMPILER_MSVC)

#   include <SAL.h>
#   define ARCH_PRINTF_FUNCTION(_fmt, _firstArg) \
        _Printf_format_string_
#   define ARCH_SCANF_FUNCTION(_fmt, _firstArg)	\
        _Printf_format_string_
#   define ARCH_NOINLINE // __declspec(noinline)
#   define ARCH_UNUSED_ARG
#   define ARCH_USED_FUNCTION

#else

// Leave macros undefined so we'll fail to build on a new system/compiler
// rather than fail mysteriously at runtime.

#endif

#if defined(doxygen)

    // The macros are already defined above in doxygen.

#elif defined(ARCH_OS_DARWIN)

    // Entry for a constructor/destructor in the custom section.
    struct Arch_ConstructorEntry {
        typedef void (*Type)(void);
        Type function;
        unsigned int version:24;    // USD version
        unsigned int priority:8;    // Priority of function
    };

    // Emit a Arch_ConstructorEntry in the __Data,pxrctor section.
#   define ARCH_CONSTRUCTOR(_name, _priority, ...) \
        static void _name(__VA_ARGS__); \
        static const Arch_ConstructorEntry arch_ctor_ ## _name \
	        __attribute__((used, section("__DATA,pxrctor"))) = { \
	    reinterpret_cast<Arch_ConstructorEntry::Type>(&_name), \
            0u, \
            _priority \
        }; \
	static void _name(__VA_ARGS__)

    // Emit a Arch_ConstructorEntry in the __Data,pxrdtor section.
#   define ARCH_DESTRUCTOR(_name, _priority, ...) \
        static void _name(__VA_ARGS__); \
        static const Arch_ConstructorEntry arch_dtor_ ## _name \
	        __attribute__((used, section("__DATA,pxrdtor"))) = { \
	    reinterpret_cast<Arch_ConstructorEntry::Type>(&_name), \
            0u, \
            _priority \
        }; \
	static void _name(__VA_ARGS__)

#elif defined(ARCH_OS_WINDOWS)

#    include "pxr/base/arch/api.h"

    // Entry for a constructor/destructor in the custom section.
    __declspec(align(16))
    struct Arch_ConstructorEntry {
        typedef void (__cdecl *Type)(void);
        Type function;
        unsigned int version:24;    // USD version
        unsigned int priority:8;    // Priority of function
    };

    // Declare the special sections.
#   pragma section(".pxrctor", read)
#   pragma section(".pxrdtor", read)

    // Emit a Arch_ConstructorEntry in the .pxrctor section.  The namespace
    // and extern are to convince the compiler and linker to leave the object
    // in the final library/executable instead of stripping it out.  In
    // clang/gcc we use __attribute__((used)) to do that.
#   define ARCH_CONSTRUCTOR(_name, _priority, ...) \
        static void _name(__VA_ARGS__); \
        namespace { \
        __declspec(allocate(".pxrctor")) \
        extern const Arch_ConstructorEntry arch_ctor_ ## _name = { \
	    reinterpret_cast<Arch_ConstructorEntry::Type>(&_name), \
            0u, \
            _priority \
        }; \
        } \
        static void _name(__VA_ARGS__)

    // Emit a Arch_ConstructorEntry in the .pxrdtor section.
#   define ARCH_DESTRUCTOR(_name, _priority, ...) \
        static void _name(__VA_ARGS__); \
        namespace { \
        __declspec(allocate(".pxrdtor")) \
        extern const Arch_ConstructorEntry arch_dtor_ ## _name = { \
	    reinterpret_cast<Arch_ConstructorEntry::Type>(&_name), \
            0u, \
            _priority \
        }; \
        } \
        static void _name(__VA_ARGS__)

    // Objects of this type run the ARCH_CONSTRUCTOR and ARCH_DESTRUCTOR
    // functions for the library containing the object in the c'tor and
    // d'tor, respectively.  Each HMODULE is handled at most once.
    struct Arch_ConstructorInit {
        ARCH_API Arch_ConstructorInit();
        ARCH_API ~Arch_ConstructorInit();
    };

    // Ensure we run constructor/destructors for this library.  We only
    // need one of these per library so we use selectany.  The pragma
    // prevents optimization from discarding the symbol.
    extern "C" {
    __pragma(comment(linker, "/include:_arch_constructor_init"))
    __declspec(selectany)
    Arch_ConstructorInit _arch_constructor_init;
    }

#elif defined(ARCH_COMPILER_GCC)

    // The used attribute is required to prevent these apparently unused
    // functions from being removed by the linker.
#   define ARCH_CONSTRUCTOR(_name, _priority, ...) \
        __attribute__((used, constructor((_priority) + 100))) \
        static void _name(__VA_ARGS__)
#   define ARCH_DESTRUCTOR(_name, _priority, ...) \
        __attribute__((used, destructor((_priority) + 100))) \
        static void _name(__VA_ARGS__)

#else

// Leave macros undefined so we'll fail to build on a new system/compiler
// rather than fail mysteriously at runtime.

#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // ARCH_ATTRIBUTES_H
