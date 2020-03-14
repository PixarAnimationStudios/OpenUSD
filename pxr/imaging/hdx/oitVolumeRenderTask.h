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
#ifndef PXR_IMAGING_HDX_OIT_VOLUME_RENDER_TASK_H
#define PXR_IMAGING_HDX_OIT_VOLUME_RENDER_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hdx/renderTask.h"

#include "pxr/imaging/hdSt/renderPassState.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxOitVolumeRenderTask
///
/// A task for rendering transparent geometry into OIT buffers.
/// Its companion task, OITResolveTask, will blend the buffers to screen.
///
class HdxOitVolumeRenderTask : public HdxRenderTask 
{
public:
    HDX_API
    HdxOitVolumeRenderTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxOitVolumeRenderTask();

    /// Sync the render pass resources
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
    HdxOitVolumeRenderTask() = delete;
    HdxOitVolumeRenderTask(const HdxOitVolumeRenderTask &) = delete;
    HdxOitVolumeRenderTask &operator =(const HdxOitVolumeRenderTask &) = delete;

    HdStRenderPassShaderSharedPtr _oitVolumeRenderPassShader;
    const bool _isOitEnabled;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_OIT_VOLUME_RENDER_TASK_H
