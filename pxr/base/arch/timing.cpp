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

#if defined(ARCH_OS_LINUX)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#elif defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <chrono>
#include <thread>
#endif

PXR_NAMESPACE_OPEN_SCOPE

static double Arch_NanosecondsPerTick = 1.0;

// Tick measurement granularity.
static uint64_t Arch_TickQuantum = ~0;
// Cost to take a measurement with ArchIntervalTimer.
static uint64_t Arch_IntervalTimerTickOverhead = ~0;

#if defined(ARCH_OS_DARWIN)

static
void
Arch_ComputeNanosecondsPerTick()
{
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    Arch_NanosecondsPerTick = static_cast<double>(info.numer) / info.denom;
}

#elif defined(ARCH_OS_LINUX)

static
void
Arch_ComputeNanosecondsPerTick()
{
#if defined(ARCH_CPU_ARM)
    uint64_t counter_hz;
    __asm __volatile("mrs	%0, CNTFRQ_EL0" : "=&r" (counter_hz));
    Arch_NanosecondsPerTick = double(1e9) / double(counter_hz);
#else
    // NOTE: Normally ifstream would be cleaner, but it causes crashes when
    //       used in conjunction with DSOs and the Intel Compiler.
    FILE *in;
    char linebuffer[1024];
    double cpuHz = 0.0;

    in = fopen("/proc/cpuinfo", "r");

    if (in) {

        while (fgets(linebuffer, sizeof(linebuffer), in)) {
            char* colon;

            if (strncmp(linebuffer, "bogomips", 8) == 0 &&
                (colon = strchr(linebuffer, ':'))) {

                colon++;
            
                // The bogomips line is Millions of Instructions / sec
                // So convert.
                // The magic 2.0 is from Red Hat, which corresponds
                // to modern Intel / AMD processors.
                cpuHz = 1e6 * atof(colon) / 2.0;
                break;
            }
        }
        
        fclose(in);
    }

    if (cpuHz == 0.0) {
        in = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
        if (in) {
            if (fgets(linebuffer, sizeof(linebuffer), in)) {
                // We just read the cpuspeed in kHz.  Convert.
                //
                cpuHz = 1000 * atof(linebuffer);
            }
           fclose(in);

        }
    }

    // If we could not find that file (the cpufreq driver is unavailable
    // for some reason, fall back to /proc/cpuinfo.
    //
    if (cpuHz == 0.0) {
        in = fopen("/proc/cpuinfo","r");

        if (!in) {
            // Note: ARCH_ERROR will abort the program
            //
            ARCH_ERROR("Cannot open /proc/cpuinfo");
        }

        while (fgets(linebuffer, sizeof(linebuffer), in)) {
            char* colon;
            if (strncmp(linebuffer, "cpu MHz", 7) == 0 &&
                (colon = strchr(linebuffer, ':'))) {
                colon++;

                // colon is pointing to a cpu speed in MHz.  Convert.
                cpuHz = 1e6 * atof(colon);

                break;
            }
        }

        fclose(in);

        if (cpuHz == 0.0) {
            ARCH_ERROR("Could not find 'cpu MHz' in /proc/cpuinfo");
        }
    }

    Arch_NanosecondsPerTick = double(1e9) / double(cpuHz);
    
#endif
}
#elif defined(ARCH_OS_WINDOWS)

static
void
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
    Arch_NanosecondsPerTick = nanosPerSecond * durationInSeconds / (t2 - t1);
}

#else    
#error Unknown architecture.
#endif

// A externally visible variable used only to ensure the compiler doesn't do
// certain optimizations we don't want in order to measure accurate times.
uint64_t testTimeAccum;

ARCH_HIDDEN
void
Arch_InitTickTimer()
{
    // Calculate Arch_NanosecondsPerTick.
    Arch_ComputeNanosecondsPerTick();

    constexpr int NumTrials = 64;
    
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
        Arch_TickQuantum = std::min(
            Arch_TickQuantum,
            *std::min_element(std::begin(times), std::prev(std::end(times))));
    }

    // Calculate the interval timer overhead (the time cost to take an interval
    // measurement).
    {
        uint64_t *escape = &testTimeAccum;
        Arch_IntervalTimerTickOverhead = ArchMeasureExecutionTime([escape]() {
            ArchIntervalTimer itimer;
            *escape = itimer.GetElapsedTicks();
        });
    }
}

uint64_t
ArchGetTickQuantum()
{
    return Arch_TickQuantum;
}

uint64_t
ArchGetIntervalTimerTickOverhead()
{
    return Arch_IntervalTimerTickOverhead;
}


int64_t
ArchTicksToNanoseconds(uint64_t nTicks)
{
    return static_cast<int64_t>(
        std::llround(static_cast<double>(nTicks)*Arch_NanosecondsPerTick));
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
    return Arch_NanosecondsPerTick;
}

uint64_t
Arch_MeasureExecutionTime(uint64_t maxMicroseconds, bool *reachedConsensus,
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

    // Sanity... limit timing to 5 seconds.
    if (maxMicroseconds > 5000000) {
        maxMicroseconds = 5000000;
    }
    
    const uint64_t maxTicks =
        ArchSecondsToTicks(double(maxMicroseconds) / 1.e6);
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
