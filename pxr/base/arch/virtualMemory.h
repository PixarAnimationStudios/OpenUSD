//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

/// Memory protection options, see ArchSetMemoryProtection().
enum ArchMemoryProtection {
    ArchProtectNoAccess,
    ArchProtectReadOnly,
    ArchProtectReadWrite,
    ArchProtectReadWriteCopy
};

/// Change the memory protection on the pages containing \p start and \p start +
/// \p numBytes to \p protection.  Return true if the protection is changed
/// successfully.  Return false in case of an error; check errno.  This function
/// rounds \p start to the nearest lower page boundary.  On POSIX systems,
/// ArchProtectReadWrite and ArchProtectReadWriteCopy are the same, on Windows
/// they differ but the Windows API documentation does not make it clear what
/// using ReadWrite means for a private file-backed mapping.
ARCH_API bool
ArchSetMemoryProtection(void const *start, size_t numBytes,
                        ArchMemoryProtection protection);
    
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_VIRTUAL_MEMORY_H
