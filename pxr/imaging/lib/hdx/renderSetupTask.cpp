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
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/imaging/hdx/debugCodes.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/renderBuffer.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hdSt/glslfxShader.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

HdStShaderCodeSharedPtr HdxRenderSetupTask::_overrideShader;

HdxRenderSetupTask::HdxRenderSetupTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdSceneTask(delegate, id)
    , _colorRenderPassShader()
    , _idRenderPassShader()
    , _viewport()
    , _cameraId()
    , _renderTags()    
{
    _colorRenderPassShader.reset(
        new HdStRenderPassShader(HdxPackageRenderPassShader()));
    _idRenderPassShader.reset(
        new HdStRenderPassShader(HdxPackageRenderPassIdShader()));

    HdRenderIndex &index = delegate->GetRenderIndex();
    _renderPassState = index.GetRenderDelegate()->CreateRenderPassState();
}

void
HdxRenderSetupTask::_Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // set raster state to TaskContext
    (*ctx)[HdxTokens->renderPassState] = VtValue(_renderPassState);
    (*ctx)[HdxTokens->renderTags] = VtValue(_renderTags);
}

void
HdxRenderSetupTask::_Sync(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdDirtyBits bits = _GetTaskDirtyBits();

    // XXX: for compatibility.
    if (bits & HdChangeTracker::DirtyParams) {
        HdxRenderTaskParams params;

        // if HdxRenderTaskParams is set, it's using old API
        if (!_GetSceneDelegateValue(HdTokens->params, &params)) {
            return;
        }

        SyncParams(params);
    }

    SyncAttachments();
    SyncCamera();
    SyncRenderPassState();
}

void
HdxRenderSetupTask::SyncRenderPassState()
{
    _renderPassState->Sync(
        GetDelegate()->GetRenderIndex().GetResourceRegistry());
}

void
HdxRenderSetupTask::_SetHdStRenderPassState(HdxRenderTaskParams const &params,
                                        HdStRenderPassState *renderPassState)
{
    if (params.enableHardwareShading) {
        renderPassState->SetOverrideShader(HdStShaderCodeSharedPtr());
    } else {
        if (!_overrideShader) {
            _CreateOverrideShader();
        }
        renderPassState->SetOverrideShader(_overrideShader);
    }
    if (params.enableIdRender) {
        renderPassState->SetRenderPassShader(_idRenderPassShader);
    } else {
        renderPassState->SetRenderPassShader(_colorRenderPassShader);
    }
}

void
HdxRenderSetupTask::SyncParams(HdxRenderTaskParams const &params)
{
    _renderPassState->SetOverrideColor(params.overrideColor);
    _renderPassState->SetWireframeColor(params.wireframeColor);
    _renderPassState->SetMaskColor(params.maskColor);
    _renderPassState->SetIndicatorColor(params.indicatorColor);
    _renderPassState->SetPointColor(params.pointColor);
    _renderPassState->SetPointSize(params.pointSize);
    _renderPassState->SetPointSelectedSize(params.pointSelectedSize);
    _renderPassState->SetLightingEnabled(params.enableLighting);
    _renderPassState->SetAlphaThreshold(params.alphaThreshold);
    _renderPassState->SetTessLevel(params.tessLevel);
    _renderPassState->SetDrawingRange(params.drawingRange);
    _renderPassState->SetCullStyle(params.cullStyle);

    // XXX TODO: Handle params.geomStyle
    // XXX TODO: Handle params.complexity
    // XXX TODO: Handle params visability (hullVisibility, surfaceVisibility)

    // depth bias
    _renderPassState->SetDepthBiasUseDefault(params.depthBiasUseDefault);
    _renderPassState->SetDepthBiasEnabled(params.depthBiasEnable);
    _renderPassState->SetDepthBias(params.depthBiasConstantFactor,
                               params.depthBiasSlopeFactor);
    _renderPassState->SetDepthFunc(params.depthFunc);

    // stencil
    _renderPassState->SetStencilEnabled(params.stencilEnable);
    _renderPassState->SetStencil(params.stencilFunc, params.stencilRef,
            params.stencilMask, params.stencilFailOp, params.stencilZFailOp,
            params.stencilZPassOp);
    
    // alpha to coverage
    // XXX:  Long-term Alpha to Coverage will be a render style on the
    // task.  However, as there isn't a fallback we current force it
    // enabled, unless a client chooses to manage the setting itself (aka usdImaging).
    _renderPassState->SetAlphaToCoverageUseDefault(
        GetDelegate()->IsEnabled(HdxOptionTokens->taskSetAlphaToCoverage));
    _renderPassState->SetAlphaToCoverageEnabled(
        !TfDebug::IsEnabled(HDX_DISABLE_ALPHA_TO_COVERAGE));

    _viewport = params.viewport;
    _renderTags = params.renderTags;
    _cameraId = params.camera;
    _attachments = params.attachments;

    if (HdStRenderPassState* extendedState =
            dynamic_cast<HdStRenderPassState*>(_renderPassState.get())) {
        _SetHdStRenderPassState(params, extendedState);
    }
}

