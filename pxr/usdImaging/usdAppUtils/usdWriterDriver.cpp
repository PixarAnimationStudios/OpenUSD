//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "pxr/usdImaging/usdAppUtils/usdWriterDriver.h"

#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdAppUtilsUsdWriterDriver::UsdAppUtilsUsdWriterDriver(
    const TfToken& rendererPluginId) :
    _imagingEngine(HdDriver(), TfToken("HdUsdWriterRendererPlugin"), false)
{
    // Disable presentation to avoid the need to create an OpenGL context when
    // using other graphics APIs such as Metal and Vulkan.
    _imagingEngine.SetEnablePresentation(false);
}

bool
UsdAppUtilsUsdWriterDriver::Render(
    const UsdStagePtr& stage,
    const UsdTimeCode timeCode,
    const std::string& outputPath)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return false;
    }

    if (outputPath.empty()) {
        TF_CODING_ERROR("Invalid empty output path");
        return false;
    }

    UsdImagingGLRenderParams renderParams;
    renderParams.frame = timeCode;

    const UsdPrim& pseudoRoot = stage->GetPseudoRoot();
    _imagingEngine.Render(pseudoRoot, renderParams);

    HdCommandArgs args;
    args[TfToken("outputPath")] = outputPath;
    const auto& serializationResult = _imagingEngine.InvokeRendererCommand(TfToken("SerializeToUsd"), args);
    if (!serializationResult)
    {
        TF_WARN("Failed to serialize stage to USD.");
    }
}

PXR_NAMESPACE_CLOSE_SCOPE