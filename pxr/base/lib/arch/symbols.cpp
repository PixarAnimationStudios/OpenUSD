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
#include "pxr/base/arch/symbols.h"
#include "pxr/base/arch/defines.h"
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
#include <dlfcn.h>
#elif defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#include <DbgHelp.h>
#include <Psapi.h>
#endif

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
            *objectPath = info.dli_fname;
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
        char modName[MAX_PATH] = {0};
        if (GetModuleFileName(module, modName, MAX_PATH)) {
            objectPath->assign(modName);
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
