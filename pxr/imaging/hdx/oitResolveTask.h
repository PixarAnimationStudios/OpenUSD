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
#ifndef PXR_IMAGING_HDX_OIT_RESOLVE_TASK_H
#define PXR_IMAGING_HDX_OIT_RESOLVE_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/task.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

typedef boost::shared_ptr<class HdStRenderPassState>
    HdStRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdRenderPass>
    HdRenderPassSharedPtr;
typedef boost::shared_ptr<class HdStRenderPassShader>
    HdStRenderPassShaderSharedPtr;

/// \class HdxOitResolveTask
///
/// A task for resolving previous passes to pixels.
///
/// It is also responsible for allocating the OIT buffers, but it
/// leaves the clearing of the OIT buffers to the OIT render tasks.
/// OIT render tasks coordinate with the resolve task through
/// HdxOitResolveTask::OitBufferAccessor.
///
class HdxOitResolveTask : public HdTask 
{
public:
    HDX_API
    static bool IsOitEnabled();

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
    ///
    /// Allocates OIT buffers if requested by OIT render task
    HDX_API
    virtual void Prepare(HdTaskContext* ctx,
                         HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    ///
    /// Resolves OIT buffers
    HDX_API
    virtual void Execute(HdTaskContext* ctx) override;

private:
    HdxOitResolveTask() = delete;
    HdxOitResolveTask(const HdxOitResolveTask &) = delete;
    HdxOitResolveTask &operator =(const HdxOitResolveTask &) = delete;

    void _PrepareOitBuffers(
        HdTaskContext* ctx, 
        HdRenderIndex* renderIndex,
        GfVec2i const& screenSize);

    void _PrepareAovBindings(
        HdTaskContext* ctx,
        HdRenderIndex* renderIndex);

    HdRenderPassSharedPtr _renderPass;
    HdStRenderPassStateSharedPtr _renderPassState;
    HdStRenderPassShaderSharedPtr _renderPassShader;

    GfVec2i _screenSize;
    HdBufferArrayRangeSharedPtr _counterBar;
    HdBufferArrayRangeSharedPtr _dataBar;
    HdBufferArrayRangeSharedPtr _depthBar;
    HdBufferArrayRangeSharedPtr _indexBar;
    HdBufferArrayRangeSharedPtr _uniformBar;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
