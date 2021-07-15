//
// Copyright 2021 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_OFFLINE_CONTEXT_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_OFFLINE_CONTEXT_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/context.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

// Context for offline rendering in HdPrman.
struct HdPrman_OfflineContext: public HdPrman_Context
{
    struct RenderOutput
    {
        RtUString name;
        riley::RenderOutputType type; 
        RtParamList params;
    };

    HDPRMAN_API
    HdPrman_OfflineContext();

    HDPRMAN_API
    ~HdPrman_OfflineContext();

    // Produces a render
    HDPRMAN_API
    void Render();

    // Checks whether context was successfully initialized.
    HDPRMAN_API
    bool IsValid() const;

    HDPRMAN_API
    void InitializeWithDefaults();
    HDPRMAN_API
    void Initialize(
        RtParamList rileyOptions, 
        riley::ShadingNode integratorNode,
        RtUString cameraName, 
        riley::ShadingNode cameraNode, 
        riley::Transform cameraXform, RtParamList cameraParams,
        riley::Extent outputFormat, TfToken outputFilename,
        std::vector<riley::ShadingNode> const & fallbackMaterialNodes,
        std::vector<riley::ShadingNode> const & fallbackVolumeNodes,
        std::vector<RenderOutput> const & renderOutputs);

    // Optional facility to quickly add a light to Riley
    HDPRMAN_API
    void SetFallbackLight(
        riley::ShadingNode node,
        riley::Transform xform,
        RtParamList params);

    riley::CameraId cameraId;

private:
    // Finishes the renderer
    void _End();
    void _SetRileyOptions(RtParamList options);
    void _SetRileyIntegrator(riley::ShadingNode node);
    void _SetCamera(RtUString name, 
        riley::ShadingNode node, 
        riley::Transform xform, 
        RtParamList params);
    void _AddRenderOutput(RtUString name, 
        riley::RenderOutputType type,
        RtParamList const& params);
    void _SetRenderTargetAndDisplay(
        riley::Extent format,TfToken outputFilename);
    void _SetFallbackMaterial(
        std::vector<riley::ShadingNode> const & materialNodes);
    void _SetFallbackVolumeMaterial(
        std::vector<riley::ShadingNode> const & materialNodes);

    riley::IntegratorId _integratorId;
    riley::RenderTargetId _rtid;
    std::vector<riley::RenderViewId> _renderViews;
    std::vector<riley::RenderOutputId> _renderOutputs;
    riley::LightInstanceId _fallbackLightId;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_OFFLINE_CONTEXT_H
