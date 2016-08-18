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
#include "pxr/base/arch/testArchAbi.h"
#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/vsnprintf.h"
#include <cassert>
#include <iostream>
#include <typeinfo>
#include <ciso646>
#ifdef ARCH_OS_WINDOWS
#include <Windows.h>
#define GETSYM GetProcAddress
#else
#include <dlfcn.h>
#define WINAPI
#define GETSYM dlsym
#endif

typedef ArchAbiBase2* (WINAPI *NewDerived)();

int
main(int argc, char** argv)
{
    // Load the plugin and get the factory function.
    std::string error;
#ifdef ARCH_OS_WINDOWS
    HMODULE plugin = (HMODULE)ArchOpenLibrary(".\\libtestArchAbiPlugin.dll", ARCH_LIBRARY_LAZY);
    if (not plugin) {
        error = ArchStringPrintf("%ld", (long)GetLastError());
    }
#else
    std::string path = ArchGetExecutablePath();
    // Up two directories.
    path = path.substr(0, path.rfind('/', path.rfind('/') - 1));
    path += "/tests/lib/libtestArchAbiPlugin.so";
    void* plugin = ArchOpenLibrary(path.c_str(), ARCH_LIBRARY_LAZY);
    if (not plugin) {
        error += dlerror();
    }
#endif
    if (not plugin) {
        std::cerr << "Failed to load plugin: " << error << std::endl;
        assert(plugin);
    }

    NewDerived newPluginDerived = (NewDerived)GETSYM(plugin, "newDerived");
    if (not newPluginDerived) {
        std::cerr << "Failed to find factory symbol" << std::endl;
        assert(newPluginDerived);
    }

    // Create a derived object in this executable and in the plugin.
    ArchAbiBase2* mainDerived = new ArchAbiDerived<int>;
    ArchAbiBase2* pluginDerived = newPluginDerived();

    // Compare.  The types should be equal and the dynamic cast should not
    // change the pointer.
    std::cout
        << "Derived types are equal: "
        << ((typeid(*mainDerived) == typeid(*pluginDerived)) ? "yes" : "no")
        << ", cast: " << pluginDerived
        << "->" << dynamic_cast<ArchAbiDerived<int>*>(pluginDerived)
        << std::endl;
    assert(typeid(*mainDerived) == typeid(*pluginDerived));
    assert(pluginDerived == dynamic_cast<ArchAbiDerived<int>*>(pluginDerived));

    return 0;
}
