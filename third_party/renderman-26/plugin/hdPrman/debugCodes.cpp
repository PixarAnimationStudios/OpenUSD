//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/debugCodes.h"

#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_PRIMVARS, "Primvars");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_MATERIALS, "Materials");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_DUMP_MATERIALX_OSL_SHADER, 
        "Print MaterialX Generated Osl Shaders");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_LIGHT_LINKING, "Light linking");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_LIGHT_LIST, "Light list");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_VSTRUCTS, "Vstruct expansion");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_LIGHT_FILTER_LINKING,
        "Light filter linking");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_IMAGE_ASSET_RESOLVE,
        "Resolved image asset paths");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_INSTANCERS, "Instancers");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_MESHLIGHT, "Mesh lights");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_RENDER_SETTINGS,
        "Debug logging for all things render settings.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_RENDER_PASS,
        "Debug logging for HdPrman RenderPass dataflow and related Riley "
        "computations.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_TERMINAL_SCENE_INDEX_OBSERVER,
        "Debug logging for HdPrman terminal scene index observer.");
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDPRMAN_MOTION_BLUR, "Motion blur");
}

PXR_NAMESPACE_CLOSE_SCOPE

