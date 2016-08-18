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

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/api.h"

/// \addtogroup group_arch_SystemFunctions
///@{

#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
#include <x86intrin.h>
#elif defined(ARCH_OS_WINDOWS)
#include <intrin.h>
#endif

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
/// microsecond.  The cost of this routine is approximately 40 nanoseconds on
/// a 900 Mhz Pentium III Linux box, 300 nanoseconds on a 400 Mhz Sun, and 200
/// nanoseconds on a 250 Mhz SGI, and 200 nanoseconds on a 500 MHz PowerMac
/// G4.
///
/// On SGIs and Linux x86 and Apple PPC systems, the time is queried by
/// reading from a hardware register, which is very fast; on the SUNS, a
/// standard system call (gethrtime()) is used, which accounts for the
/// slowness of the operation.
///
/// By contrast, the standard system call \c gettimeofday() takes,
/// respectively, 540, 4900 nanoseconds, and 370 nanoseconds on the same
/// Linux, SGI and SUN workstations.
inline uint64_t
ArchGetTickTime()
{
// On Intel, we'll use the rdtsc instruction
#if defined(ARCH_CPU_INTEL)
    return __rdtsc();
#else
#error Unknown architecture.
#endif // defined(ARCH_CPU_INTEL)
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

#endif // ARCH_TIMING_H
