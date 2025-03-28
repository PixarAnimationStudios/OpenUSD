//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/arch/virtualMemory.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/systemInfo.h"

#include <cstdint>
#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <Memoryapi.h>
#else // Assume POSIX
#include <sys/mman.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

template <class T>
static inline T *RoundToPageAddr(T *addr) {
    static uint64_t PAGEMASK = ~(static_cast<uint64_t>(ArchGetPageSize())-1);
    return reinterpret_cast<T *>(reinterpret_cast<uintptr_t>(addr) & PAGEMASK);
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
        PAGE_READWRITE, // Unclear what the difference is btw
        PAGE_WRITECOPY  // READWRITE & WRITECOPY for private mappings...
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
    if (!addr || addr == MAP_FAILED)
        return nullptr;
    return addr;
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
