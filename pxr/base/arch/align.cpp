// Copyright 2022 Pixar
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
#include "pxr/base/arch/error.h"

#if defined(ARCH_OS_OSX)
#   include <sys/malloc.h>
#elif !defined(ARCH_OS_IOS)
#   include <malloc.h>
#endif /* defined(ARCH_OS_MACOS) */

#include <cstdlib>

PXR_NAMESPACE_OPEN_SCOPE

/// Aligned memory allocation.
void *
ArchAlignedAlloc(size_t alignment, size_t size)
{
#if defined(ARCH_OS_DARWIN) || (defined(ARCH_OS_LINUX) && defined(__GLIBCXX__) && !defined(_GLIBCXX_HAVE_ALIGNED_ALLOC))
    // alignment must be >= sizeof(void*)
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }

    void *pointer;
    if (posix_memalign(&pointer, alignment, size) == 0) {
        return pointer;
    }

    return nullptr;
#elif defined(ARCH_OS_WINDOWS)
    return _aligned_malloc(size, alignment);
#else
    return aligned_alloc(alignment, size);
#endif
}

/// Free memory allocated by ArchAlignedAlloc.
void
ArchAlignedFree(void* ptr)
{
#if defined(ARCH_OS_DARWIN) || (defined(ARCH_OS_LINUX) && defined(__GLIBCXX__) && !defined(_GLIBCXX_HAVE_ALIGNED_ALLOC))
    free(ptr);
#elif defined(ARCH_OS_WINDOWS)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
