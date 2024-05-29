//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIGL_PIPELINE_H
#define PXR_IMAGING_HGIGL_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"
#include "pxr/imaging/hgiGL/api.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiGL;

/// \class HgiGLPipeline
///
/// OpenGL implementation of HgiGraphicsPipeline.
///
class HgiGLGraphicsPipeline final : public HgiGraphicsPipeline
{
public:
    HGIGL_API
    ~HgiGLGraphicsPipeline() override;

    /// Apply pipeline state
    HGIGL_API
    void BindPipeline();

protected:
    friend class HgiGL;

    HGIGL_API
    HgiGLGraphicsPipeline(HgiGL const* hgi,
                          HgiGraphicsPipelineDesc const& desc);

private:
    HgiGLGraphicsPipeline() = delete;
    HgiGLGraphicsPipeline & operator=(const HgiGLGraphicsPipeline&) = delete;
    HgiGLGraphicsPipeline(const HgiGLGraphicsPipeline&) = delete;

    HgiGL const *_hgi;
    uint32_t _vao;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
