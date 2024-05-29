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
    // Base classes must precede derived classes.  Indentation indicates class
    // hierarchy.
    TF_WRAP(UsdRiMaterialAPI);
    TF_WRAP(UsdRiSplineAPI);

    TF_WRAP(UsdRiStatementsAPI);

    TF_WRAP(UsdRiTokens);

    TF_WRAP(UsdRiRmanUtilities);

    TF_WRAP(UsdRiRenderPassAPI);
}
