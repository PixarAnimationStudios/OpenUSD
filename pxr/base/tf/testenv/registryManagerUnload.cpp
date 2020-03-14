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

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/dl.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/arch/symbols.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/library.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Registry function tag type
class Tf_TestRegistryFunctionPlugin {};

static void
_LoadAndUnloadSharedLibrary(const std::string & libraryPath)
{
    std::string dlErrorMsg;
    void * handle = TfDlopen(libraryPath.c_str(), ARCH_LIBRARY_NOW, &dlErrorMsg);
    TF_AXIOM(handle);
    TF_AXIOM(dlErrorMsg.empty());
    TF_AXIOM(!TfDlclose(handle));
}

static bool
Test_TfRegistryManagerUnload()
{
    TfDebug::Enable(TF_DLOPEN);
    TfDebug::Enable(TF_DLCLOSE);

    // Compute path to test library.
    std::string libraryPath;
    TF_AXIOM(ArchGetAddressInfo((void*)Test_TfRegistryManagerUnload, &libraryPath, NULL, NULL, NULL));
    libraryPath = TfGetPathName(libraryPath) +
                  "lib" ARCH_PATH_SEP
#if !defined(ARCH_OS_WINDOWS)
                  "lib"
#endif
                  "TestTfRegistryFunctionPlugin" ARCH_LIBRARY_SUFFIX;

    // Make sure that this .so exists
    printf("Checking test shared lib: %s\n", libraryPath.c_str());
    TF_AXIOM(!ArchFileAccess(libraryPath.c_str(), R_OK));

    // Load and unload a shared library that has a registration function
    // before anyone subscribes to that type.
    _LoadAndUnloadSharedLibrary(libraryPath);

    // Subscribe to the registry function from our unloaded shared library.
    // This will crash as in bug 99729 if the registry manager fails to remove
    // functions from the unloaded library.
    TfRegistryManager::GetInstance()
        .SubscribeTo<Tf_TestRegistryFunctionPlugin>();

    // Load and unload again just to make sure that we still don't crash.
    _LoadAndUnloadSharedLibrary(libraryPath);

    return true;
}

TF_ADD_REGTEST(TfRegistryManagerUnload);
