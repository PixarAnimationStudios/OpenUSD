//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/skinningSettings.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HD_ENABLE_DEFERRED_SKINNING, false,
    "Instead of creating skinning extComputations, emit all the skinning "
    "inputs as primvars and defer skinning to the renderer because renderers "
    "like storm might have special optimization like vertex shaders "
    "implementation. Caveat is current implementation requires modifying "
    "sceneIndices in usdImaging and usdSkelImaging and the graph doesn't have "
    "any renderer knowledge so we can not dynamically change the behavior. "
    "When this is true, renderers that don't support this feature will lose "
    "skinning. It would be better to make this renderer configurable in the "
    "future.");

namespace HdSkinningSettings {

bool
IsSkinningDeferred()
{
    // HYD-3475
    // Currently deferred skinning doesn't work in GL so need to 
    // setenv HGI_ENABLE_VULKAN=1 as well.
    return TfGetEnvSetting(HD_ENABLE_DEFERRED_SKINNING);
}

TfTokenVector 
GetSkinningInputNames(const TfTokenVector& extraInputNames)
{
    TfTokenVector inputNames;
    for (const TfToken& input: HdSkinningInputTokens->allTokens) {
        // HYD-3533
        // need to investigate why skinningScaleXforms is corrupting primvar 
        // inputs in the shader, this is working in compute shader code path.
        if (input != HdSkinningInputTokens->skinningScaleXforms) {
            inputNames.push_back(input);
        }
    }
    for (const TfToken& skelInput: HdSkinningSkelInputTokens->allTokens) {
        inputNames.push_back(skelInput);
    }
    for (const TfToken& extraInput: extraInputNames) {
        inputNames.push_back(extraInput);
    }
    return inputNames;
}

} // namespace HdSkinningSettings

PXR_NAMESPACE_CLOSE_SCOPE
