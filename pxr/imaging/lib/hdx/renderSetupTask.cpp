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

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/renderPassState.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/surfaceShader.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/base/gf/frustum.h"

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
        if (not _GetSceneDelegateValue(HdTokens->params, &params)) {
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
    // XXX:  Long-term ALpha to Coverage will be a render style on the
    // task.  However, as there isn't a fallback we current force it
    // enabled, unless a client chooses to manage the setting itself (aka usdImaging).
    _renderPassState->SetAlphaToCoverageUseDefault(
        GetDelegate()->IsEnabled(HdxOptionTokens->taskSetAlphaToCoverage));
    _renderPassState->SetAlphaToCoverageEnabled(
        not TfDebug::IsEnabled(HDX_DISABLE_ALPHA_TO_COVERAGE));

    _viewport = params.viewport;

    _camera = GetDelegate()->GetRenderIndex().GetCamera(params.camera);
}

void
HdxRenderSetupTask::SyncCamera()
{
    if (_camera and _renderPassState) {
        GfMatrix4d modelViewMatrix, projectionMatrix;

        // XXX This code will be removed when we drop support for
        // storing frustum in the render index
        VtValue frustumVt = _camera->Get(HdTokens->cameraFrustum);

        if(frustumVt.IsHolding<GfFrustum>()) {

            // Extract the window policy to adjust the frustum correctly
            VtValue windowPolicy = _camera->Get(HdTokens->windowPolicy);
            if (not TF_VERIFY(windowPolicy.IsHolding<CameraUtilConformWindowPolicy>())) {
                return;
            }

            const CameraUtilConformWindowPolicy policy = 
                windowPolicy.Get<CameraUtilConformWindowPolicy>();

            // Extract the frustum and calculate the correctly fitted/cropped
            // viewport
            GfFrustum frustum = frustumVt.Get<GfFrustum>();
            CameraUtilConformWindow(
                &frustum, policy,
                _viewport[3] != 0.0 ? _viewport[2] / _viewport[3] : 1.0);

            // Now that we have the actual frustum let's calculate the matrices
            // so we can upload them to the gpu via the raster state
            modelViewMatrix = frustum.ComputeViewMatrix();;
            projectionMatrix = frustum.ComputeProjectionMatrix();
        } else {
            VtValue modelViewMatrixVt = _camera->Get(HdShaderTokens->worldToViewMatrix);
            VtValue projectionMatrixVt = _camera->Get(HdShaderTokens->projectionMatrix);

            modelViewMatrix = modelViewMatrixVt.Get<GfMatrix4d>();
            projectionMatrix = projectionMatrixVt.Get<GfMatrix4d>();
        }

        const VtValue &vClipPlanes = _camera->Get(HdTokens->clipPlanes);
        const HdRenderPassState::ClipPlanesVector &clipPlanes =
            vClipPlanes.Get<HdRenderPassState::ClipPlanesVector>();

        // sync render pass state
        _renderPassState->SetCamera(modelViewMatrix, projectionMatrix, _viewport);
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
    return lhs.overrideColor           == rhs.overrideColor           and
           lhs.wireframeColor          == rhs.wireframeColor          and
           lhs.enableLighting          == rhs.enableLighting          and
           lhs.enableIdRender          == rhs.enableIdRender          and
           lhs.alphaThreshold          == rhs.alphaThreshold          and
           lhs.tessLevel               == rhs.tessLevel               and
           lhs.drawingRange            == rhs.drawingRange            and
           lhs.enableHardwareShading   == rhs.enableHardwareShading   and
           lhs.depthBiasEnable         == rhs.depthBiasEnable         and
           lhs.depthBiasConstantFactor == rhs.depthBiasConstantFactor and
           lhs.depthBiasSlopeFactor    == rhs.depthBiasSlopeFactor    and
           lhs.depthFunc               == rhs.depthFunc               and
           lhs.cullStyle               == rhs.cullStyle               and
           lhs.geomStyle               == rhs.geomStyle               and
           lhs.complexity              == rhs.complexity              and
           lhs.hullVisibility          == rhs.hullVisibility          and
           lhs.surfaceVisibility       == rhs.surfaceVisibility       and
           lhs.camera                  == rhs.camera                  and
           lhs.viewport                == rhs.viewport;
}

bool operator!=(const HdxRenderTaskParams& lhs, const HdxRenderTaskParams& rhs) 
{
    return not(lhs == rhs);
}
