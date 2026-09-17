//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/arch/virtualMemory.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/systemInfo.h"

#include <cstdint>
#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <memoryapi.h>
#else // Assume POSIX
#include <sys/mman.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

static uintptr_t
GetPageMask()
{
    const int pageSize = ArchGetPageSize();
    // Every address computation below derives from this mask, so a value that
    // is not a positive power of two would silently produce wrong addresses
    // rather than fail visibly.  ArchGetPageSize() documents power-of-two, but
    // it returns sysconf(_SC_PAGE_SIZE) narrowed to int on POSIX, which would
    // be -1 if sysconf() ever failed -- yielding a mask of 1.
    if (pageSize <= 0 || (pageSize & (pageSize - 1)) != 0) {
        ARCH_ERROR("ArchGetPageSize() is not a positive power of two");
    }
    return ~static_cast<uintptr_t>(pageSize - 1);
}

// Round \p addr down to the start of the page containing it.
static inline void *
RoundToPageAddr(void *addr)
{
    static const uintptr_t pageMask = GetPageMask();
    return reinterpret_cast<void *>(
        reinterpret_cast<uintptr_t>(addr) & pageMask);
}

#if defined (ARCH_OS_WINDOWS)

void *
ArchReserveVirtualMemory(size_t numBytes)
{
    return VirtualAlloc(NULL, numBytes, MEM_RESERVE, PAGE_NOACCESS);
}

bool
ArchCommitVirtualMemoryRange(void *start, size_t numBytes)
{
    return VirtualAlloc(start, numBytes, MEM_COMMIT, PAGE_READWRITE) != NULL;
}

bool
ArchFreeVirtualMemory(void *start, size_t /*numBytes*/)
{
    // MEM_RELEASE requires a size of 0 and releases the whole reservation.
    return VirtualFree(start, 0, MEM_RELEASE);
}

bool
ArchSetMemoryProtection(void const *start, size_t numBytes,
                        ArchMemoryProtection protection)
{
    void *pageStart = RoundToPageAddr(const_cast<void *>(start));
    SIZE_T len = numBytes + (reinterpret_cast<char const *>(start)-
                             reinterpret_cast<char const *>(pageStart));

    DWORD protXlat[] = {
        PAGE_NOACCESS,
        PAGE_READONLY,
        PAGE_READWRITE,
        PAGE_WRITECOPY  // Private file-backed mappings only; see the note on
                        // ArchMemoryProtection in virtualMemory.h.
    };

    DWORD oldProtect;
    return VirtualProtect(pageStart, len, protXlat[protection], &oldProtect);
}

#else // not ARCH_OS_WINDOWS, assume POSIX (mmap, mprotect)

void *
ArchReserveVirtualMemory(size_t numBytes)
{
    void *addr = mmap(NULL, numBytes, PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return addr == MAP_FAILED ? nullptr : addr;
}

bool
ArchCommitVirtualMemoryRange(void *start, size_t numBytes)
{
    void *pageStart = RoundToPageAddr(start);
    size_t len = numBytes + (reinterpret_cast<char *>(start)-
                             reinterpret_cast<char *>(pageStart));
    int result = mprotect(pageStart, len, PROT_READ | PROT_WRITE);
    return result == 0;
}

bool
ArchFreeVirtualMemory(void *start, size_t numBytes)
{
    return munmap(start, numBytes) == 0;
}

bool
ArchSetMemoryProtection(void const *start, size_t numBytes,
                        ArchMemoryProtection protection)
{
    void *pageStart = RoundToPageAddr(const_cast<void *>(start));
    size_t len = numBytes + (reinterpret_cast<char const *>(start)-
                             reinterpret_cast<char const *>(pageStart));

    int protXlat[] = {
        PROT_NONE,
        PROT_READ,
        PROT_READ | PROT_WRITE, // Yes these are the same on POSIX
        PROT_READ | PROT_WRITE  //
    };

    int result = mprotect(pageStart, len, protXlat[protection]);
    return result == 0;
}

#endif // POSIX

PXR_NAMESPACE_CLOSE_SCOPE
