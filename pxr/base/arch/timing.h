//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_TIMING_H
#define PXR_BASE_ARCH_TIMING_H

/// \file arch/timing.h
/// \ingroup group_arch_SystemFunctions
/// High-resolution, low-cost timing routines.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/inttypes.h"

/// \addtogroup group_arch_SystemFunctions
///@{

// See if we should use the x86 TSC register for timing.
#if defined(PXR_ARCH_PREFER_TSC_TIMING) &&                              \
    defined(ARCH_OS_LINUX) &&                                           \
    defined(ARCH_CPU_INTEL) &&                                          \
    (defined(ARCH_COMPILER_CLANG) || defined(ARCH_COMPILER_GCC))
#define ARCH_USE_TSC_TIMING 1
#else
#define ARCH_USE_TSC_TIMING 0
#endif

#if ARCH_USE_TSC_TIMING
#include <x86intrin.h>
#endif

#include <atomic>
#include <chrono>
#include <cmath>

// XXX: None of <algorithm>, <iterator>, nor <numeric> are used by the timing
// routines, but the includes have been here forever and removing them causes
// other code that is not including all the headers it needs to fail to compile.
// These should be deprecated and removed.
#include <algorithm>
#include <iterator>
#include <numeric>

PXR_NAMESPACE_OPEN_SCOPE

// Use a std::chrono clock if we're not using the Intel RDTSC instruction
#if !ARCH_USE_TSC_TIMING
/// The std::chrono clock to use to measure time.
///
/// std::chrono::steady_clock appears to be fast and accurate on all of our
/// supported systems.
using Arch_TimingClock = std::chrono::steady_clock;
#endif

/// Return the current time in system-dependent units.
///
/// The current time is returned as a number of "ticks", where each tick
/// represents some system-dependent amount of time.  The resolution of the
/// timing routines varies, but on all systems, it is well under one
/// microsecond.  The cost of this routine is in the 10s-to-100s of nanoseconds
/// on GHz class machines.
inline uint64_t
ArchGetTickTime()
{
#if ARCH_USE_TSC_TIMING
    return __rdtsc();
#else
    return Arch_TimingClock::now().time_since_epoch().count();
#endif
}


/// Get a "start" tick time for measuring an interval of time, followed by a
/// later call to ArchGetStopTickTime().  Or see ArchIntervalTimer.  This is
/// like ArchGetTickTime but it includes compiler & CPU fencing & reordering
/// constraints in an attempt to get the best measurement possible.
inline uint64_t
ArchGetStartTickTime()
{
    uint64_t t;

#if ARCH_USE_TSC_TIMING

    // Prevent reorders by the compiler.
    std::atomic_signal_fence(std::memory_order_seq_cst);
    asm volatile(
        "lfence\n\t"
        "rdtsc\n\t"
        "shl $32, %%rdx\n\t"
        "or %%rdx, %0\n\t"
        "lfence"
        : "=a"(t)
        :
        // rdtsc writes rdx
        // shl modifies cc flags
        : "rdx", "cc");

#else

    std::atomic_signal_fence(std::memory_order_seq_cst);
    t = ArchGetTickTime();
    std::atomic_signal_fence(std::memory_order_seq_cst);

#endif

    return t;
}

/// Get a "stop" tick time for measuring an interval of time.  See
/// ArchGetStartTickTime() or ArchIntervalTimer.  This is like ArchGetTickTime
/// but it includes compiler & CPU fencing & reordering constraints in an
/// attempt to get the best measurement possible.
inline uint64_t
ArchGetStopTickTime()
{
    uint64_t t;

#if ARCH_USE_TSC_TIMING

    // Prevent reorders by the compiler.
    std::atomic_signal_fence(std::memory_order_seq_cst);
    asm volatile(
        "rdtscp\n\t"
        "shl $32, %%rdx\n\t"
        "or %%rdx, %0\n\t"
        "lfence"
        : "=a"(t)
        :
        // rdtscp writes rcx & rdx
        // shl modifies cc flags
        : "rcx", "rdx", "cc");

#else

    std::atomic_signal_fence(std::memory_order_seq_cst);
    t = ArchGetTickTime();
    std::atomic_signal_fence(std::memory_order_seq_cst);

#endif

    return t;
}

/// A simple timer class for measuring an interval of time using the
/// ArchTickTimer facilities.
struct ArchIntervalTimer
{
    explicit ArchIntervalTimer(bool start=true)
        : _started(start) {
        if (_started) {
            _startTicks = ArchGetStartTickTime();
        }
    }

    void Start() {
        _started = true;
        _startTicks = ArchGetStartTickTime();
    }

    bool IsStarted() const {
        return _started;
    }

    uint64_t GetStartTicks() const {
        return _startTicks;
    }

    uint64_t GetCurrentTicks() {
        return ArchGetStopTickTime();
    }

