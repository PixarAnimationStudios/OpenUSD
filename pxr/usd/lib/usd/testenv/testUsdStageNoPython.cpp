//
// Copyright 2017 Pixar
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


// Test that a simple USD program can open a stage without initializing
// Python.

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#ifdef _DEBUG
  #undef _DEBUG
  #include <Python.h>
  #define _DEBUG
#else
  #include <Python.h>
#endif
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
