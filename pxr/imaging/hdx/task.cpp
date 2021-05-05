//
// Copyright 2018 Pixar
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

    HgiTextureHandle aovTexture, aovTextureIntermediate;
    
    if (_HasTaskContextData(ctx, HdxAovTokens->colorIntermediate)) {
        _GetTaskContextData(ctx, HdAovTokens->color, &aovTexture);
        _GetTaskContextData(
            ctx, HdxAovTokens->colorIntermediate, &aovTextureIntermediate);
        (*ctx)[HdAovTokens->color] = VtValue(aovTextureIntermediate);
        (*ctx)[HdxAovTokens->colorIntermediate] = VtValue(aovTexture);
    }
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

