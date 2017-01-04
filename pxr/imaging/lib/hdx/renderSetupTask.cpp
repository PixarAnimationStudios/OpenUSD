//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/camera.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/debugCodes.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/surfaceShader.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

HdxRenderSetupTask::HdxRenderSetupTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _viewport()
    , _camera()
{
    _colorRenderPassShader.reset(
        new HdRenderPassShader(HdxPackageRenderPassShader()));
    _idRenderPassShader.reset(
        new HdRenderPassShader(HdxPackageRenderPassIdShader()));

    _renderPassState.reset(
        new HdRenderPassState(_colorRenderPassShader));
}

void
HdxRenderSetupTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // set raster state to TaskContext
    (*ctx)[HdxTokens->renderPassState] = VtValue(_renderPassState);
}

void
HdxRenderSetupTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    HdChangeTracker::DirtyBits bits = _GetTaskDirtyBits();

    // XXX: for compatibility.
    if (bits & HdChangeTracker::DirtyParams) {
        HdxRenderTaskParams params;

        // if HdxRenderTaskParams is set, it's using old API
        if (!_GetSceneDelegateValue(HdTokens->params, &params)) {
            return;
        }

        Sync(params);
    }

    SyncCamera();
}

void
HdxRenderSetupTask::Sync(HdxRenderTaskParams const &params)
{
    _renderPassState->SetOverrideColor(params.overrideColor);
    _renderPassState->SetWireframeColor(params.wireframeColor);
    _renderPassState->SetLightingEnabled(params.enableLighting);
    _renderPassState->SetAlphaThreshold(params.alphaThreshold);
    _renderPassState->SetTessLevel(params.tessLevel);
    _renderPassState->SetDrawingRange(params.drawingRange);
    _renderPassState->SetCullStyle(params.cullStyle);
    if (params.enableHardwareShading) {
        _renderPassState->SetOverrideShader(HdShaderSharedPtr());
    } else {
        _renderPassState->SetOverrideShader(
            GetDelegate()->GetRenderIndex().GetShaderFallback());
    }
    if (params.enableIdRender) {
        _renderPassState->SetRenderPassShader(_idRenderPassShader);
    } else {
        _renderPassState->SetRenderPassShader(_colorRenderPassShader);
    }

    // XXX TODO: Handle params.geomStyle
    // XXX TODO: Handle params.complexity
    // XXX TODO: Handle params visability (hullVisibility, surfaceVisibility)

    // depth bias
    _renderPassState->SetDepthBiasUseDefault(params.depthBiasUseDefault);
    _renderPassState->SetDepthBiasEnabled(params.depthBiasEnable);
    _renderPassState->SetDepthBias(params.depthBiasConstantFactor,
                               params.depthBiasSlopeFactor);
    _renderPassState->SetDepthFunc(params.depthFunc);

    // alpha to coverage
    // XXX:  Long-term Alpha to Coverage will be a render style on the
    // task.  However, as there isn't a fallback we current force it
    // enabled, unless a client chooses to manage the setting itself (aka usdImaging).
    _renderPassState->SetAlphaToCoverageUseDefault(
        GetDelegate()->IsEnabled(HdxOptionTokens->taskSetAlphaToCoverage));
    _renderPassState->SetAlphaToCoverageEnabled(
        !TfDebug::IsEnabled(HDX_DISABLE_ALPHA_TO_COVERAGE));

    _viewport = params.viewport;

    const HdRenderIndex &renderIndex = GetDelegate()->GetRenderIndex();
    _camera = static_cast<const HdxCamera *>(
                renderIndex.GetSprim(HdPrimTypeTokens->camera,
                                     params.camera));
}

void
HdxRenderSetupTask::SyncCamera()
{
    if (_camera && _renderPassState) {
        VtValue modelViewVt  = _camera->Get(HdxCameraTokens->worldToViewMatrix);
        VtValue projectionVt = _camera->Get(HdxCameraTokens->projectionMatrix);
        GfMatrix4d modelView = modelViewVt.Get<GfMatrix4d>();
        GfMatrix4d projection= projectionVt.Get<GfMatrix4d>();

        // If there is a window policy available in this camera
        // we will extract it and adjust the projection accordingly.
        VtValue windowPolicy = _camera->Get(HdxCameraTokens->windowPolicy);
        if (windowPolicy.IsHolding<CameraUtilConformWindowPolicy>()) {
            const CameraUtilConformWindowPolicy policy = 
                windowPolicy.Get<CameraUtilConformWindowPolicy>();

            projection = CameraUtilConformedWindow(projection, policy,
                _viewport[3] != 0.0 ? _viewport[2] / _viewport[3] : 1.0);
        }

        const VtValue &vClipPlanes = _camera->Get(HdxCameraTokens->clipPlanes);
        const HdRenderPassState::ClipPlanesVector &clipPlanes =
            vClipPlanes.Get<HdRenderPassState::ClipPlanesVector>();

        // sync render pass state
        _renderPassState->SetCamera(modelView, projection, _viewport);
        _renderPassState->SetClipPlanes(clipPlanes);
        _renderPassState->Sync();
    }
}


// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxRenderTaskParams& pv)
{
    out << "RenderTask Params: (...) " 
        << pv.overrideColor << " " 
        << pv.wireframeColor << " " 
        << pv.enableLighting << " "
        << pv.enableIdRender << " "
        << pv.alphaThreshold << " "
        << pv.tessLevel << " "
        << pv.drawingRange << " "
        << pv.enableHardwareShading << " "
        << pv.depthBiasEnable << " "
        << pv.depthBiasConstantFactor << " "
        << pv.depthBiasSlopeFactor << " "
        << pv.depthFunc << " "
        << pv.cullStyle << " "
        << pv.geomStyle << " "
        << pv.complexity << " "
        << pv.hullVisibility << " "
        << pv.surfaceVisibility << " "
        << pv.camera << " "
        << pv.viewport
        ;
    return out;
}

bool operator==(const HdxRenderTaskParams& lhs, const HdxRenderTaskParams& rhs) 
{
    return lhs.overrideColor           == rhs.overrideColor           && 
           lhs.wireframeColor          == rhs.wireframeColor          && 
           lhs.enableLighting          == rhs.enableLighting          && 
           lhs.enableIdRender          == rhs.enableIdRender          && 
           lhs.alphaThreshold          == rhs.alphaThreshold          && 
           lhs.tessLevel               == rhs.tessLevel               && 
           lhs.drawingRange            == rhs.drawingRange            && 
           lhs.enableHardwareShading   == rhs.enableHardwareShading   && 
           lhs.depthBiasEnable         == rhs.depthBiasEnable         && 
           lhs.depthBiasConstantFactor == rhs.depthBiasConstantFactor && 
           lhs.depthBiasSlopeFactor    == rhs.depthBiasSlopeFactor    && 
           lhs.depthFunc               == rhs.depthFunc               && 
           lhs.cullStyle               == rhs.cullStyle               && 
           lhs.geomStyle               == rhs.geomStyle               && 
           lhs.complexity              == rhs.complexity              && 
           lhs.hullVisibility          == rhs.hullVisibility          && 
           lhs.surfaceVisibility       == rhs.surfaceVisibility       && 
           lhs.camera                  == rhs.camera                  && 
           lhs.viewport                == rhs.viewport;
}

bool operator!=(const HdxRenderTaskParams& lhs, const HdxRenderTaskParams& rhs) 
{
    return !(lhs == rhs);
}
