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
#ifndef PXR_BASE_ARCH_PRAGMAS_H
#define PXR_BASE_ARCH_PRAGMAS_H

/// \file arch/pragmas.h
/// Pragmas for controlling compiler-specific behaviors.
///
/// This header contains pragmas used to control compiler-specific behaviors.
/// Behaviors that are not supported or required by a certain compiler should
/// be implemented as a no-op.

#include "pxr/base/arch/defines.h"

#if defined(ARCH_COMPILER_GCC)

    #define ARCH_PRAGMA_PUSH \
        _Pragma("GCC diagnostic push")

    #define ARCH_PRAGMA_POP \
        _Pragma("GCC diagnostic pop")

    // Convert errors about variables that may be used before initialization
    // into warnings.
    //
    // This works around GCC bug 47679.
    #define ARCH_PRAGMA_MAYBE_UNINITIALIZED \
        _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")

    #define ARCH_PRAGMA_MACRO_REDEFINITION \
        _Pragma("GCC diagnostic ignored \"-Wbuiltin-macro-redefined\"")

    #define ARCH_PRAGMA_WRITE_STRINGS \
        _Pragma("GCC diagnostic ignored \"-Wwrite-strings\"")

#elif defined(ARCH_COMPILER_CLANG)

    #define ARCH_PRAGMA_PUSH \
        _Pragma("clang diagnostic push")

    #define ARCH_PRAGMA_POP \
        _Pragma("clang diagnostic pop")

    #define ARCH_PRAGMA_MACRO_REDEFINITION \
        _Pragma("clang diagnostic ignored \"-Wbuiltin-macro-redefined\"")

    #define ARCH_PRAGMA_UNDEFINED_VAR_TEMPLATE \
        _Pragma("clang diagnostic ignored \"-Wundefined-var-template\"")

    #define ARCH_PRAGMA_WRITE_STRINGS \
        _Pragma("clang diagnostic ignored \"-Wwrite-strings\"")

    #define ARCH_PRAGMA_INSTANTIATION_AFTER_SPECIALIZATION \
        _Pragma("clang diagnostic ignored \"-Winstantiation-after-specialization\"")

    #define ARCH_PRAGMA_INSTANCE_METHOD_NOT_FOUND \
        _Pragma("clang diagnostic ignored \"-Wobjc-method-access\"")

#elif defined(ARCH_COMPILER_MSVC)

    #define ARCH_PRAGMA_PUSH \
        __pragma(warning(push)) 

    #define ARCH_PRAGMA_POP \
        __pragma(warning(pop)) 

    #define ARCH_PRAGMA_MACRO_TOO_FEW_ARGUMENTS \
        __pragma(warning(disable:4003)) 

    #define ARCH_PRAGMA_MACRO_REDEFINITION \
        __pragma(warning(disable:4005)) 

    #define ARCH_PRAGMA_QUALIFIER_HAS_NO_MEANING \
        __pragma(warning(disable:4180)) 

    #define ARCH_PRAGMA_ZERO_SIZED_STRUCT \
        __pragma(warning(disable:4200)) 

    #define ARCH_PRAGMA_NEEDS_EXPORT_INTERFACE \
        __pragma(warning(disable:4251)) 

    #define ARCH_PRAGMA_CONVERSION_FROM_SIZET \
        __pragma(warning(disable:4267)) 

    #define ARCH_PRAGMA_MAY_NOT_BE_ALIGNED \
        __pragma(warning(disable:4316)) 

    #define ARCH_PRAGMA_SHIFT_TO_64_BITS \
        __pragma(warning(disable:4334)) 

    #define ARCH_PRAGMA_DESTRUCTOR_IMPLICIT_DEFINE \
        __pragma(warning(disable:4624))

    #define ARCH_PRAGMA_DEPRECATED_POSIX_NAME \
        __pragma(warning(disable:4996)) 

    #define ARCH_PRAGMA_FORCING_TO_BOOL \
        __pragma(warning(disable:4800)) 

    #define ARCH_PRAGMA_UNSAFE_USE_OF_BOOL \
        __pragma(warning(disable:4804)) 

    #define ARCH_PRAGMA_UNARY_MINUS_ON_UNSIGNED \
        __pragma(warning(disable:4146)) 

