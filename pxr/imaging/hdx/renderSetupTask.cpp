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
    : HdTask(id)
    , _colorRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            HdxPackageRenderPassColorShader()))
    , _idRenderPassShader(
        std::make_shared<HdStRenderPassShader>(
            HdxPackageRenderPassIdShader()))
    , _overrideWindowPolicy{false, CameraUtilFit}
    , _viewport(0)
{
}

HdxRenderSetupTask::~HdxRenderSetupTask() = default;

void
HdxRenderSetupTask::Sync(HdSceneDelegate* delegate,
                         HdTaskContext* ctx,
                         HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxRenderTaskParams params;

        if (!_GetTaskParams(delegate, &params)) {
            return;
        }

        SyncParams(delegate, params);
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxRenderSetupTask::Prepare(HdTaskContext* ctx,
                            HdRenderIndex* renderIndex)
{

    _PrepareAovBindings(ctx, renderIndex);
    PrepareCamera(renderIndex);

    HdRenderPassStateSharedPtr &renderPassState =
            _GetRenderPassState(renderIndex);

    renderPassState->Prepare(renderIndex->GetResourceRegistry());
    (*ctx)[HdxTokens->renderPassState] = VtValue(_renderPassState);
}

void
HdxRenderSetupTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // set raster state to TaskContext
    (*ctx)[HdxTokens->renderPassState] = VtValue(_renderPassState);
}

