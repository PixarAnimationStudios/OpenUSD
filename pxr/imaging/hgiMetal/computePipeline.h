//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_METAL_COMPUTE_PIPELINE_H
#define PXR_IMAGING_HGI_METAL_COMPUTE_PIPELINE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/computePipeline.h"

#include "pxr/imaging/hgiMetal/api.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiMetal;

/// \class HgiMetalComputePipeline
///
/// Metal implementation of HgiComputePipeline.
///
class HgiMetalComputePipeline final : public HgiComputePipeline
{
public:
    HGIMETAL_API
    HgiMetalComputePipeline(HgiMetal *hgi, HgiComputePipelineDesc const& desc);

    HGIMETAL_API
    ~HgiMetalComputePipeline() override;

    /// Apply pipeline state
    HGIMETAL_API
    void BindPipeline(id<MTLComputeCommandEncoder> computeEncoder);
    
    HGIMETAL_API
    id<MTLComputePipelineState> GetMetalPipelineState();

private:
    HgiMetalComputePipeline() = delete;
    HgiMetalComputePipeline & operator=(const HgiMetalComputePipeline&) = delete;
    HgiMetalComputePipeline(const HgiMetalComputePipeline&) = delete;
    
    id<MTLComputePipelineState> _computePipelineState;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
