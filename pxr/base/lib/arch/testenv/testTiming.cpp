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
#include "pxr/base/arch/nap.h"
#include "pxr/base/arch/error.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define MINNAPTIME 4
#define NAPTIME 5
#define MAXNAPTIME 6

int main()
{
    uint64_t startTick, endTick, hSeconds;

    for(int i = 0; i < 20; i++) {
        startTick = ArchGetTickTime();
        ArchNap(NAPTIME);
        endTick = ArchGetTickTime();
        hSeconds = ArchTicksToNanoseconds(endTick - startTick) / 10000000;
       if(hSeconds < MINNAPTIME || hSeconds > MAXNAPTIME) {
            ARCH_ERROR("ArchTiming failed, possibly due to a "
                                  "process being swapped out.  Try running "
                                  "it again, and if does not fail "
                                  "consistently it's ok to ignore this.");
        }
    }
    
    ArchNap(0);

    uint64_t ticks = ArchGetTickTime();
    ARCH_AXIOM( (uint64_t) ArchTicksToNanoseconds(ticks) == 
        uint64_t(static_cast<double>(ticks)*ArchGetNanosecondsPerTick() + .5));

    double nanos = double(ArchTicksToNanoseconds(ticks)) / 1e9;
    double secs = ArchTicksToSeconds(ticks);
    double epsilon = 0.0001;
    ARCH_AXIOM( (nanos - epsilon <= secs) && (nanos + epsilon >= secs) );

    return 0;
}
