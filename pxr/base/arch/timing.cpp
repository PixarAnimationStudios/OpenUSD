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

#include "pxr/pxr.h"
#include "pxr/base/arch/timing.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/export.h"

#include <atomic>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>
#include <type_traits>
#include <thread>

#if defined(ARCH_OS_LINUX)
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#elif defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <chrono>
#endif

PXR_NAMESPACE_OPEN_SCOPE

template <class T>
constexpr static T UninitializedState = -1;

template <class T>
constexpr static T InitializingState = -2;

static std::atomic<double> 
Arch_NanosecondsPerTick{UninitializedState<double>};

// Tick measurement granularity.
static std::atomic<int64_t> 
Arch_TickQuantum{UninitializedState<int64_t>};

// Cost to take a measurement with ArchIntervalTimer.
static std::atomic<int64_t>  
Arch_IntervalTimerTickOverhead{UninitializedState<int64_t>};

template <typename T>
static
T
GetAtomicVar(std::atomic<T> &atomicVar, T (*computeVal)())
{
    T state = atomicVar.load(std::memory_order_relaxed);
    
    // Value has beeen initialized and can be read immediately.
    if (state >= 0) {
        return state;
    }

    // First thread that needs the value will start computing it.
    if (state == UninitializedState<T> && 
        atomicVar.compare_exchange_strong(state, InitializingState<T>)) {
        
        T newVal = computeVal();
        atomicVar.store(newVal,std::memory_order_relaxed);
        return newVal;
    }
    // Another thread is currently updating the value.
    // Block until value is ready.
    while (state < 0) {
        std::this_thread::yield();
        state = atomicVar.load(std::memory_order_relaxed);
    }
    return state;
}


#if defined(ARCH_OS_DARWIN)

static
double
Arch_ComputeNanosecondsPerTick()
{
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    return static_cast<double>(info.numer) / info.denom;
}

#elif defined(ARCH_OS_LINUX)

static
double
Arch_ComputeNanosecondsPerTick()
{
#if defined(ARCH_CPU_ARM)
    uint64_t counter_hz;
    __asm __volatile("mrs	%0, CNTFRQ_EL0" : "=&r" (counter_hz));
    Arch_NanosecondsPerTick = double(1e9) / double(counter_hz);
#else

    // Measure how long it takes to call ::now().
    uint64_t nowDuration =
        ArchMeasureExecutionTime(std::chrono::steady_clock::now);

    auto clockStart = std::chrono::steady_clock::now();
    ArchIntervalTimer itimer;
    std::this_thread::sleep_for(std::chrono::milliseconds(6));
    const auto clockEnd = std::chrono::steady_clock::now();
    const auto ticks = itimer.GetElapsedTicks();

    const std::chrono::duration<double> clockSecs = clockEnd - clockStart;
    const double clockNanoSecs = clockSecs.count() * 1e9;

    // Subtract the tick timer overhead for the one measurement we made as well
    // as the overhead to call now() one time.
    return clockNanoSecs /
        double(ticks - ArchGetIntervalTimerTickOverhead() - nowDuration);
#endif
}
#elif defined(ARCH_OS_WINDOWS)

static
double
Arch_ComputeNanosecondsPerTick()
{
    // We want to use rdtsc so we need to find the frequency.  We run a
    // small sleep here to compute it using QueryPerformanceCounter()
    // which is independent of rdtsc.  So we wait for some duration using
    // QueryPerformanceCounter() (whose frequency we know) then compute
    // how many ticks elapsed during that time and from that the number
    // of ticks per nanosecond.
    LARGE_INTEGER qpcFreq, qpcStart, qpcEnd;
    QueryPerformanceFrequency(&qpcFreq);
    const auto delay = (qpcFreq.QuadPart >> 4); // 1/16th of a second.
    QueryPerformanceCounter(&qpcStart);
    const auto t1 = ArchGetTickTime();
    do {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        QueryPerformanceCounter(&qpcEnd);
    } while (qpcEnd.QuadPart - qpcStart.QuadPart < delay);
    QueryPerformanceCounter(&qpcEnd);
    const auto t2 = ArchGetTickTime();

    // Total time take during the loop in seconds.
    const auto durationInSeconds =
        static_cast<double>(qpcEnd.QuadPart - qpcStart.QuadPart) /
        qpcFreq.QuadPart;

    // Nanoseconds per tick.
    constexpr auto nanosPerSecond = 1.0e9;
    return nanosPerSecond * durationInSeconds / (t2 - t1);
}

#else    
#error Unknown architecture.
#endif

// A externally visible variable used only to ensure the compiler doesn't do
// certain optimizations we don't want in order to measure accurate times.
uint64_t testTimeAccum;

