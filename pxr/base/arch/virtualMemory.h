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
///
/// The functions here report failure by return value.  On POSIX systems the
/// underlying mmap(), mprotect() or munmap() call also sets errno; on Windows
/// the underlying VirtualAlloc(), VirtualProtect() or VirtualFree() call
/// reports through GetLastError() and leaves errno untouched.
///
/// Ranges are handled at page granularity: these functions operate on every
/// page that [\p start, \p start + \p numBytes) touches, and on no others.
/// Subranges of a single reservation are therefore independent only where their
/// boundaries are page-aligned; two subranges that share a page are always
/// committed and protected together.  Use ArchGetPageSize() to reason about
/// such boundaries when needed.  Alignment is key, not just size: a page-sized
/// range starting mid-page touches two pages.
///
/// \p numBytes must be nonzero throughout: zero is not handled consistently
/// across platforms.

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

/// Reserve \p numBytes bytes of virtual memory.  The memory is inaccessible
/// until ArchCommitVirtualMemoryRange() is called on subranges to write to and
/// read from.  Return nullptr in case of an error.
ARCH_API void *
ArchReserveVirtualMemory(size_t numBytes);

/// Make the range of \p numBytes bytes starting at \p start available for
/// reading and writing.  The range must be within one previously reserved by
/// ArchReserveVirtualMemory().  Newly committed memory reads as zero.  It is
/// not an error to commit a range that was previously partly or fully
/// committed, and doing so preserves the contents of the part that was.  Return
/// false in case of an error.
ARCH_API bool
ArchCommitVirtualMemoryRange(void *start, size_t numBytes);

/// Return memory obtained with ArchReserveVirtualMemory() to the system.  The
/// \p start argument must be the value returned from a previous call to
/// ArchReserveVirtualMemory(), and \p numBytes must match the argument from
/// that call.  The whole reservation is freed, committed subranges included.
/// Memory within the range may not be accessed after this call.  Return false
/// in case of an error.
///
/// A mismatched \p numBytes is not diagnosed consistently: POSIX either fails
/// or unmaps the wrong range, while Windows ignores \p numBytes and releases
/// the whole reservation regardless.
ARCH_API bool
ArchFreeVirtualMemory(void *start, size_t numBytes);

/// Memory protection options, see ArchSetMemoryProtection().
///
/// ArchProtectReadWriteCopy applies only to a private file-backed mapping.  On
/// POSIX it is identical to ArchProtectReadWrite; on Windows it maps to
/// PAGE_WRITECOPY, which is meaningful for a view of a file mapping opened with
/// FILE_MAP_COPY and is not applicable to memory from
/// ArchReserveVirtualMemory().
enum ArchMemoryProtection {
    ArchProtectNoAccess,
    ArchProtectReadOnly,
    ArchProtectReadWrite,
    ArchProtectReadWriteCopy
};

/// Change the memory protection on the pages containing \p start through \p
/// start + \p numBytes to \p protection.  Return true if the protection is
/// changed successfully.  Return false in case of an error.  Changing
/// protection does not disturb the contents of the pages, and in particular
/// ArchProtectNoAccess does not release them.
ARCH_API bool
ArchSetMemoryProtection(void const *start, size_t numBytes,
                        ArchMemoryProtection protection);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_ARCH_VIRTUAL_MEMORY_H
