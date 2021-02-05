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
#ifndef PXR_IMAGING_HDX_RENDER_TASK_H
#define PXR_IMAGING_HDX_RENDER_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hdx/renderSetupTask.h"  // for short-term compatibility.
#include "pxr/imaging/hdSt/renderPassState.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

using HdRenderPassStateSharedPtr = std::shared_ptr<class HdRenderPassState>;
using HdRenderPassSharedPtr = std::shared_ptr<class HdRenderPass>;
using HdxRenderSetupTaskSharedPtr = std::shared_ptr<class HdxRenderSetupTask>;

/// \class HdxRenderTask
///
/// A task for rendering geometry to pixels.
///
/// Rendering state management can be handled two ways:
/// 1.) An application can create an HdxRenderTask and pass it the
///     HdxRenderTaskParams struct as "params".
/// 2.) An application can create an HdxRenderSetupTask and an
///     HdxRenderTask, and pass params to the setup task. In this case
///     the setup task must run first.
///
/// Parameter unpacking is handled by HdxRenderSetupTask; in case #1,
/// HdxRenderTask creates a dummy setup task internally to manage the sync
/// process.
///
/// Case #2 introduces complexity; the benefit is that by changing which
/// setup task you run before the render task, you can change the render
/// parameters without incurring a hydra sync or rebuilding any resources.
///
class HdxRenderTask : public HdxTask
{
public:
    HDX_API
    HdxRenderTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxRenderTask() override;

    /// Hooks for progressive rendering (delegated to renderpasses).
    HDX_API
    bool IsConverged() const override;

    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

    /// Collect Render Tags used by the task.
    HDX_API
    const TfTokenVector &GetRenderTags() const override;

protected:
    /// Sync the render pass resources
    HDX_API
    void _Sync(HdSceneDelegate* delegate,
               HdTaskContext* ctx,
               HdDirtyBits* dirtyBits) override;

    HDX_API
    HdRenderPassStateSharedPtr _GetRenderPassState(HdTaskContext *ctx) const;

    // XXX: Storm specific API
    // While HdDrawItem is currently a core-Hydra concept, it'll be moved
    // to Storm. Until then, allow querying the render pass to know if there's
    // draw submission work.

    // Returns whether the render pass has any draw items to submit.
    // For non-Storm backends, this returns true.
    // When using with Storm tasks, make sure to call it after
    // HdxRenderTask::Prepare().
    HDX_API
    bool _HasDrawItems() const;

private:
    HdRenderPassSharedPtr _pass;
    TfTokenVector _renderTags;

    // Optional internal render setup task, for params unpacking.
    HdxRenderSetupTaskSharedPtr _setupTask;

    // XXX: Storm specific API
    // Setup additional state that HdStRenderPassState requires.
    void _SetHdStRenderPassState(HdTaskContext *ctx,
                                 HdStRenderPassState *renderPassState);
    
    // Inspect the AOV bindings to determine if any of them need to be cleared.
    bool _NeedToClearAovs(HdRenderPassStateSharedPtr const &renderPassState)
        const;

    HdxRenderTask() = delete;
    HdxRenderTask(const HdxRenderTask &) = delete;
    HdxRenderTask &operator =(const HdxRenderTask &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_RENDER_TASK_H
