//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDX_DEBUG_DUMP_SHADOW_TEXTURES,
        "Output shadow textures to image files");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDX_DISABLE_ALPHA_TO_COVERAGE,
        "Disable alpha to coverage transpancy");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDX_INTERSECT,
        "Output debug info of intersector");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDX_SELECTION_SETUP,
        "Output debug info during creation of selection buffer");
}

PXR_NAMESPACE_CLOSE_SCOPE

