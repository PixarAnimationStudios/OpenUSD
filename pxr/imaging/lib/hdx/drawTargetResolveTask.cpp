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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdx/drawTargetResolveTask.h"
#include "pxr/imaging/hdx/drawTargetRenderPass.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hdSt/drawTarget.h"

#include "pxr/imaging/glf/drawTarget.h"

PXR_NAMESPACE_OPEN_SCOPE

typedef std::unique_ptr<HdxDrawTargetRenderPass>
                                               HdxDrawTargetRenderPassUniquePtr;
typedef std::vector<HdxDrawTargetRenderPassUniquePtr>
                                          HdxDrawTargetRenderPassUniquePtrVector;

HdxDrawTargetResolveTask::HdxDrawTargetResolveTask(HdSceneDelegate* delegate,
                                                   SdfPath const& id)
 : HdSceneTask(delegate, id)
{
}

HdxDrawTargetResolveTask::~HdxDrawTargetResolveTask()
{
}

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
HdxDrawTargetResolveTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Extract the list of render pass for draw targets from the task context.
    // This list is set from drawTargetTask.cpp during Sync phase.
    HdTaskContext::const_iterator valueIt =
                                   ctx->find(HdxTokens->drawTargetRenderPasses);
    if (valueIt == ctx->cend()) {
        TF_CODING_ERROR("drawTargetRenderPasses token missing from "
                        "task context");
        return;
    }

    std::vector<GlfDrawTarget*> drawTargets;


    const VtValue &valueVt = (valueIt->second);
    if (valueVt.IsHolding<HdxDrawTargetRenderPass *>()) {
        drawTargets.resize(1);

        HdxDrawTargetRenderPass *pass =
                              valueVt.UncheckedGet<HdxDrawTargetRenderPass *>();

        drawTargets[0] = boost::get_pointer(pass->GetDrawTarget());

    } else if (valueVt.IsHolding<HdxDrawTargetRenderPassUniquePtrVector *>()) {
        HdxDrawTargetRenderPassUniquePtrVector *passes =
               valueVt.UncheckedGet<HdxDrawTargetRenderPassUniquePtrVector *>();

        // Iterate through all renderpass (drawtarget renderpass), extract the
        // draw target and resolve them if needed. We need to resolve them to 
        // regular buffers so use them in the rest of the pipeline.
 
        size_t numDrawTargets = passes->size();

        drawTargets.resize(numDrawTargets);
        for (size_t i = 0; i < numDrawTargets; ++i) {
            drawTargets[i] = boost::get_pointer((*passes)[i]->GetDrawTarget());
        }
    } else {
        TF_CODING_ERROR("drawTargetRenderPasses in task context is of "
                        "unexpected type");
        return;
    }

    if (!drawTargets.empty()) {
        GlfDrawTarget::Resolve(drawTargets);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

