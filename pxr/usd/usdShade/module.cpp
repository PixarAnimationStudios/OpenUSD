//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP(UsdShadeTypes);
    TF_WRAP(UsdShadeUtils);
    TF_WRAP(UsdShadeUdimUtils);
    TF_WRAP(UsdShadeConnectableAPI);
    TF_WRAP(UsdShadeCoordSysAPI);
    TF_WRAP(UsdShadeInput);
    TF_WRAP(UsdShadeOutput);
    TF_WRAP(UsdShadeShader);
    TF_WRAP(UsdShadeShaderDefParser);
    TF_WRAP(UsdShadeShaderDefUtils);
    TF_WRAP(UsdShadeNodeDefAPI);
    TF_WRAP(UsdShadeNodeGraph);
    TF_WRAP(UsdShadeMaterial); 
    TF_WRAP(UsdShadeMaterialBindingAPI);
    TF_WRAP(UsdShadeTokens);
}
