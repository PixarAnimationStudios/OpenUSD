//
// Copyright 2018 Pixar
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

#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"

#include "pxr/base/tf/stopwatch.h"

#include <iostream>
#include <vector>
#include <fenv.h>

PXR_NAMESPACE_USING_DIRECTIVE

static void
WriteStats(FILE *file, const char *name, const TfStopwatch &timer)
{
    fprintf(
        file,
        "{'profile':'%s','metric':'time','value':%f,'samples':%zu}\n",
        name,
        timer.GetSeconds(),
        timer.GetSampleCount());
}

#if defined(ARCH_OS_WINDOWS)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__ ((noinline))
#endif

// Make the loops call this add function so the compiler doesn't unroll the loop
// differently for the different cases.
int NOINLINE Add(int a, int b)
{
    return a + b;
}

int NOINLINE TestEmpty(int N) 
{
    int i = 0;
    for (int x = 0; x < N; x++) {
        i = Add(i, x);
    }
    return i;
}

int NOINLINE TestTraceScope(int N) 
{
    int i = 0;
    for (int x = 0; x < N; x++) {        
        TRACE_SCOPE("foo");
        i = Add(i, x);
    }
    return i;
}

int NOINLINE TestTick(int N) 
{
    int i = 0;
    for (int x = 0; x < N; x++) {        
        // A TRACE_SCOPE has to do two of these, one for begin and one for end.
        ArchGetTickTime();
        i = Add(i, x);
        ArchGetTickTime();
    }
    return i;
}

int NOINLINE TestPushBack(int N, std::vector<uint64_t>& tickVec) 
{
    int i = 0;
    for (int x = 0; x < N; x++) {        
        // A TRACE_SCOPE has to do two of these, one for begin and one for end.
        tickVec.push_back(ArchGetTickTime());
        i = Add(i, x);
        tickVec.push_back(ArchGetTickTime());
    }
    return i;
}

int
main(int argc, char *argv[])
{
    FILE *statsFile = fopen("perfstats.raw", "w");

    TfStopwatch watch;
    int i = 0;
    int N = 1e8;
    double traceDisableTime, traceTime, noTraceTime, tickTime, pushTickTime;
    std::vector<uint64_t> tickVec;

    TraceCollector* _col = &TraceCollector::GetInstance();

    _col->SetEnabled(false);
    //
    // TraceScope
    //
    watch.Start();
    i = TestTraceScope(N);
    watch.Stop();

    // Print out the sum in i so that the compiler doesn't try to optimize
    // it out.
    std::cout << "i=" << i << std::endl;
    WriteStats(statsFile, "trace_disabled", watch);
    traceDisableTime = watch.GetSeconds();
    watch.Reset();
    
    _col->SetEnabled(true);

    //
    // TraceScope
    //
    watch.Start();
    i = TestTraceScope(N);
    watch.Stop();

    // Print out the sum in i so that the compiler doesn't try to optimize
    // it out.
    std::cout << "i=" << i << std::endl;
    WriteStats(statsFile, "trace_enabled", watch);
    
    traceTime = watch.GetSeconds();
    watch.Reset();


    //
    // ArchGetTickTime()
    //
    watch.Start();
    i = TestTick(N);
    watch.Stop();
    std::cout << "i=" << i << std::endl;
    tickTime = watch.GetSeconds();
    watch.Reset();

    //
    // vector.push_back(ArchGetTickTime())
    //
    watch.Start();
    i = TestPushBack(N, tickVec);
    watch.Stop();
    std::cout << "i=" << i << std::endl;
    pushTickTime = watch.GetSeconds();
    watch.Reset();

    //
    // Nothing
    //
    watch.Start();
    i = TestEmpty(N);
    watch.Stop();
    std::cout << "i=" << i << std::endl;
    noTraceTime = watch.GetSeconds();
    watch.Reset();

    // 
    // Print Report
    //
    std::cout << "Time (untimed): " << noTraceTime << std::endl;
    std::cout << "Time (TraceScope disabled): " << traceDisableTime << std::endl;
    std::cout << "Time (TraceScope): " << traceTime << std::endl;
    std::cout << "Time (ArchGetTickTime): " << tickTime << std::endl;
    std::cout << "Time (PushBack): " << pushTickTime << std::endl;


    std::cout << "Cost per disabled scope(ns): " 
        << (traceDisableTime - noTraceTime) / double(N) * 1e9 << std::endl;
    std::cout << "Cost per scope(ns): "     
        << (traceTime - noTraceTime) / double(N) * 1e9 << std::endl;
    std::cout << "Cost per tickTime(ns): "  
        << (tickTime - noTraceTime) / double(N) * 1e9 << std::endl;
    std::cout << "Cost per push_back(ns): " 
        << (pushTickTime - noTraceTime) / double(N) * 1e9 << std::endl;
    

    double diffTick = (traceTime - tickTime) / tickTime * 100.0;
    double diffDisabled = 
        (traceDisableTime - noTraceTime) / noTraceTime  * 100.0;
    std::cout << "trace %diff untimed: "   << diffDisabled << std::endl;
    std::cout << "trace %diff tick: "      << diffTick << std::endl;
    std::cout << "trace %diff push_back: " 
        << (traceTime - pushTickTime) / pushTickTime * 100.0 << std::endl;

    fprintf(
        statsFile,
        "{'profile':'trace_vs_tick','metric':'time','value':%f,'samples':1}\n",
        diffTick);
    fprintf(
        statsFile,
        "{'profile':'disabled_overhead','metric':'time','value':%f,'samples':1}\n",
        diffDisabled);

    fclose(statsFile);

    return 0;
}
