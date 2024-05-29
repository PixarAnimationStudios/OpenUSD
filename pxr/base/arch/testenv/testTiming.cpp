//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/timing.h"
#include "pxr/base/arch/error.h"
#include <chrono>
#include <thread>

PXR_NAMESPACE_USING_DIRECTIVE

int main()
{
    ARCH_AXIOM(ArchGetNanosecondsPerTick() > 0.0);
    // If you're not even doing 1 tick a second, it's probably a bogus value
    ARCH_AXIOM(ArchGetNanosecondsPerTick() < 1e9);
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