static
int64_t 
Arch_ComputeTickQuantum()
{
    constexpr int NumTrials = 64;
    uint64_t currMin = ~0;

    // Calculate the timer quantum.
    for (int trial = 0; trial != NumTrials; ++trial) {
        uint64_t times[] = {
            ArchGetTickTime(),
            ArchGetTickTime(),
            ArchGetTickTime(),
            ArchGetTickTime(),
            ArchGetTickTime()
        };
        for (int i = 0; i != std::extent<decltype(times)>::value - 1; ++i) {
            times[i] = times[i+1] - times[i];
        }
        currMin = std::min(currMin, *std::min_element(
            std::begin(times), std::prev(std::end(times))));
    }
    return static_cast<int64_t>(currMin);
}

uint64_t
ArchGetTickQuantum()
{
    return GetAtomicVar(Arch_TickQuantum, Arch_ComputeTickQuantum);
}

static
int64_t
Arch_ComputeIntervalTimerTickOverhead()
{
    uint64_t *escape = &testTimeAccum;
    return static_cast<int64_t>(ArchMeasureExecutionTime([escape]() {
        ArchIntervalTimer itimer;
        *escape = itimer.GetElapsedTicks();
    }));
}
uint64_t
ArchGetIntervalTimerTickOverhead()
{
    return GetAtomicVar(Arch_IntervalTimerTickOverhead, 
        Arch_ComputeIntervalTimerTickOverhead);
}


int64_t
ArchTicksToNanoseconds(uint64_t nTicks)
{
    return static_cast<int64_t>(
        std::llround(static_cast<double>(nTicks)*ArchGetNanosecondsPerTick()));
}

double
ArchTicksToSeconds(uint64_t nTicks)
{
    return double(ArchTicksToNanoseconds(nTicks)) / 1e9;
}

uint64_t
ArchSecondsToTicks(double seconds)
{
    return static_cast<uint64_t>(
        std::llround(1.0e9 * seconds / ArchGetNanosecondsPerTick()));
}

double 
ArchGetNanosecondsPerTick() 
{
    return GetAtomicVar(Arch_NanosecondsPerTick,           
        Arch_ComputeNanosecondsPerTick);
}

uint64_t
Arch_MeasureExecutionTime(uint64_t maxTicks, bool *reachedConsensus,
                          void const *m, uint64_t (*callM)(void const *, int))
{
    auto measureN = [m, callM](int nTimes) { return callM(m, nTimes); };
    
    // XXX pin to a certain cpu?  (not possible on macos)

    // Run 10 times upfront to estimate how many runs to include in each sample.
    uint64_t estTicksPer = ~0;
    for (int i = 10; i--; ) {
        estTicksPer = std::min(estTicksPer, measureN(1));
    }

    // We want the tick quantum noise to -> 0.1% or less of the total time.
    // Since measured times are +/- 1 quantum, we multiply by 2000 to get the
    // desired runtime, and from there figure number of iterations for a sample.
    const uint64_t minTicksPerSample = 2000 * ArchGetTickQuantum();
    const int sampleIters = (estTicksPer < minTicksPerSample)
        ? (minTicksPerSample + estTicksPer/2) / estTicksPer
        : 1;

    auto measureSample = [&measureN, sampleIters]() {
        return (measureN(sampleIters) + sampleIters/2) / sampleIters;
    };

    // Now fill the sample buffer.  We are looking for the median to be equal to
    // the minimum -- we consider this good consensus on the fastest time.  Then
    // iteratively discard the slowest and fastest samples, fill with new
    // samples and repeat.  If we fail to gain consensus after maxMicroseconds,
    // then just take the fastest median we saw.

    constexpr int NumSamples = 64;
    uint64_t sampleTimes[NumSamples];
    for (uint64_t &t: sampleTimes) {
        t = measureSample();
    }

    // Sanity... limit timing to 5 billion ticks
    if (maxTicks > 5.e9) {
        maxTicks = 5.e9;
    }
    
    ArchIntervalTimer limitTimer;

    uint64_t bestMedian = ~0;
    while (true) {
        std::sort(std::begin(sampleTimes), std::end(sampleTimes));
        // If the fastest is the same as the median, we have good consensus.
        if (sampleTimes[0] == sampleTimes[NumSamples/2]) {
            if (reachedConsensus) {
                *reachedConsensus = true;
            }
            return sampleTimes[0];
        }
        if (limitTimer.GetElapsedTicks() >= maxTicks) {
            // Time's up!
            break;
        }
        // Otherwise update best median, and resample.
        bestMedian = std::min(bestMedian, sampleTimes[NumSamples/2]);
        // Replace the slowest 1/3...
        for (int i = (NumSamples - NumSamples/3); i != NumSamples; ++i) {
            sampleTimes[i] = measureSample();
        }
        // ...and the very fastest.
        for (int i = 0; i != NumSamples/10; ++i) {
            sampleTimes[i] = measureSample();
        }
    } while (limitTimer.GetElapsedTicks() < maxTicks);

    // Unable to obtain consensus.  Take best median we saw.
    if (reachedConsensus) {
        *reachedConsensus = false;
    }

    return bestMedian;
}


PXR_NAMESPACE_CLOSE_SCOPE
