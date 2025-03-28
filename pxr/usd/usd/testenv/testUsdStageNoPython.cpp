//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//


// Test that a simple USD program can open a stage without initializing
// Python.

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include "pxr/base/tf/pySafePython.h"
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include "pxr/pxr.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/tf/diagnostic.h"

#include <cstdio>

PXR_NAMESPACE_USING_DIRECTIVE

static void
_OpenAndExport(const std::string &assetPath)
{
    printf("Opening stage: %s\n", assetPath.c_str());

    UsdStageRefPtr stage = UsdStage::Open(assetPath);
    TF_AXIOM(stage);

    printf("============= Begin Stage Dump =============\n");
    std::string flattened;
    bool exportSucceeded = stage->ExportToString(&flattened);
    TF_AXIOM(exportSucceeded);
    printf("%s", flattened.c_str());
    printf("============= End Stage Dump  =============\n");

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TF_AXIOM(!Py_IsInitialized());
#endif // PXR_PYTHON_SUPPORT_ENABLED
}

int
main(int argc, const char *argv[])
{
#ifdef PXR_PYTHON_SUPPORT_ENABLED
    // Sanity check that no libraries have a static initializer that is
    // initializing Python.
    TF_AXIOM(!Py_IsInitialized());
#endif // PXR_PYTHON_SUPPORT_ENABLED

    UsdStageRefPtr emptyStage = UsdStage::CreateInMemory();

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TF_AXIOM(!Py_IsInitialized());
#endif // PXR_PYTHON_SUPPORT_ENABLED

    std::string asciiFilePath = "ascii.usd";
    _OpenAndExport(asciiFilePath);

    std::string binaryFilePath = "binary.usd";
    _OpenAndExport(binaryFilePath);

    return 0;
}
