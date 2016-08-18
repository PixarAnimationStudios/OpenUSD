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
#ifndef ARCH_ALIGN_H
#define ARCH_ALIGN_H

/// \file arch/align.h
/// \ingroup group_arch_Memory
/// Provide architecture-specific memory-alignment information.

#if !defined(__cplusplus)
#error This include file can only be included in C++ programs.
#endif

#include "pxr/base/arch/defines.h"
#include <cstddef>
#include <cstdint>

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
#define ARCH_CACHE_LINE_SIZE 64

///@}

#endif	// ARCH_ALIGN_H 
