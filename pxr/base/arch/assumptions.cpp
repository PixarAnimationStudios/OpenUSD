//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/align.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/arch/math.h"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <limits>

#if defined(ARCH_OS_LINUX)
#include <unistd.h>
#elif defined(ARCH_OS_DARWIN)
#include <sys/sysctl.h>
#include <mach-o/arch.h>
#elif defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <memory>
#endif

PXR_NAMESPACE_OPEN_SCOPE

static size_t
Arch_ObtainCacheLineSize()
{
#if defined(ARCH_OS_LINUX)
    return sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
#elif defined(ARCH_OS_DARWIN)
    size_t cacheLineSize = 0;
    size_t cacheLineSizeSize = sizeof(cacheLineSize);
    sysctlbyname("hw.cachelinesize", &cacheLineSize, &cacheLineSizeSize, 0, 0);
    return cacheLineSize;
#elif defined(ARCH_OS_WINDOWS)
    DWORD bufferSize = 0;
    using INFO = SYSTEM_LOGICAL_PROCESSOR_INFORMATION;

    // Get the number of bytes required.
    ::GetLogicalProcessorInformation(nullptr, &bufferSize);

    // Get the count of structures
    size_t total = bufferSize / sizeof(INFO);

    // Allocate the array of processor INFOs.
    std::unique_ptr<INFO[]> buffer(new INFO[total]);

    size_t lineSize = 0;
    if (::GetLogicalProcessorInformation(&buffer[0], &bufferSize))
    {
        for (size_t current = 0; current != total; ++current)
        {
            if (buffer[current].Relationship == RelationCache &&
                1 == buffer[current].Cache.Level)
            {
                lineSize = buffer[current].Cache.LineSize;
                break;
            }
        }
    }

    return lineSize;
#else
#error Arch_ObtainCacheLineSize not implemented for OS.
#endif
}

ARCH_HIDDEN
void
Arch_ValidateAssumptions()
{
    enum SomeEnum { BLAH };

    /*
     * We do an atomic compare-and-swap on enum's, treating then as ints,
     * so we are assuming that an enum is the same size as an int.
     */
    static_assert(sizeof(SomeEnum) == sizeof(int),
                  "sizeof(enum) != sizeof(int)");

    /*
     * Assert that sizeof(int) is 4.
     */
    static_assert(sizeof(int) == 4, "sizeof(int) != 4");

    /*
     * Verify that float and double are IEEE-754 compliant.
     */
    static_assert(sizeof(float) == sizeof(uint32_t) &&
                  sizeof(double) == sizeof(uint64_t) &&
                  std::numeric_limits<float>::is_iec559 &&
                  std::numeric_limits<double>::is_iec559,
                  "float/double not IEEE-754 compliant");

    /*
     * Check the demangler on a very simple type.
     */
    if (ArchGetDemangled<int>() != "int") {
        ARCH_WARNING("C++ demangling appears badly broken.");
    }
    
    size_t cacheLineSize = Arch_ObtainCacheLineSize();

#if defined(ARCH_OS_DARWIN) && defined(ARCH_CPU_INTEL)
    /*
     * On MacOS with Rosetta 2, we may be an Intel x86_64 binary running on
     * an Apple Silicon arm64 cpu. macOS always returns the underlying
     * HW's cache line size, so we explicitly approve this exception here
     * by setting the detected cache line size to be what we expect.
     * This won't align, but the impact is one of performance. 
     * We don't really care about it because when this is happening, we're 
     * emulating x64_64 on arm64 which has a far greater performance impact.
     */
    const size_t ROSETTA_WORKAROUND_CACHE_LINE_SIZE = 128;
    NXArchInfo const* archInfo = NXGetLocalArchInfo();
    if (archInfo && ((archInfo->cputype & ~CPU_ARCH_MASK) == CPU_TYPE_ARM)) {
        if ((cacheLineSize != ROSETTA_WORKAROUND_CACHE_LINE_SIZE)) {
            ARCH_WARNING(
                "Cache-line size mismatch may negatively impact performance.");
        }
        cacheLineSize = ARCH_CACHE_LINE_SIZE;
    }
#endif

    /*
     * Make sure that the ARCH_CACHE_LINE_SIZE constant is set as expected
     * on the current hardware architecture.
     */ 
    if (ARCH_CACHE_LINE_SIZE != cacheLineSize) {
        ARCH_WARNING("ARCH_CACHE_LINE_SIZE != Arch_ObtainCacheLineSize()");
    }

    /*
     * Make sure that the machine is little-endian.  We do not support
     * big-endian machines.
     */
    {
        uint32_t check;
        char buf[] = { 1, 2, 3, 4 };
        memcpy(&check, buf, sizeof(check));
        if (check != 0x04030201) {
            ARCH_ERROR("Big-endian byte order not supported.");
        }
    }    
}

PXR_NAMESPACE_CLOSE_SCOPE
