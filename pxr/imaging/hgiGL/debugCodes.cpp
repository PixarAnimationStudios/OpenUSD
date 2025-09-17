//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file debugCodes.cpp

#include "pxr/imaging/hgiGL/debugCodes.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HGIGL_DEBUG_FRAMEBUFFER_CACHE,
        "Debug framebuffer cache management per context arena.");
}


PXR_NAMESPACE_CLOSE_SCOPE

