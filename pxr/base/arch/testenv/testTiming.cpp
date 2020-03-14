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
#include "pxr/base/arch/error.h"
#include <chrono>
#include <thread>

PXR_NAMESPACE_USING_DIRECTIVE

int main()
{
    // Verify conversions for many tick counts.
    for (size_t ticks = 0ul; ticks != 1ul << 24u; ++ticks) {
        ARCH_AXIOM( (uint64_t) ArchTicksToNanoseconds(ticks) == 
            uint64_t(static_cast<double>(ticks)*ArchGetNanosecondsPerTick() + .5));

        double nanos = double(ArchTicksToNanoseconds(ticks)) / 1e9;
        double secs = ArchTicksToSeconds(ticks);
        double epsilon = 0.0001;
        ARCH_AXIOM( (nanos - epsilon <= secs) && (nanos + epsilon >= secs) );
    }

    // Compute some time delta.
    const auto t1 = ArchGetTickTime();
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    const auto t2 = ArchGetTickTime();
    const auto delta = t2 - t1;

    // Verify the delta is reasonable.  We allow a lot of leeway on the top
    // end in case of heavy machine load.
    ARCH_AXIOM(ArchTicksToSeconds(delta) > 1.4);
    ARCH_AXIOM(ArchTicksToSeconds(delta) < 5.0);

    return 0;
}
