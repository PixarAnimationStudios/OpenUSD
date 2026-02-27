//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "renderDelegate.h"

#include "hd3DGaussianSplat.h"
#include "renderBuffer.h"
#include "renderParam.h"
#include "renderPass.h"
#include "renderer.h"

#include "debugCodes.h"

#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/renderThread.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Supported Hydra prim types.
const TfTokenVector HdParticleFieldRenderDelegate::SUPPORTED_RPRIM_TYPES = {
    HdPrimTypeTokens->particleField};
const TfTokenVector HdParticleFieldRenderDelegate::SUPPORTED_SPRIM_TYPES = {
    HdPrimTypeTokens->camera};
const TfTokenVector HdParticleFieldRenderDelegate::SUPPORTED_BPRIM_TYPES = {
    HdPrimTypeTokens->renderBuffer};

static void _RenderCallback(
    HdParticleFieldRenderer* renderer, HdRenderThread* renderThread)
{
    renderer->Clear();
    renderer->Render(renderThread);
}

HdParticleFieldRenderDelegate::HdParticleFieldRenderDelegate()
    : HdRenderDelegate() { _Setup(); }

HdParticleFieldRenderDelegate::HdParticleFieldRenderDelegate(
    const HdRenderSettingsMap& settingsMap)
    : HdRenderDelegate(settingsMap)
{
    _Setup();
}

void HdParticleFieldRenderDelegate::_Setup() {

    // Store top-level objects inside a render param that can be
    // passed to prims during Sync(). Also pass a handle to the render thread.
    _renderParam = std::make_unique<HdParticleFieldRenderParam>(&_renderer,
        &_renderThread, &_sceneVersion);

    // Set the background render thread's rendering entrypoint to
    // HdGaussianSplatsRenderer::Render.
    _renderThread.SetRenderCallback(std::bind(_RenderCallback, &_renderer,
        &_renderThread));
    // Start the background render thread.
    _renderThread.StartThread();

    _resourceRegistry =
        std::shared_ptr<HdResourceRegistry>(new HdResourceRegistry());
}

HdParticleFieldRenderDelegate::~HdParticleFieldRenderDelegate() {
    _renderThread.StopThread();

    // Clean resources.
    _renderParam.reset();
    _resourceRegistry.reset();
}

TfTokenVector const&
HdParticleFieldRenderDelegate::GetSupportedRprimTypes() const {
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const&
HdParticleFieldRenderDelegate::GetSupportedSprimTypes() const {
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const&
HdParticleFieldRenderDelegate::GetSupportedBprimTypes() const {
    return SUPPORTED_BPRIM_TYPES;
}

HdRenderParam* HdParticleFieldRenderDelegate::GetRenderParam() const {
    return _renderParam.get();
}

HdRenderSettingDescriptorList
HdParticleFieldRenderDelegate::GetRenderSettingDescriptors() const {
    return _settingDescriptors;
}

HdResourceRegistrySharedPtr
HdParticleFieldRenderDelegate::GetResourceRegistry() const {
    return _resourceRegistry;
}

HdRenderPassSharedPtr
HdParticleFieldRenderDelegate::CreateRenderPass(
    HdRenderIndex* index, const HdRprimCollection& collection)
{
    return HdRenderPassSharedPtr(
        new HdParticleFieldRenderPass(index, collection, &_renderer,
            &_renderThread, &_sceneVersion));
}

void HdParticleFieldRenderDelegate::CommitResources(HdChangeTracker* tracker) {}

HdRprim* HdParticleFieldRenderDelegate::CreateRprim(
    const TfToken& typeId, const SdfPath& rprimId)
{
    if (typeId == HdPrimTypeTokens->particleField) {
        TF_DEBUG(HDPARTICLEFIELD_GENERAL).Msg(
            "[%s] Create HdGaussianSplats Rprim type %s id %s\n",
            TF_FUNC_NAME().c_str(), typeId.GetText(), rprimId.GetText());

        return new Hd3DGaussianSplat(rprimId);
    } else {
        TF_CODING_ERROR("Unknown Rprim type=%s id=%s",
            typeId.GetText(), rprimId.GetText());
    }
    return nullptr;
}

void HdParticleFieldRenderDelegate::DestroyRprim(HdRprim* rprim) {
    TF_DEBUG(HDPARTICLEFIELD_GENERAL).Msg(
        "[%s] Destroy Rprim id %s\n",
        TF_FUNC_NAME().c_str(), rprim->GetId().GetText());
    delete rprim;
}

HdSprim* HdParticleFieldRenderDelegate::CreateSprim(
    const TfToken& typeId, const SdfPath& sprimId)
{
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdCamera(sprimId);
    } else {
        TF_CODING_ERROR("Unknown Sprim type=%s id=%s",
            typeId.GetText(), sprimId.GetText());
    }
    return nullptr;
}

HdSprim* HdParticleFieldRenderDelegate::CreateFallbackSprim(
    const TfToken& typeId)
{
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdCamera(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Fallback Sprim type=%s id=%s",
            typeId.GetText(), typeId.GetText());
    }
    return nullptr;
}

void HdParticleFieldRenderDelegate::DestroySprim(HdSprim* sprim) {
    delete sprim;
}

HdBprim* HdParticleFieldRenderDelegate::CreateBprim(
    const TfToken& typeId, const SdfPath& bprimId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdParticleFieldRenderBuffer(bprimId);
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }

    return nullptr;
}

HdBprim* HdParticleFieldRenderDelegate::CreateFallbackBprim(
    const TfToken& typeId)
{
    if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdParticleFieldRenderBuffer(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void HdParticleFieldRenderDelegate::DestroyBprim(HdBprim* bprim) {
    delete bprim;
}

HdInstancer* HdParticleFieldRenderDelegate::CreateInstancer(
    HdSceneDelegate* delegate, const SdfPath& id)
{
    TF_CODING_ERROR("Creating Instancer not supported id=%s", id.GetText());
    return nullptr;
}

void HdParticleFieldRenderDelegate::DestroyInstancer(HdInstancer* instancer)
{
    TF_CODING_ERROR("Destroy instancer not supported");
}

HdAovDescriptor HdParticleFieldRenderDelegate::GetDefaultAovDescriptor(
    const TfToken& aovName) const
{
    if (aovName == HdAovTokens->color) {
        return HdAovDescriptor(
            HdFormatUNorm8Vec4, true, VtValue(GfVec4f(0.0f)));
    } else if (aovName == HdAovTokens->depth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(1.0f));
    } else if (aovName == HdAovTokens->primId ||
               aovName == HdAovTokens->instanceId ||
               aovName == HdAovTokens->elementId) {
        return HdAovDescriptor(HdFormatInt32, false, VtValue(-1));
    }

    return HdAovDescriptor();
}

bool HdParticleFieldRenderDelegate::IsPauseSupported() const { return true; }

bool HdParticleFieldRenderDelegate::Pause() {
    _renderThread.PauseRender();
    return true;
}

bool HdParticleFieldRenderDelegate::Resume() {
    _renderThread.ResumeRender();
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