void
HdxRenderSetupTask::SyncAttachments()
{
    // Walk the attachments, resolving the render index references as they're
    // encountered.
    const HdRenderIndex &renderIndex = GetDelegate()->GetRenderIndex();
    HdRenderPassAttachmentVector attachments = _attachments;
    for (size_t i = 0; i < attachments.size(); ++i)
    {
        if (attachments[i].renderBuffer == nullptr) {
            attachments[i].renderBuffer = static_cast<HdRenderBuffer*>(
                renderIndex.GetBprim(HdPrimTypeTokens->renderBuffer,
                attachments[i].renderBufferId));
        }
    }
    _renderPassState->SetAttachments(attachments);
}

void
HdxRenderSetupTask::SyncCamera()
{
    const HdRenderIndex &renderIndex = GetDelegate()->GetRenderIndex();
    const HdCamera *camera = static_cast<const HdCamera *>(
        renderIndex.GetSprim(HdPrimTypeTokens->camera, _cameraId));

    if (camera) {
        VtValue modelViewVt  = camera->Get(HdCameraTokens->worldToViewMatrix);
        VtValue projectionVt = camera->Get(HdCameraTokens->projectionMatrix);
        GfMatrix4d modelView = modelViewVt.Get<GfMatrix4d>();
        GfMatrix4d projection= projectionVt.Get<GfMatrix4d>();

        // If there is a window policy available in this camera
        // we will extract it and adjust the projection accordingly.
        VtValue windowPolicy = camera->Get(HdCameraTokens->windowPolicy);
        if (windowPolicy.IsHolding<CameraUtilConformWindowPolicy>()) {
            const CameraUtilConformWindowPolicy policy = 
                windowPolicy.Get<CameraUtilConformWindowPolicy>();

            projection = CameraUtilConformedWindow(projection, policy,
                _viewport[3] != 0.0 ? _viewport[2] / _viewport[3] : 1.0);
        }

        const VtValue &vClipPlanes = camera->Get(HdCameraTokens->clipPlanes);
        const HdRenderPassState::ClipPlanesVector &clipPlanes =
            vClipPlanes.Get<HdRenderPassState::ClipPlanesVector>();

        // sync render pass state
        _renderPassState->SetCamera(modelView, projection, _viewport);
        _renderPassState->SetClipPlanes(clipPlanes);
    }
}

void
HdxRenderSetupTask::_CreateOverrideShader()
{
    static std::mutex shaderCreateLock;

    if (!_overrideShader) {
        std::lock_guard<std::mutex> lock(shaderCreateLock);
        if (!_overrideShader) {
            _overrideShader = HdStShaderCodeSharedPtr(new HdStGLSLFXShader(
                GlfGLSLFXSharedPtr(new GlfGLSLFX(
                    HdStPackageFallbackSurfaceShader()))));
        }
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
        << pv.maskColor << " " 
        << pv.indicatorColor << " " 
        << pv.pointColor << " "
        << pv.pointSize << " "
        << pv.pointSelectedSize << " "
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
        << pv.viewport << " ";
        for (auto const& a : pv.attachments) {
            out << a << " ";
        }
        for (auto const& rt : pv.renderTags) {
            out << rt << " ";
        }
    return out;
}

bool operator==(const HdxRenderTaskParams& lhs, const HdxRenderTaskParams& rhs) 
{
    return lhs.overrideColor           == rhs.overrideColor           &&
           lhs.wireframeColor          == rhs.wireframeColor          &&
           lhs.maskColor               == rhs.maskColor               &&
           lhs.indicatorColor          == rhs.indicatorColor          &&
           lhs.pointColor              == rhs.pointColor              &&
           lhs.pointSize               == rhs.pointSize               &&
           lhs.pointSelectedSize       == rhs.pointSelectedSize       &&
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
           lhs.attachments             == rhs.attachments             &&
           lhs.camera                  == rhs.camera                  &&
           lhs.viewport                == rhs.viewport                &&
           lhs.renderTags              == rhs.renderTags;
}

bool operator!=(const HdxRenderTaskParams& lhs, const HdxRenderTaskParams& rhs) 
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
