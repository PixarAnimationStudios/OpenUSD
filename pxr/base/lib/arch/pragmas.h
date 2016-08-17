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
#ifndef ARCH_PRAGMAS_H
#define ARCH_PRAGMAS_H

/// \file arch/pragmas.h
/// Pragmas for controlling compiler-specific behaviors.
///
/// This header contains pragmas used to control compiler-specific behaviors.
/// Behaviors that are not supported or required by a certain compiler should
/// be implemented as a no-op.

#include "pxr/base/arch/defines.h"

#if defined(ARCH_COMPILER_GCC)

#define ARCH_PRAGMA_PUSH												\
	_Pragma("GCC diagnostic push"); 

#define ARCH_PRAGMA_RESTORE												\
	_Pragma("GCC diagnostic pop");

/// Convert errors about variables that may be used before initialization into
/// warnings. A warning is emitted, but won't cause a build failure with
/// -Werror enabled.
#define ARCH_PRAGMA_PUSH_NOERROR_MAYBE_UNINITIALIZED \
    ARCH_PRAGMA_PUSH													\
    _Pragma("GCC diagnostic warning \"-Wmaybe-uninitialized\"");

#define ARCH_PRAGMA_MACRO_REDEFINITION
	ARCH_PRAGMA_PUSH													\
	_Pragma("GCC diagnostic warning \"-Wno-builtin-macro-redefined\"");

#elif defined(ARCH_COMPILER_MSVC)

#define ARCH_PRAGMA_PUSH												\
	__pragma(warning(push)) 

#define ARCH_PRAGMA_RESTORE												\
	__pragma(warning(pop)) 

#define ARCH_PRAGMA_MACRO_TOO_FEW_ARGUMENTS								\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4003)) 

#define ARCH_PRAGMA_MACRO_REDEFINITION									\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4005)) 

#define ARCH_PRAGMA_QUALIFIER_HAS_NO_MEANING							\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4180)) 

#define ARCH_PRAGMA_ZERO_SIZED_STRUCT									\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4200)) 

#define ARCH_PRAGMA_NEEDS_EXPORT_INTERFACE								\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4251)) 

#define ARCH_PRAGMA_CONVERSION_FROM_SIZET								\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4267)) 

#define ARCH_PRAGMA_MAY_NOT_BE_ALIGNED									\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4316)) 

#define ARCH_PRAGMA_SHIFT_TO_64_BITS									\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4334)) 

#define ARCH_PRAGMA_DESTRUCTOR_IMPLICIT_DEFINE							\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4624))

#define ARCH_PRAGMA_DEPRECATED_POSIX_NAME								\
	ARCH_PRAGMA_PUSH													\
	__pragma(warning(disable:4996)) 

#endif

#if !defined ARCH_PRAGMA_MACRO_TOO_FEW_ARGUMENTS
#define		 ARCH_PRAGMA_MACRO_TOO_FEW_ARGUMENTS
#endif

#if !defined ARCH_PRAGMA_PUSH_NOERROR_MAYBE_UNINITIALIZED
#define		 ARCH_PRAGMA_PUSH_NOERROR_MAYBE_UNINITIALIZED
#endif

#if !defined ARCH_PRAGMA_MACRO_REDEFINITION
#define		 ARCH_PRAGMA_MACRO_REDEFINITION
#endif

#if !defined ARCH_PRAGMA_ZERO_SIZED_STRUCT
#define		 ARCH_PRAGMA_ZERO_SIZED_STRUCT
#endif

#if !defined ARCH_PRAGMA_NEEDS_EXPORT_INTERFACE
#define      ARCH_PRAGMA_NEEDS_EXPORT_INTERFACE
#endif

#if !defined ARCH_PRAGMA_CONVERSION_FROM_SIZET
#define      ARCH_PRAGMA_CONVERSION_FROM_SIZET
#endif

#if !defined ARCH_PRAGMA_MAY_NOT_BE_ALIGNED
#define      ARCH_PRAGMA_MAY_NOT_BE_ALIGNED
#endif

#if !defined ARCH_PRAGMA_SHIFT_TO_64_BITS
#define		 ARCH_PRAGMA_SHIFT_TO_64_BITS
#endif

#if !defined ARCH_PRAGMA_DESTRUCTOR_IMPLICIT_DEFINE
#define      ARCH_PRAGMA_DESTRUCTOR_IMPLICIT_DEFINE
#endif

#if !defined ARCH_PRAGMA_DEPRECATED_POSIX_NAME
#define		 ARCH_PRAGMA_DEPRECATED_POSIX_NAME
#endif

#endif /* ARCH_PRAGMAS_H */
