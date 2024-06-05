//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/skydomeTask.h"

#include "pxr/imaging/hdx/fullscreenShader.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/simpleLightingShader.h"
#include "pxr/imaging/hdSt/textureHandle.h"
#include "pxr/imaging/hdSt/textureObject.h"

#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((skydomeFrag, "SkydomeFragment"))
    (skydomeTexture)
);

HdxSkydomeTask::HdxSkydomeTask(HdSceneDelegate* delegate, SdfPath const& id)
    : HdxTask(id)
    , _setupTask()
    , _settingsVersion(0)
    , _skydomeVisibility(true)
{
}

HdxSkydomeTask::~HdxSkydomeTask() = default;

void
HdxSkydomeTask::_Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();

    if (!_compositor) {
        _compositor = std::make_unique<HdxFullscreenShader>(
            _GetHgi(), "Skydome");
    }

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxRenderTaskParams params;

        // Following the pattern used in HdxRenderTask, where the params
        // (HdxRenderTaskParams) is optional. If present, use an internal setup 
        // task to unpack it. Otherwise rely on the renderPassState from the
        // task context during Execute.
        VtValue valueVt = delegate->Get(GetId(), HdTokens->params);
        if (valueVt.IsHolding<HdxRenderTaskParams>()) {

            params = valueVt.UncheckedGet<HdxRenderTaskParams>();
            if (!_setupTask) {
                _setupTask = std::make_shared<HdxRenderSetupTask>(
                    delegate, GetId());
            }
            _setupTask->SyncParams(delegate, params);
        }
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxSkydomeTask::Prepare(HdTaskContext* ctx, HdRenderIndex* renderIndex)
{
    if (_setupTask) {
        _setupTask->Prepare(ctx, renderIndex);
    }

    const HdRenderDelegate* renderDelegate = renderIndex->GetRenderDelegate();
    const unsigned int currentSettingsVersion =
        renderDelegate->GetRenderSettingsVersion();
    if (_settingsVersion != currentSettingsVersion) {
        _settingsVersion = currentSettingsVersion;
        _skydomeVisibility = renderDelegate->
            GetRenderSetting<bool>(
                HdRenderSettingsTokens->domeLightCameraVisibility,
                true);
    }

    _renderIndex = renderIndex;
}

void
HdxSkydomeTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Get the gfxCmdsDesc from the RenderPassState
    HdRenderPassStateSharedPtr renderPassState = _GetRenderPassState(ctx);
    HdStRenderPassState * const hdStRenderPassState =
        dynamic_cast<HdStRenderPassState*>(renderPassState.get());
    if (!hdStRenderPassState) {
        return;
    }
    HgiGraphicsCmdsDesc gfxCmdsDesc = 
        hdStRenderPassState->MakeGraphicsCmdsDesc(_renderIndex);

    // If the skydome is visible by the camera, get the Domelight's
    // transformation matrix from the lighting Context
    bool haveDomeLight = false;
    GfMatrix4f lightTransform(1);
    if (_skydomeVisibility) {
        GlfSimpleLightingContextRefPtr lightingContext;
        if (_GetTaskContextData(ctx, HdxTokens->lightingContext,
                                &lightingContext)) {

            GlfSimpleLightVector const& lights = lightingContext->GetLights();
            for (int i = 0; i < lightingContext->GetNumLightsUsed(); ++i) {

                GlfSimpleLight const &light = lights[i]; 
                if (light.IsDomeLight()) {
                    lightTransform = GfMatrix4f(
                        light.GetTransform().GetInverse());
                    haveDomeLight = true;
                    break;
                }
            }
        }
    }

    const bool haveColorAOV = !gfxCmdsDesc.colorTextures.empty();

    // If the skydome is not camera visible in a colorAOV or there is no
    // domelight/skydomeTexture, clear the AOVs
    if (!_skydomeVisibility || !haveColorAOV ||
        !haveDomeLight || !_GetSkydomeTexture(ctx)) {
        _GetHgi()->SubmitCmds(_GetHgi()->CreateGraphicsCmds(gfxCmdsDesc).get());
        return;
    }

    // Otherwise, set the fragment shader for the fullscreenShader
    _SetFragmentShader();

    // Get the Inverse Projection and View To World Matrices
    const GfMatrix4f invProjMatrix(
        hdStRenderPassState->GetProjectionMatrix().GetInverse());
    const GfMatrix4f viewToWorldMatrix(
        hdStRenderPassState->GetWorldToViewMatrix().GetInverse());

    // Update the Parameter Buffer if needed
    if (_UpdateParameterBuffer(invProjMatrix, viewToWorldMatrix, lightTransform)){
        constexpr size_t byteSize = sizeof(_ParameterBuffer);
        _compositor->SetShaderConstants(byteSize, &_parameterData);
    }
    
    // Bind the skydome texture 
    _compositor->BindTextures({_skydomeTexture});

    // Get the viewport size
    GfVec4i viewport = hdStRenderPassState->ComputeViewport();

    // Get the Color/Depth and Color/Depth Resolve Textures from the gfxCmdsDesc
    // so that the fullscreenShader can use them to create the appropriate
    // HgiGraphicsPipeline, HgiGraphicsCmdsDesc, and HgiGraphicsCmds 
    HgiTextureHandle colorDst = gfxCmdsDesc.colorTextures.empty() 
        ? HgiTextureHandle() 
        : gfxCmdsDesc.colorTextures[0];
    HgiTextureHandle colorResolveDst = gfxCmdsDesc.colorResolveTextures.empty()
        ? HgiTextureHandle()
        : gfxCmdsDesc.colorResolveTextures[0];
    HgiTextureHandle depthDst = gfxCmdsDesc.depthTexture;
    HgiTextureHandle depthResolveDst = gfxCmdsDesc.depthResolveTexture;

    // Draw the Skydome 
    _compositor->Draw(colorDst, colorResolveDst,
                      depthDst, depthResolveDst, viewport);
}

