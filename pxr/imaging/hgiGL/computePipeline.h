//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIGL_COMPUTE_PIPELINE_H
#define PXR_IMAGING_HGIGL_COMPUTE_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgiGL/api.h"


PXR_NAMESPACE_OPEN_SCOPE


/// \class HgiGLComputePipeline
///
/// OpenGL implementation of HgiComputePipeline.
///
class HgiGLComputePipeline final : public HgiComputePipeline
{
public:
    HGIGL_API
    ~HgiGLComputePipeline() override;

    /// Apply pipeline state
    HGIGL_API
    void BindPipeline();

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLComputePipeline(HgiComputePipelineDesc const& desc);

private:
    HgiGLComputePipeline() = delete;
    HgiGLComputePipeline & operator=(const HgiGLComputePipeline&) = delete;
    HgiGLComputePipeline(const HgiGLComputePipeline&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
