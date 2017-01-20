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

#if defined(ARCH_OS_LINUX)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#elif defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

static double Arch_NanosecondsPerTick = 1.0;

#if defined(ARCH_OS_DARWIN)

ARCH_HIDDEN
void
Arch_InitTickTimer()
{
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    Arch_NanosecondsPerTick = static_cast<double>(info.numer) / info.denom;
}

#elif defined(ARCH_OS_LINUX)

ARCH_HIDDEN
void
Arch_InitTickTimer()
{
    // NOTE: Normally ifstream would be cleaner, but it causes crashes when
    //       used in conjunction with DSOs and the Intel Compiler.
    FILE *in;
    char linebuffer[1024];
    double cpuHz = 0.0;

    in = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
    if (in) {
	if (fgets(linebuffer, sizeof(linebuffer), in)) {
	    // We just read the cpuspeed in kHz.  Convert.
	    //
	    cpuHz = 1000 * atof(linebuffer);
	}
	fclose(in);
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
		(colon = strchr(linebuffer, ':')))
	    {
		colon++;

		// colon is pointing to a cpu speed in MHz.  Convert.
		//
		cpuHz = 1e6 * atof(colon);

		break;
	    }
	}

	fclose(in);

	if (cpuHz == 0.0) {
	    ARCH_ERROR("Could not find 'cpu MHz' in /proc/cpuinfo");
	}
    }

    Arch_NanosecondsPerTick = 1e9 / cpuHz;
}
#elif defined(ARCH_OS_WINDOWS)

ARCH_HIDDEN
void
Arch_InitTickTimer()
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    Arch_NanosecondsPerTick = 1e9 / freq.QuadPart;
}

#else    
#error Unknown architecture.
#endif

int64_t
ArchTicksToNanoseconds(uint64_t nTicks)
{
    return int64_t(static_cast<double>(nTicks)*Arch_NanosecondsPerTick + .5);
}

double
ArchTicksToSeconds(uint64_t nTicks)
{
    return double(ArchTicksToNanoseconds(nTicks)) / 1e9;
}

uint64_t
ArchSecondsToTicks(double seconds) {
    return static_cast<uint64_t>(1.0e9 * seconds / ArchGetNanosecondsPerTick());
}

double 
ArchGetNanosecondsPerTick() 
{
    return Arch_NanosecondsPerTick;
}

uint64_t
Arch_GetTickTime()
{
    return ArchGetTickTime();	// we're C++, so we don't get masked
}				// by the #define in timing.h

PXR_NAMESPACE_CLOSE_SCOPE