#endif

#if !defined ARCH_PRAGMA_PUSH
    #define ARCH_PRAGMA_PUSH
#endif

#if !defined ARCH_PRAGMA_POP
    #define ARCH_PRAGMA_POP
#endif

#if !defined ARCH_PRAGMA_MAYBE_UNINITIALIZED
    #define ARCH_PRAGMA_MAYBE_UNINITIALIZED
#endif

#if !defined ARCH_PRAGMA_MACRO_REDEFINITION
    #define ARCH_PRAGMA_MACRO_REDEFINITION
#endif

#if !defined ARCH_PRAGMA_WRITE_STRINGS
    #define ARCH_PRAGMA_WRITE_STRINGS
#endif

#if !defined ARCH_PRAGMA_MACRO_TOO_FEW_ARGUMENTS
    #define ARCH_PRAGMA_MACRO_TOO_FEW_ARGUMENTS
#endif

#if !defined ARCH_PRAGMA_QUALIFIER_HAS_NO_MEANING
    #define ARCH_PRAGMA_QUALIFIER_HAS_NO_MEANING
#endif

#if !defined ARCH_PRAGMA_ZERO_SIZED_STRUCT
    #define ARCH_PRAGMA_ZERO_SIZED_STRUCT
#endif

#if !defined ARCH_PRAGMA_NEEDS_EXPORT_INTERFACE
    #define ARCH_PRAGMA_NEEDS_EXPORT_INTERFACE
#endif

#if !defined ARCH_PRAGMA_CONVERSION_FROM_SIZET
    #define ARCH_PRAGMA_CONVERSION_FROM_SIZET
#endif

#if !defined ARCH_PRAGMA_MAY_NOT_BE_ALIGNED
    #define ARCH_PRAGMA_MAY_NOT_BE_ALIGNED
#endif

#if !defined ARCH_PRAGMA_SHIFT_TO_64_BITS
    #define ARCH_PRAGMA_SHIFT_TO_64_BITS
#endif

#if !defined ARCH_PRAGMA_DESTRUCTOR_IMPLICIT_DEFINE
    #define ARCH_PRAGMA_DESTRUCTOR_IMPLICIT_DEFINE
#endif

#if !defined ARCH_PRAGMA_DEPRECATED_POSIX_NAME
    #define ARCH_PRAGMA_DEPRECATED_POSIX_NAME
#endif

#if !defined ARCH_PRAGMA_FORCING_TO_BOOL
    #define ARCH_PRAGMA_FORCING_TO_BOOL
#endif

#if !defined ARCH_PRAGMA_UNSAFE_USE_OF_BOOL
    #define ARCH_PRAGMA_UNSAFE_USE_OF_BOOL
#endif

#if !defined ARCH_PRAGMA_UNARY_MINUS_ON_UNSIGNED
    #define ARCH_PRAGMA_UNARY_MINUS_ON_UNSIGNED
#endif

#if !defined ARCH_PRAGMA_INSTANTIATION_AFTER_SPECIALIZATION
    #define ARCH_PRAGMA_INSTANTIATION_AFTER_SPECIALIZATION
#endif

#if !defined ARCH_PRAGMA_UNDEFINED_VAR_TEMPLATE
    #define ARCH_PRAGMA_UNDEFINED_VAR_TEMPLATE
#endif

#if !defined ARCH_PRAGMA_INSTANCE_METHOD_NOT_FOUND
    #define ARCH_PRAGMA_INSTANCE_METHOD_NOT_FOUND
#endif

#endif // PXR_BASE_ARCH_PRAGMAS_H
