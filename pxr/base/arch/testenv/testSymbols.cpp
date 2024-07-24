//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/symbols.h"
#include "pxr/base/arch/error.h"

#include <cstdlib>

PXR_NAMESPACE_USING_DIRECTIVE

static void Code() { }
static int data = 1;
static int bss;

static bool _GetLibraryPath(void* address, std::string* result)
{
    return ArchGetAddressInfo(address, result, NULL, NULL, NULL);
}

static std::string GetBasename(const std::string& path)
{
#if defined(ARCH_OS_WINDOWS)
    std::string::size_type i = path.find_last_of("/\\");
    if (i != std::string::npos) {
        std::string::size_type j = path.find(".exe");
        if (j != std::string::npos) {
            return path.substr(i + 1, j - i - 1);
        }
        return path.substr(i + 1);
    }
#else
    std::string::size_type i = path.rfind('/');
    if (i != std::string::npos) {
        return path.substr(i + 1);
    }
#endif
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
