//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/imaging/hdx/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxTask::HdxTask(SdfPath const& id)
    : HdTask(id)
    , _hgi(nullptr)
{
}

HdxTask::~HdxTask() = default;

bool
HdxTask::IsConverged() const
{
    return true;
}

void
HdxTask::Sync(
    HdSceneDelegate* delegate,
    HdTaskContext* ctx,
    HdDirtyBits* dirtyBits)
{
    // Hgi is pushed is provided by the application and pushed into the
    // task context by hydra.
    // We only have to find the Hgi driver once as it should not change.
    // All gpu resources (in tasks and Storm) are created with a specific
    // Hgi device so we (correctly) assume Hgi* will not change during a
    // Hydra session. Not all tasks need Hgi, so we do not consider it an
    // error here to not find Hgi.
    if (!_hgi) {
        _hgi = HdTask::_GetDriver<Hgi*>(ctx, HgiTokens->renderDriver);
    }

    // Proceeed with the Sync Phase
    _Sync(delegate, ctx, dirtyBits);
}

void
HdxTask::_ToggleRenderTarget(HdTaskContext* ctx)
{
    if (!_HasTaskContextData(ctx, HdAovTokens->color)) {
        return;
    }

    if (_HasTaskContextData(ctx, HdxAovTokens->colorIntermediate)) {
        _SwapTextures(ctx, HdAovTokens->color, HdxAovTokens->colorIntermediate);
    }
}

void
HdxTask::_ToggleDepthTarget(HdTaskContext* ctx)
{
    if (!_HasTaskContextData(ctx, HdAovTokens->depth)) {
        return;
    }

    if (_HasTaskContextData(ctx, HdxAovTokens->depthIntermediate)) {
        _SwapTextures(ctx, HdAovTokens->depth, HdxAovTokens->depthIntermediate);
    }
}

void
HdxTask::_SwapTextures(
    HdTaskContext* ctx,
    const TfToken& textureToken,
    const TfToken& textureIntermediateToken)
{
    VtValue& texture = (*ctx)[textureToken];
    VtValue& textureIntermediate = (*ctx)[textureIntermediateToken];
    std::swap(texture, textureIntermediate);
}

Hgi*
HdxTask::_GetHgi() const
{
    if (ARCH_UNLIKELY(!_hgi)) {
        TF_CODING_ERROR("Hgi driver missing. See HdxTask::Sync.");
    }
    return _hgi;
}

PXR_NAMESPACE_CLOSE_SCOPE

