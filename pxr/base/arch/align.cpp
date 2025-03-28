// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/align.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/error.h"

#if defined(ARCH_OS_IPHONE)
#elif defined(ARCH_OS_DARWIN)
#   include <sys/malloc.h>
#else
#   include <malloc.h>
#endif /* defined(ARCH_OS_IPHONE) */

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
