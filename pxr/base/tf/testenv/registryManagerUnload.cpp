//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/debugCodes.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/diagnosticLite.h"
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
