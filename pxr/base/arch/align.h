//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_ARCH_ALIGN_H
#define PXR_BASE_ARCH_ALIGN_H

/// \file arch/align.h
/// \ingroup group_arch_Memory
/// Provide architecture-specific memory-alignment information.

#if !defined(__cplusplus)
#error This include file can only be included in C++ programs.
#endif

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"
#include "pxr/base/arch/defines.h"
#include <cstddef>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

/// \addtogroup group_arch_Memory
///@{

/// Return suitably aligned memory size.
///
/// Requests to \c malloc() or \c ::new for a given size are often rounded
/// upward.  Given a request for \c nBytes bytes of storage, this function
/// returns the amount that would actually be consumed by the system to
/// satisfy it. This is needed for efficient user-defined memory management.
///
inline size_t
ArchAlignMemorySize(size_t nBytes) {
    return (nBytes + 7) & (~0x7);
}

/// Maximum extra space needed for alignment.
///
/// The \c ArchAlignMemorySize() can increase the required memory by no more
/// than \c ARCH_MAX_ALIGNMENT_INCREASE.
///
/// \hideinitializer
#define ARCH_MAX_ALIGNMENT_INCREASE	7

/// Align memory to the next "best" alignment value.
///
/// This will take a pointer and bump it to the next ideal alignment boundary
/// that will work for all data types.
///
inline void *
ArchAlignMemory(void *base)
{
    return reinterpret_cast<void *>
	((reinterpret_cast<uintptr_t>(base) + 7) & ~0x7);
}

/// The size of a CPU cache line on the current processor architecture in bytes.
///
/// \hideinitializer
#if defined(ARCH_OS_DARWIN) && defined(ARCH_CPU_ARM)
#define ARCH_CACHE_LINE_SIZE 128
#else
#define ARCH_CACHE_LINE_SIZE 64
#endif

/// Aligned memory allocation.
ARCH_API
void *
ArchAlignedAlloc(size_t alignment, size_t size);

/// Free memory allocated by ArchAlignedAlloc.
ARCH_API
void
ArchAlignedFree(void* ptr);

///@}

PXR_NAMESPACE_CLOSE_SCOPE

#endif	// PXR_BASE_ARCH_ALIGN_H 
