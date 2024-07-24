//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_OIT_RENDER_TASK_H
#define PXR_IMAGING_HDX_OIT_RENDER_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hdx/renderTask.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

using HdStRenderPassShaderSharedPtr = std::shared_ptr<HdStRenderPassShader>;

/// \class HdxOitRenderTask
///
/// A task for rendering transparent geometry into OIT buffers.
/// Its companion task, OITResolveTask, will blend the buffers to screen.
///
class HdxOitRenderTask : public HdxRenderTask 
{
public:
    HDX_API
    HdxOitRenderTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxOitRenderTask() override;

    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx, 
                 HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

protected:
    /// Sync the render pass resources
    HDX_API
    void _Sync(HdSceneDelegate* delegate,
               HdTaskContext* ctx,
               HdDirtyBits* dirtyBits) override;

private:
    HdxOitRenderTask() = delete;
    HdxOitRenderTask(const HdxOitRenderTask &) = delete;
    HdxOitRenderTask &operator =(const HdxOitRenderTask &) = delete;

    HdStRenderPassShaderSharedPtr _oitTranslucentRenderPassShader;
    HdStRenderPassShaderSharedPtr _oitOpaqueRenderPassShader;
    const bool _isOitEnabled;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_OIT_RENDER_TASK_H
