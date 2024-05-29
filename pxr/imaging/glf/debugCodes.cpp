//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file debugCodes.cpp

#include "pxr/imaging/glf/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(GLF_DEBUG_CONTEXT_CAPS,
        "Glf report when context caps are initialized and dump contents");
    TF_DEBUG_ENVIRONMENT_SYMBOL(GLF_DEBUG_ERROR_STACKTRACE,
        "Glf dump stack trace on GL error");
    TF_DEBUG_ENVIRONMENT_SYMBOL(GLF_DEBUG_SHADOW_TEXTURES,
        "Glf logging for shadow map management");
    TF_DEBUG_ENVIRONMENT_SYMBOL(GLF_DEBUG_DUMP_SHADOW_TEXTURES,
        "Glf outputs shadows textures to image files");
    TF_DEBUG_ENVIRONMENT_SYMBOL(GLF_DEBUG_POST_SURFACE_LIGHTING,
        "Glf post surface lighting setup");
}

PXR_NAMESPACE_CLOSE_SCOPE