HdRenderPassStateSharedPtr 
HdxSkydomeTask::_GetRenderPassState(HdTaskContext *ctx) const
{
    if (_setupTask) {
        // If HdxRenderTaskParams is set on this task, we will have created an
        // internal HdxRenderSetupTask in _Sync, to sync and unpack the params,
        // and we should use the resulting resources.
        return _setupTask->GetRenderPassState();
    } else {
        // Otherwise, we expect an application-created HdxRenderSetupTask to
        // have run and put the renderpass resources in the task context.
        // See HdxRenderSetupTask::_Execute.
        HdRenderPassStateSharedPtr renderPassState;
        _GetTaskContextData(ctx, HdxTokens->renderPassState, &renderPassState);
        return renderPassState;
    }
}

bool
HdxSkydomeTask::_GetSkydomeTexture(HdTaskContext* ctx)
{
    TRACE_FUNCTION();

    // Get the texture from the Lighting Shader
    HdStLightingShaderSharedPtr lightingShader;
    bool haveLightingShader = 
        _GetTaskContextData(ctx, HdxTokens->lightingShader, &lightingShader);
    if (!haveLightingShader) {
        return false;
    }
    HdStSimpleLightingShader *simpleLightingShader = 
        dynamic_cast<HdStSimpleLightingShader*>(lightingShader.get());
    if (!simpleLightingShader) {
        return false;
    }
    HdStTextureHandleSharedPtr domeLightTextureHandle = 
        simpleLightingShader->GetDomeLightEnvironmentTextureHandle();
    if (!domeLightTextureHandle) {
        return false;
    }
    const HdStUvTextureObject *const domeLightTextureObject =
        dynamic_cast<HdStUvTextureObject*>(
            domeLightTextureHandle->GetTextureObject().get());
    if (!domeLightTextureObject->IsValid()) {
        return false;
    }
    _skydomeTexture = domeLightTextureObject->GetTexture();

    return true;
}

void 
HdxSkydomeTask::_SetFragmentShader()
{
    HgiShaderFunctionDesc fragDesc;
    fragDesc.debugName = _tokens->skydomeFrag.GetString();
    fragDesc.shaderStage = HgiShaderStageFragment;

    HgiShaderFunctionAddStageInput(&fragDesc, "uvOut", "vec2");
    HgiShaderFunctionAddTexture(&fragDesc, "skydomeTexture");
    HgiShaderFunctionAddStageOutput(&fragDesc, "hd_FragColor", "vec4", "color");
    HgiShaderFunctionAddStageOutput(
        &fragDesc, "gl_FragDepth", "float", "depth(any)");

    // The order of the constant parameters has to match the order in the 
    // _ParameterBuffer struct
    HgiShaderFunctionAddConstantParam(&fragDesc, "invProjMatrix", "mat4");
    HgiShaderFunctionAddConstantParam(&fragDesc, "viewToWorld", "mat4");
    HgiShaderFunctionAddConstantParam(&fragDesc, "lightTransform", "mat4");

    _compositor->SetProgram(
        HdxPackageSkydomeShader(), _tokens->skydomeFrag, fragDesc);
}

bool
HdxSkydomeTask::_UpdateParameterBuffer(
    const GfMatrix4f& invProjMatrix,
    const GfMatrix4f& viewToWorldMatrix,
    const GfMatrix4f& lightTransform)
{
    // All data is still the same, no need to update the storage buffer
    if (invProjMatrix     == _parameterData.invProjMatrix &&
        viewToWorldMatrix == _parameterData.viewToWorldMatrix &&
        lightTransform    == _parameterData.lightTransform) {
        return false;
    }

    _parameterData.invProjMatrix = invProjMatrix;
    _parameterData.viewToWorldMatrix = viewToWorldMatrix;
    _parameterData.lightTransform = lightTransform;
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