void
HdxRenderSetupTask::_SetRenderpassAndOverrideShadersForStorm(
    HdxRenderTaskParams const &params,
    HdStRenderPassState *renderPassState)
{
    if (params.enableSceneMaterials) {
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
HdxRenderSetupTask::SyncParams(HdSceneDelegate* delegate,
                               HdxRenderTaskParams const &params)
{
    HdRenderIndex &renderIndex = delegate->GetRenderIndex();
    HdRenderPassStateSharedPtr &renderPassState =
            _GetRenderPassState(&renderIndex);

    renderPassState->SetOverrideColor(params.overrideColor);
    renderPassState->SetWireframeColor(params.wireframeColor);
    renderPassState->SetPointColor(params.pointColor);
    renderPassState->SetPointSize(params.pointSize);
    renderPassState->SetLightingEnabled(params.enableLighting);
    renderPassState->SetAlphaThreshold(params.alphaThreshold);
    renderPassState->SetCullStyle(params.cullStyle);

    renderPassState->SetMaskColor(params.maskColor);
    renderPassState->SetIndicatorColor(params.indicatorColor);
    renderPassState->SetPointSelectedSize(params.pointSelectedSize);

    // Storm render pipeline state
    {
        renderPassState->SetDepthBiasUseDefault(params.depthBiasUseDefault);
        renderPassState->SetDepthBiasEnabled(params.depthBiasEnable);
        renderPassState->SetDepthBias(params.depthBiasConstantFactor,
                                    params.depthBiasSlopeFactor);
        renderPassState->SetDepthFunc(params.depthFunc);
        renderPassState->SetEnableDepthMask(params.depthMaskEnable);

        renderPassState->SetStencilEnabled(params.stencilEnable);
        renderPassState->SetStencil(params.stencilFunc, params.stencilRef,
                params.stencilMask, params.stencilFailOp, params.stencilZFailOp,
                params.stencilZPassOp);

        renderPassState->SetBlendEnabled(params.blendEnable);
        renderPassState->SetBlend(
                params.blendColorOp,
                params.blendColorSrcFactor, params.blendColorDstFactor,
                params.blendAlphaOp,
                params.blendAlphaSrcFactor, params.blendAlphaDstFactor);
        renderPassState->SetBlendConstantColor(params.blendConstantColor);
        
        renderPassState->SetAlphaToCoverageEnabled(
            params.enableAlphaToCoverage &&
            !TfDebug::IsEnabled(HDX_DISABLE_ALPHA_TO_COVERAGE));

        if (HdStRenderPassState * const hdStRenderPassState =
                    dynamic_cast<HdStRenderPassState*>(renderPassState.get())) {
            hdStRenderPassState->SetUseAovMultiSample(
                params.useAovMultiSample);
            hdStRenderPassState->SetResolveAovMultiSample(
                params.resolveAovMultiSample);
            
            _SetRenderpassAndOverrideShadersForStorm(
                params, hdStRenderPassState);
        }
    }

    _viewport = params.viewport;
    _framing = params.framing;
    _overrideWindowPolicy = params.overrideWindowPolicy;
    _cameraId = params.camera;
    _aovBindings = params.aovBindings;
}

void
HdxRenderSetupTask::_PrepareAovBindings(HdTaskContext* ctx,
                                        HdRenderIndex* renderIndex)
{
    // Walk the aov bindings, resolving the render index references as they're
    // encountered.
    for (size_t i = 0; i < _aovBindings.size(); ++i) {
        if (_aovBindings[i].renderBuffer == nullptr) {
            _aovBindings[i].renderBuffer = static_cast<HdRenderBuffer*>(
                renderIndex->GetBprim(HdPrimTypeTokens->renderBuffer,
                _aovBindings[i].renderBufferId));
        }
    }

    HdRenderPassStateSharedPtr &renderPassState =
            _GetRenderPassState(renderIndex);
    renderPassState->SetAovBindings(_aovBindings);

    if (!_aovBindings.empty()) {
        // XXX Tasks that are not RenderTasks (OIT, ColorCorrection etc) also
        // need access to AOVs, but cannot access SetupTask or RenderPassState.
        // One option is to let them know about the aovs directly (as task
        // parameters), but instead we do so via the task context.
        (*ctx)[HdxTokens->aovBindings] = VtValue(_aovBindings);
    }
}

void
HdxRenderSetupTask::PrepareCamera(HdRenderIndex* renderIndex)
{
    // If the render delegate does not support cameras, then
    // there is nothing to do here.
    if (!renderIndex->IsSprimTypeSupported(HdTokens->camera)) {
        return;
    } 

    const HdCamera *camera = static_cast<const HdCamera *>(
        renderIndex->GetSprim(HdPrimTypeTokens->camera, _cameraId));
    TF_VERIFY(camera);

    HdRenderPassStateSharedPtr const &renderPassState =
            _GetRenderPassState(renderIndex);

    if (_framing.IsValid()) {
        renderPassState->SetCameraAndFraming(
            camera, _framing, _overrideWindowPolicy);
    } else {
        renderPassState->SetCameraAndViewport(
            camera, _viewport);
    }
}

void
HdxRenderSetupTask::_CreateOverrideShader()
{
    static std::mutex shaderCreateLock;

    if (!_overrideShader) {
        std::lock_guard<std::mutex> lock(shaderCreateLock);
        if (!_overrideShader) {
            _overrideShader =
                std::make_shared<HdStGLSLFXShader>(
                    std::make_shared<HioGlslfx>(
                        HdStPackageFallbackSurfaceShader()));
        }
    }
}


HdRenderPassStateSharedPtr &
HdxRenderSetupTask::_GetRenderPassState(HdRenderIndex* renderIndex)
{
    if (!_renderPassState) {
        HdRenderDelegate *renderDelegate = renderIndex->GetRenderDelegate();
        _renderPassState = renderDelegate->CreateRenderPassState();
    }

    return _renderPassState;
}

// --------------------------------------------------------------------------- //
// VtValue Requirements
// --------------------------------------------------------------------------- //

std::ostream& operator<<(std::ostream& out, const HdxRenderTaskParams& pv)
{
    out << "RenderTask Params: (...) " 
        << pv.overrideColor << " " 
        << pv.wireframeColor << " " 
        << pv.pointColor << " "
        << pv.pointSize << " "
        << pv.enableLighting << " "
        << pv.enableIdRender << " "
        << pv.alphaThreshold << " "
        << pv.enableSceneMaterials << " "
        << pv.enableSceneLights << " "

        << pv.maskColor << " " 
        << pv.indicatorColor << " " 
        << pv.pointSelectedSize << " "

        << pv.depthBiasUseDefault << " "
        << pv.depthBiasEnable << " "
        << pv.depthBiasConstantFactor << " "
        << pv.depthBiasSlopeFactor << " "
        << pv.depthFunc << " "
        << pv.depthMaskEnable << " "
        << pv.stencilFunc << " "
        << pv.stencilRef << " "
        << pv.stencilMask << " "
        << pv.stencilFailOp << " "
        << pv.stencilZFailOp << " "
        << pv.stencilZPassOp << " "
        << pv.stencilEnable << " "
        << pv.blendColorOp << " "
        << pv.blendColorSrcFactor << " "
        << pv.blendColorDstFactor << " "
        << pv.blendAlphaOp << " "
        << pv.blendAlphaSrcFactor << " "
        << pv.blendAlphaDstFactor << " "
        << pv.blendConstantColor << " "
        << pv.blendEnable << " "
        << pv.enableAlphaToCoverage << ""
        << pv.useAovMultiSample << ""
        << pv.resolveAovMultiSample << ""

        << pv.camera << " "
        << pv.framing.displayWindow << " "
        << pv.framing.dataWindow << " "
        << pv.framing.pixelAspectRatio << " "
        << pv.viewport << " "
        << pv.cullStyle << " ";

    for (auto const& a : pv.aovBindings) {
        out << a << " ";
    }
    return out;
}

bool operator==(const HdxRenderTaskParams& lhs, const HdxRenderTaskParams& rhs) 
{
    return lhs.overrideColor            == rhs.overrideColor            &&
           lhs.wireframeColor           == rhs.wireframeColor           &&
           lhs.pointColor               == rhs.pointColor               &&
           lhs.pointSize                == rhs.pointSize                &&
           lhs.enableLighting           == rhs.enableLighting           &&
           lhs.enableIdRender           == rhs.enableIdRender           &&
           lhs.alphaThreshold           == rhs.alphaThreshold           &&
           lhs.enableSceneMaterials     == rhs.enableSceneMaterials     &&
           lhs.enableSceneLights        == rhs.enableSceneLights        &&
 
           lhs.maskColor                == rhs.maskColor                &&
           lhs.indicatorColor           == rhs.indicatorColor           &&
           lhs.pointSelectedSize        == rhs.pointSelectedSize        &&
 
           lhs.aovBindings              == rhs.aovBindings              &&
           
           lhs.depthBiasUseDefault      == rhs.depthBiasUseDefault      &&
           lhs.depthBiasEnable          == rhs.depthBiasEnable          &&
           lhs.depthBiasConstantFactor  == rhs.depthBiasConstantFactor  &&
           lhs.depthBiasSlopeFactor     == rhs.depthBiasSlopeFactor     &&
           lhs.depthFunc                == rhs.depthFunc                &&
           lhs.depthMaskEnable          == rhs.depthMaskEnable          &&
           lhs.stencilFunc              == rhs.stencilFunc              &&
           lhs.stencilRef               == rhs.stencilRef               &&
           lhs.stencilMask              == rhs.stencilMask              &&
           lhs.stencilFailOp            == rhs.stencilFailOp            &&
           lhs.stencilZFailOp           == rhs.stencilZFailOp           &&
           lhs.stencilZPassOp           == rhs.stencilZPassOp           &&
           lhs.stencilEnable            == rhs.stencilEnable            &&
           lhs.blendColorOp             == rhs.blendColorOp             &&
           lhs.blendColorSrcFactor      == rhs.blendColorSrcFactor      &&
           lhs.blendColorDstFactor      == rhs.blendColorDstFactor      &&
           lhs.blendAlphaOp             == rhs.blendAlphaOp             &&
           lhs.blendAlphaSrcFactor      == rhs.blendAlphaSrcFactor      &&
           lhs.blendAlphaDstFactor      == rhs.blendAlphaDstFactor      &&
           lhs.blendConstantColor       == rhs.blendConstantColor       &&
           lhs.blendEnable              == rhs.blendEnable              &&
           lhs.enableAlphaToCoverage    == rhs.enableAlphaToCoverage    &&
           lhs.useAovMultiSample        == rhs.useAovMultiSample        &&
           lhs.resolveAovMultiSample    == rhs.resolveAovMultiSample    &&
           
           lhs.camera                   == rhs.camera                   &&
           lhs.framing                  == rhs.framing                  &&
           lhs.viewport                 == rhs.viewport                 &&
           lhs.cullStyle                == rhs.cullStyle                &&
           lhs.overrideWindowPolicy     == rhs.overrideWindowPolicy;
}

bool operator!=(const HdxRenderTaskParams& lhs, const HdxRenderTaskParams& rhs) 
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