    uint64_t GetElapsedTicks() {
        if (!_started) {
            return 0;
        }
        return ArchGetStopTickTime() - _startTicks;
    }
private:
    bool _started = false;
    uint64_t _startTicks;
};

/// Return the tick time resolution.  Although the number of ticks per second
/// may be very large, on many current systems the tick timers do not update at
/// that rate.  Rather, sequential calls to ArchGetTickTime() may report
/// increases of 10s to 100s of ticks, with a minimum increment betwewen calls.
/// This function returns that minimum increment as measured at startup time.
///
/// Note that if this value is of sufficient size, then short times measured
/// with tick timers are potentially subject to significant noise.  In
/// particular, an interval of measured tick time is liable to be off by +/- one
/// ArchGetTickQuantum().
ARCH_API
uint64_t ArchGetTickQuantum();

/// Return the ticks taken to record an interval of time with ArchIntervalTimer,
/// as measured at startup time.
ARCH_API
uint64_t ArchGetIntervalTimerTickOverhead();

    
/// Get nanoseconds per tick. Useful when converting ticks obtained from
/// \c ArchTickTime()
#if defined(doxygen) || ARCH_USE_TSC_TIMING
ARCH_API
double ArchGetNanosecondsPerTick();
#else
inline double ArchGetNanosecondsPerTick()
{
    return 1e+9 * Arch_TimingClock::period::num / Arch_TimingClock::period::den;
}
#endif
/// Convert a duration measured in "ticks", as returned by
/// \c ArchGetTickTime(), to nanoseconds.
///
/// An example to test the timing routines would be:
/// \code
///     ArchIntervalTimer iTimer;
///     sleep(10);
///
///     // duration should be approximately 10 * 1e9 = 1e10 nanoseconds.
///     int64_t duration = ArchTicksToNanoseconds(iTimer.GetElapsedTicks());
/// \endcode
///
#if defined(doxygen) || ARCH_USE_TSC_TIMING
ARCH_API
int64_t ArchTicksToNanoseconds(uint64_t nTicks);
#else
inline int64_t ArchTicksToNanoseconds(uint64_t nTicks)
{
    return static_cast<int64_t>(
        std::llround(nTicks * ArchGetNanosecondsPerTick()));
}
#endif

/// Convert a duration measured in "ticks", as returned by
/// \c ArchGetTickTime(), to seconds.
#if defined(doxygen) || ARCH_USE_TSC_TIMING
ARCH_API
double ArchTicksToSeconds(uint64_t nTicks);
#else
inline double ArchTicksToSeconds(uint64_t nTicks)
{
    return nTicks * ArchGetNanosecondsPerTick() / 1e+9;
}
#endif

/// Convert a duration in seconds to "ticks", as returned by
/// \c ArchGetTickTime().
#if defined(doxygen) || ARCH_USE_TSC_TIMING
ARCH_API
uint64_t ArchSecondsToTicks(double seconds);
#else
inline uint64_t ArchSecondsToTicks(double seconds)
{
    return seconds * 1e+9 / ArchGetNanosecondsPerTick();
}
#endif

ARCH_API
uint64_t
Arch_MeasureExecutionTime(uint64_t maxTicks, bool *reachedConsensus,
                          void const *m, uint64_t (*callM)(void const *, int));

/// Run \p fn repeatedly attempting to determine a consensus fastest execution
/// time with low noise, for up to \p maxTicks, then return the consensus
/// fastest execution time.  If a consensus is not reached in that time, return
/// a best estimate instead.  If \p reachedConsensus is not null, set it to
/// indicate whether or not a consensus was reached.  This function ignores \p
/// maxTicks greater than 5 billion ticks and runs for up to 5 billion ticks
/// instead. The \p fn will run for an indeterminate number of times, so it 
/// should be side-effect free.  Also, it should do essentially the same work 
/// on every invocation so that timing its execution makes sense.
template <class Fn>
uint64_t
ArchMeasureExecutionTime(
    Fn const &fn,
    uint64_t maxTicks = 1e7,
    bool *reachedConsensus = nullptr)
{
    auto measureN = [&fn](int nTimes) -> uint64_t {
        ArchIntervalTimer iTimer;
        for (int i = nTimes; i--; ) {
            std::atomic_signal_fence(std::memory_order_seq_cst);
            (void)fn();
            std::atomic_signal_fence(std::memory_order_seq_cst);
        }
        return iTimer.GetElapsedTicks();
    };

    using MeasureNType = decltype(measureN);
    
    return Arch_MeasureExecutionTime(
        maxTicks, reachedConsensus,
        static_cast<void const *>(&measureN),
        [](void const *mN, int nTimes) {
            return (*static_cast<MeasureNType const *>(mN))(nTimes);
        });
}

///@}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_TIMING_H
