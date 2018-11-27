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
#ifndef HDX_RENDER_TASK_H
#define HDX_RENDER_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hdx/renderSetupTask.h"  // for short-term compatibility.
#include "pxr/imaging/hdSt/renderPassState.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;
typedef boost::shared_ptr<class HdxRenderSetupTask> HdxRenderSetupTaskSharedPtr;
typedef std::vector<HdRenderPassSharedPtr> HdRenderPassSharedPtrVector;

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
class HdxRenderTask : public HdTask
{
public:
    HDX_API
    HdxRenderTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxRenderTask();

    /// Hooks for progressive rendering (delegated to renderpasses).
    bool IsConverged() const;

    /// Sync the render pass resources
    HDX_API
    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override;

    /// Execute render pass task
    HDX_API
    virtual void Execute(HdTaskContext* ctx) override;


private:
    HdRenderPassSharedPtrVector _passes;

    // Optional internal render setup task, for params unpacking.
    HdxRenderSetupTaskSharedPtr _setupTask;

    // Setup additional state that HdStRenderPassState requires.
    // XXX: This should be moved to hdSt!
    void _SetHdStRenderPassState(HdTaskContext *ctx,
                                 HdStRenderPassState *renderPassState);

    HdxRenderTask() = delete;
    HdxRenderTask(const HdxRenderTask &) = delete;
    HdxRenderTask &operator =(const HdxRenderTask &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDX_RENDER_TASK_H
