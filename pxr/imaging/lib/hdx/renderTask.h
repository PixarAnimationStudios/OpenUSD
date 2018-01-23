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

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

typedef std::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef std::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;
typedef std::shared_ptr<class HdxRenderSetupTask> HdxRenderSetupTaskSharedPtr;
typedef std::vector<HdRenderPassSharedPtr> HdRenderPassSharedPtrVector;

/// \class HdxRenderTask
///
/// A task for rendering geometry to pixels.
///
class HdxRenderTask : public HdSceneTask 
{
public:
    HDX_API
    HdxRenderTask(HdSceneDelegate* delegate, SdfPath const& id);

    /// Hooks for progressive rendering (delegated to renderpasses).

    // XXX: If the render task collection param will change in the next Sync(),
    // IsConverged() should be ignored since the renderpasses will be rebuilt.
    void ResetImage();
    bool IsConverged() const;

protected:
    /// Execute render pass task
    HDX_API
    virtual void _Execute(HdTaskContext* ctx);

    /// Sync the render pass resources
    HDX_API
    virtual void _Sync(HdTaskContext* ctx);

private:
    HdRenderPassSharedPtrVector _passes;

    // XXX: temp members to keep compatibility (optional)
    HdxRenderSetupTaskSharedPtr _setupTask;

    // XXX: Setup additional state that HdStRenderPassState requires.
    // This should be moved to hdSt!
    void _SetHdStRenderPassState(HdTaskContext *ctx,
                                 HdStRenderPassState *renderPassState);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDX_RENDER_TASK_H
