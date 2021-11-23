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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_OFFLINE_RENDER_PARAM_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_OFFLINE_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/renderParam.h"

#include "Riley.h"

PXR_NAMESPACE_OPEN_SCOPE

// RenderParam for offline rendering in HdPrman.
class HdPrman_OfflineRenderParam: public HdPrman_RenderParam
{
public:

    HDPRMAN_API
    HdPrman_OfflineRenderParam();

    HDPRMAN_API
    ~HdPrman_OfflineRenderParam() override;

    // Start connection to Renderman.
    HDPRMAN_API
    void Begin(HdPrmanRenderDelegate *renderDelegate);

    // Produces a render
    HDPRMAN_API
    void Render();

    // Checks whether context was successfully initialized.
    HDPRMAN_API
    bool IsValid() const;

    // Returns Riley scene. Since this is the offline renderParam, it
    // currently does not stop the render.
    riley::Riley * AcquireRiley() override;

    riley::IntegratorId GetActiveIntegratorId() override;

private:
    // Finishes the renderer
    void _End();
    void _AddRenderOutput(RtUString name, 
        riley::RenderOutputType type,
        RtParamList const& params);
    void _SetRenderTargetAndDisplay(
        riley::Extent format,TfToken outputFilename);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_OFFLINE_RENDER_PARAM_H
