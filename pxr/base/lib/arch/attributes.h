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

#include "pxr/base/arch/export.h"

/*!
 * \file attributes.h
 * \brief Define function attributes.
 *
 * This file allows you to define architecture-specific or compiler-specific
 * options to be used outside lib/arch.
 */

#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG) || \
    defined(doxygen)

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
#   define ARCH_PRINTF_FUNCTION(_fmt, _firstArg)	\
		__attribute__((format(printf, _fmt, _firstArg)))

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
#   define ARCH_SCANF_FUNCTION(_fmt, _firstArg)	\
		__attribute__((format(scanf, _fmt, _firstArg)))

/// Macro used to indicate that a function should never be inlined.
///
/// This attribute is used as follows:
/// \code
///    void Func(T1 arg1, T2 arg2) ARCH_NOINLINE;
/// \endcode
///
/// \hideinitializer
#   define ARCH_NOINLINE __attribute__((noinline))

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
#   define ARCH_UNUSED_ARG   __attribute__ ((unused))

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
#   define ARCH_USED_FUNCTION __attribute__((used))

/// Macro to indicate a function should be executed by the dynamic loader when
/// the dynamic object (library or program) is loaded.
///
/// The priority is used to order the execution of constructors.  Valid
/// values are integers over 100.  Constructors with lower numbers are
/// run first.  Constructors for C++ objects at global scope are run
/// after these functions regardless of priority.
///
/// \hideinitializer
#   define ARCH_CONSTRUCTOR_DEFINE(_priority, _name, ...)                   \
    __attribute__((constructor(_priority)))                                 \
    static void _name(__VA_ARGS__)

/// Macro to indicate a function should be executed by the dynamic loader when
/// the dynamic object (library or program) is unloaded.
///
/// The priority is used to order the execution of destructors.  Valid
/// values are integers over 100.  Destructors with higher numbers are
/// run first.
///
/// \hideinitializer
#   define ARCH_DESTRUCTOR(_priority, _name, ...)                           \
    __attribute__((destructor(_priority)))                                  \
    static void _name(__VA_ARGS__)

#elif defined(ARCH_COMPILER_MSVC)

#include <SAL.h>

///
/// \hideinitializer
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
#   define ARCH_PRINTF_FUNCTION(_fmt, _firstArg)	\
		_Printf_format_string_
///
/// \hideinitializer
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
#   define ARCH_SCANF_FUNCTION(_fmt, _firstArg)	\
		_Printf_format_string_

///
/// \hideinitializer
/// Macro used to indicate that a function should never be inlined
///
/// This attribute is used as follows:
/// \code
///    void Func(T1 arg1, T2 arg2) ARCH_NOINLINE;
/// \endcode
///
#   define ARCH_NOINLINE // __declspec(noinline)

///
/// \hideinitializer
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
#   define ARCH_UNUSED_ARG

///
/// Macro used to indicate that a function's code must always be emitted
///        even if not required.
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
#   define ARCH_USED_FUNCTION

///
/// ARCH_CONSTRUCTOR and ARCH_DESTRUCTOR are defined in a separate
///        file for Windows due to the amount of platform specific code
///        involved.
///
#include "pxr/base/arch/attributesWindows.h"

#else
#   define ARCH_PRINTF_FUNCTION(_fmt, _firstArg)
#   define ARCH_SCANF_FUNCTION(_fmt, _firstArg)
#   define ARCH_NOINLINE
#   define ARCH_UNUSED_ARG
// ARCH_USED_FUNCTION is deliberately omitted.  If a new compiler is deployed,
// we want the build to fail rather than generate executables that will fail
// at runtime in potentially mysterious ways. Similarly ARCH_CONSTRUCTOR and
// ARCH_DESTRUCTOR.
#endif // defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)

#endif // ARCH_ATTRIBUTES_H 
