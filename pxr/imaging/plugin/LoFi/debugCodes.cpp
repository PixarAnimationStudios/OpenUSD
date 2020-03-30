//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(LOFI_RENDERER, "LoFi Renderer infos");
    TF_DEBUG_ENVIRONMENT_SYMBOL(LOFI_PERF, "LoFi Registry PerfLog infos");
    TF_DEBUG_ENVIRONMENT_SYMBOL(LOFI_REGISTRY, "LoFi Registry infos");
    TF_DEBUG_ENVIRONMENT_SYMBOL(LOFI_SHADER, "LoFi GLSL Shader infos");
    TF_DEBUG_ENVIRONMENT_SYMBOL(LOFI_ERROR, "LoFi OpenGL Errors");
}

PXR_NAMESPACE_CLOSE_SCOPE

