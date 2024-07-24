//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_PIPELINE_H
#define PXR_IMAGING_HGI_METAL_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"

#include "pxr/imaging/hgiMetal/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiMetal;

/// \class HgiMetalPipeline
///
/// Metal implementation of HgiGraphicsPipeline.
///
class HgiMetalGraphicsPipeline final : public HgiGraphicsPipeline
{
public:
    HGIMETAL_API
    HgiMetalGraphicsPipeline(HgiMetal *hgi, HgiGraphicsPipelineDesc const& desc);

    HGIMETAL_API
    ~HgiMetalGraphicsPipeline() override;

    /// Apply pipeline state
    HGIMETAL_API
    void BindPipeline(id<MTLRenderCommandEncoder> renderEncoder);

private:
    HgiMetalGraphicsPipeline() = delete;
    HgiMetalGraphicsPipeline& operator=(const HgiMetalGraphicsPipeline&)=delete;
    HgiMetalGraphicsPipeline(const HgiMetalGraphicsPipeline&) = delete;
    
    void _CreateVertexDescriptor();
    void _CreateDepthStencilState(HgiMetal *hgi);
    void _CreateRenderPipelineState(HgiMetal *hgi);

    MTLVertexDescriptor *_vertexDescriptor;
    id<MTLDepthStencilState> _depthStencilState;
    id<MTLRenderPipelineState> _renderPipelineState;
    id<MTLBuffer> _constantTessFactors;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
