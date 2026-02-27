//
//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/errorMark.h"

#include "pxr/imaging/garch/glDebugWindow.h"
#include "pxr/imaging/hd/driver.h"
#include "pxr/imaging/hd/engine.h"
#include "pxr/imaging/hdSt/renderDelegate.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/tokens.h"
#include "pxr/imaging/hdx/fullscreenShader.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/taskController.h"
#include "pxr/imaging/hdx/unitTestDelegate.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

class FullScreenEffectTask : public HdxTask
{
public:
    void Prepare(HdTaskContext* context, HdRenderIndex* renderIndex) override
    {
        _renderIndex = renderIndex;
    }

    void Execute(HdTaskContext* context) override
    {
        HdRenderPassStateSharedPtr renderPassState =
            _GetRenderPassState(context);
        const HdStRenderPassState* pHdStRenderPassState =
            dynamic_cast<HdStRenderPassState*>(renderPassState.get());
        if (!pHdStRenderPassState)
            return;

        HgiGraphicsCmdsDesc gfxCmdsDesc =
            pHdStRenderPassState->MakeGraphicsCmdsDesc(_renderIndex);

        GfVec4i viewport = pHdStRenderPassState->ComputeViewport();

        HgiTextureHandle colorDst = gfxCmdsDesc.colorTextures.empty() ?
            HgiTextureHandle() :
            gfxCmdsDesc.colorTextures[0];
        HgiTextureHandle colorResolveDst =
            gfxCmdsDesc.colorResolveTextures.empty() ?
            HgiTextureHandle() :
            gfxCmdsDesc.colorResolveTextures[0];
        HgiTextureHandle depthDst = gfxCmdsDesc.depthTexture;
        HgiTextureHandle depthResolveDst = gfxCmdsDesc.depthResolveTexture;

        _fullScreenShader->Draw(
            colorDst, colorResolveDst, depthDst, depthResolveDst, viewport);
    }

    FullScreenEffectTask(HdSceneDelegate* pDelegate, SdfPath const& id)
        : HdxTask(id)
    {
    }

private:
    void _SetUpFullScreenShader()
    {
        HgiShaderFunctionDesc fragDesc;
        fragDesc.debugName = shaderFunctionName.GetString();
        fragDesc.shaderStage = HgiShaderStageFragment;

        HgiShaderFunctionAddStageInput(&fragDesc, "uvOut", "vec2");
        HgiShaderFunctionAddStageOutput(
            &fragDesc, "hd_FragColor", "vec4", "color");

        TfToken path("fullScreenEffect.glslfx");
        _fullScreenShader->SetProgram(path, shaderFunctionName, fragDesc);
    }

    void _Sync(HdSceneDelegate* pDelegate, HdTaskContext* pCtx,
        HdDirtyBits* dirtyBits) override
    {
        if (!_fullScreenShader) {
            _fullScreenShader = std::make_unique<HdxFullscreenShader>(
                _GetHgi(), shaderFunctionName.GetString());
            _SetUpFullScreenShader();
        }

        *dirtyBits = HdChangeTracker::Clean;
    }

    HdRenderPassStateSharedPtr _GetRenderPassState(HdTaskContext* pCtx) const
    {
        HdRenderPassStateSharedPtr pRenderPassState;
        _GetTaskContextData(
            pCtx, HdxTokens->renderPassState, &pRenderPassState);
        return pRenderPassState;
    }

    HdRenderIndex* _renderIndex;
    std::unique_ptr<HdxFullscreenShader> _fullScreenShader;

    static const TfToken shaderFunctionName;
};

const TfToken FullScreenEffectTask::shaderFunctionName{"FullScreenEffect"};

int
main(int argc, char* argv[])
{
    TfErrorMark mark;

    HdPerfLog& perfLog = HdPerfLog::GetInstance();
    perfLog.Enable();

    GarchGLDebugWindow window("Hdx Test", 256, 256);
    window.Init();

    // Hgi and HdDriver should be constructed before HdEngine to ensure they
    // are destructed last. Hgi may be used during engine/delegate destruction.
    HgiUniquePtr hgi = Hgi::CreatePlatformDefaultHgi();
    HdDriver driver{HgiTokens->renderDriver, VtValue(hgi.get())};

    HdStRenderDelegate renderDelegate;
    const std::unique_ptr<HdRenderIndex> index(
        HdRenderIndex::New(&renderDelegate, {&driver}));
    const auto delegate = std::make_unique<Hdx_UnitTestDelegate>(index.get());
    const auto taskController = std::make_unique<HdxTaskController>(
        index.get(), SdfPath("/taskController"));

    HdEngine engine;

    HdTaskSharedPtrVector tasks;

    // setup AOVs
    const SdfPath colorAovId = SdfPath("/aov_color");
    const SdfPath depthAovId = SdfPath("/aov_depth");
    {
        HdRenderPassAovBindingVector aovBindings;
        HdRenderPassAovBinding colorAovBinding;
        const HdAovDescriptor colorAovDesc =
            renderDelegate.GetDefaultAovDescriptor(HdAovTokens->color);
        colorAovBinding.aovName = HdAovTokens->color;
        colorAovBinding.clearValue = VtValue(GfVec4f(0.1f, 0.1f, 0.1f, 1.0f));
        colorAovBinding.renderBufferId = colorAovId;
        colorAovBinding.aovSettings = colorAovDesc.aovSettings;
        aovBindings.push_back(std::move(colorAovBinding));

        HdRenderBufferDescriptor colorRbDesc;
        colorRbDesc.dimensions = GfVec3i(256, 256, 1);
        colorRbDesc.format = colorAovDesc.format;
        colorRbDesc.multiSampled = false;
        delegate->AddRenderBuffer(colorAovId, colorRbDesc);

        HdRenderPassAovBinding depthAovBinding;
        const HdAovDescriptor depthAovDesc =
            renderDelegate.GetDefaultAovDescriptor(HdAovTokens->depth);
        depthAovBinding.aovName = HdAovTokens->depth;
        depthAovBinding.clearValue = VtValue(1.f);
        depthAovBinding.renderBufferId = depthAovId;
        depthAovBinding.aovSettings = depthAovDesc.aovSettings;
        aovBindings.push_back(std::move(depthAovBinding));

        HdRenderBufferDescriptor depthRbDesc;
        depthRbDesc.dimensions = GfVec3i(256, 256, 1);
        depthRbDesc.format = depthAovDesc.format;
        depthRbDesc.multiSampled = false;
        delegate->AddRenderBuffer(depthAovId, depthRbDesc);

        SdfPath renderSetupTask("/renderSetupTask");
        delegate->AddRenderSetupTask(renderSetupTask);
        VtValue vParam =
            delegate->GetTaskParam(renderSetupTask, HdTokens->params);
        HdxRenderTaskParams param = vParam.Get<HdxRenderTaskParams>();
        param.aovBindings = aovBindings;
        delegate->SetTaskParam(
            renderSetupTask, HdTokens->params, VtValue(param));
        tasks.push_back(index->GetTask(renderSetupTask));
    }

    const SdfPath fullScreenEffectTaskId("/fullScreenEffectTask");
    index->InsertTask<FullScreenEffectTask>(
        delegate.get(), fullScreenEffectTaskId);
    tasks.push_back(index->GetTask(fullScreenEffectTaskId));

    engine.Execute(index.get(), &tasks);
    delegate->WriteRenderBufferToFile(colorAovId, "color.png");

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
