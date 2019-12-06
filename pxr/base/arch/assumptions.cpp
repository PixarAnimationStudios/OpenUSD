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
#include "pxr/base/arch/align.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/export.h"
#include "pxr/base/arch/math.h"

#include <cstdio>
#include <limits>

#if defined(ARCH_OS_LINUX)
#include <unistd.h>
#elif defined(ARCH_OS_DARWIN)
#include <sys/sysctl.h>
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

    /*
     * Make sure that the ARCH_CACHE_LINE_SIZE constant is set as expected
     * on the current hardware architecture.
     */ 
    if (ARCH_CACHE_LINE_SIZE != Arch_ObtainCacheLineSize()) {
        ARCH_WARNING("ARCH_CACHE_LINE_SIZE != Arch_ObtainCacheLineSize()");
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
