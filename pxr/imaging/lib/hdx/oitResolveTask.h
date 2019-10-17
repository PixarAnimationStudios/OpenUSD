//
// Copyright 2019 Pixar
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
#ifndef HDX_RESOLVE_TASK_H
#define HDX_RESOLVE_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/task.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

typedef boost::shared_ptr<class HdRenderPassState>
    HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdRenderPass>
    HdRenderPassSharedPtr;
typedef boost::shared_ptr<class HdStRenderPassShader>
    HdStRenderPassShaderSharedPtr;

/// \class HdxOitResolveTask
///
/// A task for resolving previous passes to pixels.
///
class HdxOitResolveTask : public HdTask 
{
public:
    HDX_API
    HdxOitResolveTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxOitResolveTask();

    /// Sync the resolve pass resources
    HDX_API
    virtual void Sync(HdSceneDelegate* delegate,
                      HdTaskContext* ctx,
                      HdDirtyBits* dirtyBits) override;

    /// Prepare the tasks resources
    HDX_API
    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    HDX_API
    virtual void Execute(HdTaskContext* ctx) override;

private:
    HdRenderPassSharedPtr _renderPass;
    HdRenderPassStateSharedPtr _renderPassState;
    HdStRenderPassShaderSharedPtr _renderPassShader;

    HdxOitResolveTask() = delete;
    HdxOitResolveTask(const HdxOitResolveTask &) = delete;
    HdxOitResolveTask &operator =(const HdxOitResolveTask &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
