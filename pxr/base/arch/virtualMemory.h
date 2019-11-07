//
// Copyright 2019 Pixar
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
#ifndef PXR_BASE_ARCH_VIRTUAL_MEMORY_H
#define PXR_BASE_ARCH_VIRTUAL_MEMORY_H

/// \file arch/virtualMemory.h
/// \ingroup group_arch_SystemFunctions
/// Architecture dependent routines for virtual memory.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

/// Reserve \p numBytes bytes of virtual memory.  Call ArchCommitVirtualMemory()
/// on subranges to write to and read from the memory.  Return nullptr in case
/// of an error; check errno.
ARCH_API void *
ArchReserveVirtualMemory(size_t numBytes);

/// Make the range of \p numBytes bytes starting at \p start available for
/// reading and writing.  The range must be within one previously reserved by
/// ArchReserveVirtualMemory().  It is not an error to commit a range that was
/// previously partly or fully committed.  Return false in case of an error;
/// check errno.
ARCH_API bool
ArchCommitVirtualMemoryRange(void *start, size_t numBytes);

/// Return memory obtained with ArchReserveVirtualMemory() to the system.  The
/// \p start argument must be the value returned from a previous call to
/// ArchReserveVirtualMemory, and \p numBytes must match the argument from that
/// call.  Memory within the range may not be accessed after this call.  Return
/// false in case of an error; check errno.
ARCH_API bool
ArchFreeVirtualMemory(void *start, size_t numBytes);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_VIRTUAL_MEMORY_H
