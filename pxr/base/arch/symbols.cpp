//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/symbols.h"
#include "pxr/base/arch/defines.h"

#if defined(ARCH_OS_LINUX)
#include <dlfcn.h>
#elif defined(ARCH_OS_DARWIN)
#include <dlfcn.h>
#elif defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

bool
ArchGetAddressInfo(
    void* address,
    std::string* objectPath, void** baseAddress,
    std::string* symbolName, void** symbolAddress)
{
#if defined(_GNU_SOURCE) || defined(ARCH_OS_DARWIN)

    Dl_info info;
    if (dladdr(address, &info)) {
        if (objectPath) {
            // The object filename may be a relative path if, for instance,
            // the given address comes from an executable that was invoked
            // with a relative path, or from a shared library that was 
            // dlopen'd with a relative path. We want to always return 
            // absolute paths, so do the resolution here.
            //
            // This may be incorrect if the current working directory was 
            // changed after the source object was loaded.
            *objectPath = ArchAbsPath(info.dli_fname);
        }
        if (baseAddress) {
            *baseAddress = info.dli_fbase;
        }
        if (symbolName) {
            *symbolName = info.dli_sname ? info.dli_sname : "";
        }
        if (symbolAddress) {
            *symbolAddress = info.dli_saddr;
        }
        return true;
    }
    return false;

#elif defined(ARCH_OS_WINDOWS)

    if (!address) {
        return false;
    }

    HMODULE module = nullptr;
    if (!::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             reinterpret_cast<LPCSTR>(address),
                             &module)) {
        return false;
    }

    if (objectPath) {
        wchar_t modName[ARCH_PATH_MAX] = {0};
        if (GetModuleFileNameW(module, modName, ARCH_PATH_MAX)) {
            objectPath->assign(ArchWindowsUtf16ToUtf8(modName));
        }
    }

    if (baseAddress || symbolName || symbolAddress) {
        DWORD displacement;
        HANDLE process = GetCurrentProcess();
        SymInitialize(process, NULL, TRUE);

        // Symbol
        ULONG64 symBuffer[(sizeof(SYMBOL_INFO) +
                          MAX_SYM_NAME * sizeof(TCHAR) +
                          sizeof(ULONG64) - 1) / sizeof(ULONG64)];
        SYMBOL_INFO *symbol = (SYMBOL_INFO*)symBuffer;
        symbol->MaxNameLen = MAX_SYM_NAME;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

        // Line
        IMAGEHLP_LINE64 line = {0};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        DWORD64 dwAddress = (DWORD64)address;
        SymFromAddr(process, dwAddress, NULL, symbol);
        if (!SymGetLineFromAddr64(process, dwAddress,
                                  &displacement, &line)) {
            return false;
        }

        if (baseAddress) {
            MODULEINFO moduleInfo = {0};
            if (!GetModuleInformation(process, module,
                                      &moduleInfo, sizeof(moduleInfo))) {
                return false;
            }
            *baseAddress = moduleInfo.lpBaseOfDll;
        }

        if (symbolName) {
            *symbolName = symbol->Name ? symbol->Name : "";
        }

        if (symbolAddress) {
            *symbolAddress = (void*)symbol->Address;
        }
    }
    return true;

#else

    return false;

#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
