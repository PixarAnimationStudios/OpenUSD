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
#include "hdPrman/offlineRenderPass.h"
#include "hdPrman/offlineRenderParam.h"
#include "hdPrman/camera.h"
#include "hdPrman/renderDelegate.h"
#include "pxr/imaging/hd/renderPassState.h"

#include "hdPrman/rixStrings.h"     // Strings

#include "RixShadingUtils.h"        // RixConstants

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_OfflineRenderPass::HdPrman_OfflineRenderPass(
    HdRenderIndex *index,
    HdRprimCollection const &collection,
    std::shared_ptr<HdPrman_RenderParam> renderParam)
: HdRenderPass(index, collection)
, _converged(false)
{
    _offlineRenderParam =
        std::dynamic_pointer_cast<HdPrman_OfflineRenderParam>(renderParam);
}

HdPrman_OfflineRenderPass::~HdPrman_OfflineRenderPass() = default;

bool
HdPrman_OfflineRenderPass::IsConverged() const
{
    return _converged;
}

void
HdPrman_OfflineRenderPass::_Execute(
    HdRenderPassStateSharedPtr const& renderPassState,
    TfTokenVector const &renderTags)
{
    // Enable/disable the fallback light when the scene provides no lights.
    _offlineRenderParam->SetFallbackLightsEnabled(
        !_offlineRenderParam->HasSceneLights());

    const HdPrmanCamera * const hdCam =
        dynamic_cast<HdPrmanCamera const *>(renderPassState->GetCamera());

    HdPrmanCameraContext &cameraContext =
        _offlineRenderParam->GetCameraContext();
    cameraContext.SetCamera(hdCam);

    if (renderPassState->GetFraming().IsValid()) {
        // For new clients setting the camera framing.
        cameraContext.SetFraming(renderPassState->GetFraming());
    } else {
        // For old clients using the viewport.
        //
        // Note that we ignore the viewport's offset. But that has no
        // effect because the resulting output image is the same (at
        // least up to the display/data window metadata in OpenEXR).
        const GfVec4f vp = renderPassState->GetViewport();
        cameraContext.SetFraming(
            CameraUtilFraming(
                GfRect2i(GfVec2i(0), vp[2], vp[3])));
    }

    cameraContext.SetWindowPolicy(renderPassState->GetWindowPolicy());

    const bool camChanged = cameraContext.IsInvalid();

    HdPrmanRenderDelegate * const renderDelegate =
        static_cast<HdPrmanRenderDelegate*>(
            GetRenderIndex()->GetRenderDelegate());
    const int currentSettingsVersion =
        renderDelegate->GetRenderSettingsVersion();

    const int lastVersion = _offlineRenderParam->GetLastSettingsVersion();

    if (lastVersion != currentSettingsVersion || camChanged) {
        
        cameraContext.MarkValid();
        _offlineRenderParam->SetLastSettingsVersion(currentSettingsVersion);

        cameraContext.UpdateRileyCameraAndClipPlanes(
            _offlineRenderParam->AcquireRiley());

        RtParamList &options = _offlineRenderParam->GetOptions();
        cameraContext.SetRileyOptions(
            &options);
        _offlineRenderParam->SetOptionsFromRenderSettings(
            renderDelegate,
            options);

        _offlineRenderParam->AcquireRiley()->SetOptions(options);

        const GfVec2i resolution = 
            cameraContext.GetResolutionFromDisplayWindow();

        _offlineRenderParam->SetResolutionOfRenderTarget(resolution.data());
    }

    _offlineRenderParam->Render();
    _converged = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
