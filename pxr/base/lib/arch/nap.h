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
#ifndef ARCH_NAP_H
#define ARCH_NAP_H

/// \file arch/nap.h
/// \ingroup group_arch_Multithreading
/// Routines for very brief pauses in execution.

#include "pxr/base/arch/inttypes.h"
#if defined(ARCH_OS_WINDOWS)
#include <windows.h>
#endif

/// \addtogroup group_arch_Multithreading
///@{

/// Sleep for some number of centiseconds.
///
/// Sleep for \c n/100 seconds.  Note: if your intent is to simply yield the
/// processors, DO NOT call this with a value of zero (as one can do with
/// sginap()). Call \c ArchThreadYield() instead.
void ArchNap(size_t nhundredths);

/// Yield to the operating system thread scheduler.
///
/// Returns control to the operating system thread scheduler as a means of
/// temporarily suspending the calling thread.
void ArchThreadYield();

/// Pause execution of the current thread.
///
/// Pause execution of the current thread without returning control to the
/// operating system scheduler. This function can be used as a means of
/// gracefully spin waiting while potentially yielding CPU resouces to
/// hyper-threads.
inline void ArchThreadPause() {
#if defined (ARCH_CPU_INTEL) && defined(ARCH_COMPILER_GCC)
    __asm__ __volatile__ ("pause");
#elif defined(ARCH_OS_WINDOWS)
    YieldProcessor();
#else
#warning Unknown architecture. Pause instruction skipped.
#endif
}

///@}

#endif // ARCH_NAP_H
