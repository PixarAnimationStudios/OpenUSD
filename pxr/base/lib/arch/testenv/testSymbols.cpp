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
#include "pxr/base/arch/error.h"
#include <cstdlib>

static void Code() { }
static int data = 1;
static int bss;

static bool _GetLibraryPath(void* address, std::string* result)
{
    return ArchGetAddressInfo(address, result, NULL, NULL, NULL);
}

static std::string GetBasename(const std::string& path)
{
    std::string::size_type i = path.rfind('/');
    if (i != std::string::npos) {
        return path.substr(i + 1);
    }
    return path;
}

int main()
{
    std::string path;

    // Invalid pointer.
    ARCH_AXIOM(!_GetLibraryPath(0, &path));

    // Pointer to a local non-function.
    ARCH_AXIOM(!_GetLibraryPath(&path, &path));

    // Pointer into the DATA section.
    ARCH_AXIOM(_GetLibraryPath(&data, &path));
    ARCH_AXIOM(GetBasename(path) == "testArchSymbols");

    // Pointer into the BSS section.
    ARCH_AXIOM(_GetLibraryPath(&bss, &path));
    ARCH_AXIOM(GetBasename(path) == "testArchSymbols");

    // Find this library.
    ARCH_AXIOM(_GetLibraryPath((void*)&Code, &path));
    ARCH_AXIOM(GetBasename(path) == "testArchSymbols");

    // Find another library.
    ARCH_AXIOM(_GetLibraryPath((void*)&exit, &path));
    ARCH_AXIOM(GetBasename(path) != "testArchSymbols");

    return 0;
}
