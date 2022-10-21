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
#include "pxr/pxr.h"
#include "pxr/base/arch/virtualMemory.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/systemInfo.h"

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
