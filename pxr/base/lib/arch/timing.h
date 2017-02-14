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
#ifndef ARCH_TIMING_H
#define ARCH_TIMING_H

/// \file arch/timing.h
/// \ingroup group_arch_SystemFunctions
/// High-resolution, low-cost timing routines.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/inttypes.h"

/// \addtogroup group_arch_SystemFunctions
///@{

#if defined(ARCH_OS_LINUX)
#include <x86intrin.h>
#elif defined(ARCH_OS_DARWIN)
#include <mach/mach_time.h>
#elif defined(ARCH_OS_WINDOWS)
#include <intrin.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

/// Macro that defines the clocks per second
///
/// Unfortunately, Red Hat 7.1 does not define CLK_TCK correctly, so
/// ARCH_CLK_TCK is the only guaranteed way to get the value of CLK_TCK. The
/// value is currently the same on all of our platforms.
#define ARCH_CLK_TCK 100

/// Return the current time in system-dependent units.
///
/// The current time is returned as a number of "ticks", where each tick
/// represents some system-dependent amount of time.  The resolution of the
/// timing routines varies, but on all systems, it is well under one
/// microsecond.  The cost of this routine is in the tens of nanoseconds
/// on GHz class machines.
inline uint64_t
ArchGetTickTime()
{
#if defined(ARCH_OS_DARWIN)
    // On Darwin we'll use mach_absolute_time().
    return mach_absolute_time();
#elif defined(ARCH_OS_WINDOWS)
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count.QuadPart;
#elif defined(ARCH_OS_LINUX) && defined(ARCH_CPU_INTEL)
    // On Intel we'll use the rdtscp instruction.
    // For future reference: Linux stashes the core ID
    // into tsc_aux. May want to expose this to ensure
    // we haven't jumped cores during timing.
    uint32_t tsc_aux;
    return __rdtscp(&tsc_aux);
#else
#error Unknown architecture.
#endif
}

/// Convert a duration measured in "ticks", as returned by
/// \c ArchGetTickTime(), to nanoseconds.
///
/// An example to test the timing routines would be:
/// \code
///     int64_t t1 = ArchGetTickTime();
///     sleep(10);
///     int64_t t2 = ArchGetTickTime();
///
///     // duration should be approximately 10/// 1e9 = 1e10 nanoseconds.
///     int64_t duration = ArchTicksToNanoseconds(t2 - t1);
/// \endcode
///
ARCH_API
int64_t ArchTicksToNanoseconds(uint64_t nTicks);

/// Convert a duration measured in "ticks", as returned by
/// \c ArchGetTickTime(), to seconds.
ARCH_API
double ArchTicksToSeconds(uint64_t nTicks);

/// Convert a duration in seconds to "ticks", as returned by
/// \c ArchGetTickTime().
ARCH_API
uint64_t ArchSecondsToTicks(double seconds);
    
/// Get nanoseconds per tick. Useful when converting ticks obtained from
/// \c ArchTickTime()
ARCH_API
double ArchGetNanosecondsPerTick();

///@}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // ARCH_TIMING_H
