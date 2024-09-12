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
    TF_WRAP(UsdShadeInput);
    TF_WRAP(UsdShadeOutput);
    TF_WRAP(UsdShadeShaderDefParser);
    TF_WRAP(UsdShadeShaderDefUtils);

    // Generated Schema classes.  Do not remove or edit the following line.
    #include "generatedSchema.module.h"
}
