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

#if defined(ARCH_OS_LINUX) && defined(ARCH_CPU_INTEL)
#include <x86intrin.h>
#elif defined(ARCH_OS_DARWIN)
#include <mach/mach_time.h>
#elif defined(ARCH_OS_WINDOWS)
#include <intrin.h>
#endif

#include <algorithm>
#include <atomic>
#include <iterator>
#include <numeric>

PXR_NAMESPACE_OPEN_SCOPE

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
#if defined(ARCH_OS_DARWIN)
    // On Darwin we'll use mach_absolute_time().
    return mach_absolute_time();
#elif defined(ARCH_CPU_INTEL)
    // On Intel we'll use the rdtsc instruction.
    return __rdtsc();
#elif defined (ARCH_CPU_ARM)
    uint64_t result;
    __asm __volatile("mrs	%0, CNTVCT_EL0" : "=&r" (result));
    return result;
#else
#error Unknown architecture.
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
#if defined (ARCH_OS_DARWIN)
    return ArchGetTickTime();
#elif defined (ARCH_CPU_ARM)
    std::atomic_signal_fence(std::memory_order_seq_cst);
    asm volatile("mrs %0, cntvct_el0" : "=r"(t));
    std::atomic_signal_fence(std::memory_order_seq_cst);
#elif defined (ARCH_COMPILER_MSVC)
    _mm_lfence();
    std::atomic_signal_fence(std::memory_order_seq_cst);
    t = __rdtsc();
    _mm_lfence();
    std::atomic_signal_fence(std::memory_order_seq_cst);
#elif defined(ARCH_CPU_INTEL) && \
    (defined(ARCH_COMPILER_CLANG) || defined(ARCH_COMPILER_GCC))
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
#error "Unsupported architecture."
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
#if defined (ARCH_OS_DARWIN)
    return ArchGetTickTime();
#elif defined (ARCH_CPU_ARM)
    std::atomic_signal_fence(std::memory_order_seq_cst);
    asm volatile("mrs %0, cntvct_el0" : "=r"(t));
    std::atomic_signal_fence(std::memory_order_seq_cst);
#elif defined (ARCH_COMPILER_MSVC)
    std::atomic_signal_fence(std::memory_order_seq_cst);
    unsigned aux;
    t = __rdtscp(&aux);
    _mm_lfence();
    std::atomic_signal_fence(std::memory_order_seq_cst);
#elif defined(ARCH_CPU_INTEL) && \
    (defined(ARCH_COMPILER_CLANG) || defined(ARCH_COMPILER_GCC))
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
#error "Unsupported architecture."
#endif
  return t;
}

#if defined (doxygen) ||                                                       \
    (!defined(ARCH_OS_DARWIN) && defined(ARCH_CPU_INTEL) &&                    \
     (defined(ARCH_COMPILER_CLANG) || defined(ARCH_COMPILER_GCC)))

/// A simple timer class for measuring an interval of time using the
/// ArchTickTimer facilities.
struct ArchIntervalTimer
{
    /// Construct a timer and start timing if \p start is true.
    explicit ArchIntervalTimer(bool start=true)
        : _started(start) {
        if (_started) {
            Start();
        }
    }

    /// Start the timer, or reset the start time if it has already been started.
    void Start() {
        _started = true;
        std::atomic_signal_fence(std::memory_order_seq_cst);
        asm volatile(
            "lfence\n\t"
            "rdtsc\n\t"
            "lfence"
            : "=a"(_startLow), "=d"(_startHigh) :: );
    }

    /// Return true if this timer is started.
    bool IsStarted() const {
        return _started;
    }

    /// Return this timer's start time, or 0 if it hasn't been started.
    uint64_t GetStartTicks() const {
        return (uint64_t(_startHigh) << 32) + _startLow;
    }
 
    /// Read and return the current time.
    uint64_t GetCurrentTicks() {
        return ArchGetStopTickTime();
    }

    /// Read the current time and return the difference between it and the start
    /// time.  If the timer was not started, return 0.
    uint64_t GetElapsedTicks() {
        if (!_started) {
            return 0;
        }
        uint32_t stopLow, stopHigh;
        std::atomic_signal_fence(std::memory_order_seq_cst);
        asm volatile(
            "rdtscp\n\t"
            "lfence"
            : "=a"(stopLow), "=d"(stopHigh)
            :
            // rdtscp writes rcx
            : "rcx");
        return ((uint64_t(stopHigh) << 32) + stopLow) -
            ((uint64_t(_startHigh) << 32) + _startLow);
    }
private:
    bool _started = false;
    uint32_t _startLow = 0, _startHigh = 0;
};

#else

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

#endif

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


/// Convert a duration measured in "ticks", as returned by
/// \c ArchGetTickTime(), to nanoseconds.
///
/// An example to test the timing routines would be:
/// \code
///     ArchIntervalTimer iTimer;
///     sleep(10);
///
///     // duration should be approximately 10/// 1e9 = 1e10 nanoseconds.
///     int64_t duration = ArchTicksToNanoseconds(iTimer.GetElapsedTicks());
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
