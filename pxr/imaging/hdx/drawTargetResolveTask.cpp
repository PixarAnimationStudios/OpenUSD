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
#include "pxr/imaging/hdx/drawTargetResolveTask.h"
#include "pxr/imaging/hdx/drawTargetRenderPass.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/glf/drawTarget.h"

PXR_NAMESPACE_OPEN_SCOPE

HdxDrawTargetResolveTask::HdxDrawTargetResolveTask(HdSceneDelegate* delegate,
                                                   SdfPath const& id)
 : HdTask(id)
{
}

HdxDrawTargetResolveTask::~HdxDrawTargetResolveTask() = default;

void
HdxDrawTargetResolveTask::Sync(HdSceneDelegate* delegate,
                               HdTaskContext* ctx,
                               HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxDrawTargetResolveTask::Prepare(HdTaskContext* ctx,
                                  HdRenderIndex* renderIndex)
{
}

void
HdxDrawTargetResolveTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Extract the list of render pass for draw targets from the task context.
    // This list is set from drawTargetTask.cpp during Sync phase.
    const HdTaskContext::const_iterator valueIt =
        ctx->find(HdxTokens->drawTargetRenderPasses);
    if (valueIt == ctx->cend()) {
        return;
    }

    const VtValue &valueVt = valueIt->second;
    if (!valueVt.IsHolding<HdxDrawTargetRenderPass *>()) {
        TF_CODING_ERROR("drawTargetRenderPasses in task context is of "
                        "unexpected type");
        return;
    }        

    HdxDrawTargetRenderPass * const pass =
        valueVt.UncheckedGet<HdxDrawTargetRenderPass *>();
    GlfDrawTarget * const drawTarget = 
        boost::get_pointer(pass->GetDrawTarget());
    if (!drawTarget) {
        return;
    }

    drawTarget->Resolve();
}

PXR_NAMESPACE_CLOSE_SCOPE

