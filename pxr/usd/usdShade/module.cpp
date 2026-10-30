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
    TF_WRAP_NB(UsdShadeTypes);
    TF_WRAP_NB(UsdShadeUtils);
    TF_WRAP_NB(UsdShadeUdimUtils);
    TF_WRAP_NB(UsdShadeInput);
    TF_WRAP_NB(UsdShadeOutput);
    TF_WRAP_NB(UsdShadeShaderDefParser);
    TF_WRAP_NB(UsdShadeShaderDefUtils);

    // Generated Schema classes.  Do not remove or edit the following line.
    #include "generatedSchema.module.h"
}
