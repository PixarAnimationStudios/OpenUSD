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
#include "pxr/base/arch/library.h"
#include "pxr/base/arch/errno.h"

#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

void* ArchLibraryOpen(const std::string &filename, int flag)
{
#if defined(ARCH_OS_WINDOWS)
    return LoadLibrary(filename.c_str());
#else
    return dlopen(filename.c_str(), flag);
#endif
}

const char* ArchLibraryError()
{
#if defined(ARCH_OS_WINDOWS)
    DWORD error = ::GetLastError();
    return error ?  ArchStrSysError(error).c_str() : nullptr;
#else
    return dlerror();
#endif
}

int ArchLibraryClose(void* handle)
{
#if defined(ARCH_OS_WINDOWS)
    int status = ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
    int status = dlclose(handle);
#endif
    return status;
}