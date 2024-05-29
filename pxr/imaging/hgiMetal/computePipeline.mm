//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiMetal/computePipeline.h"
#include "pxr/imaging/hgiMetal/conversions.h"
#include "pxr/imaging/hgiMetal/diagnostic.h"
#include "pxr/imaging/hgiMetal/resourceBindings.h"
#include "pxr/imaging/hgiMetal/shaderProgram.h"
#include "pxr/imaging/hgiMetal/shaderFunction.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalComputePipeline::HgiMetalComputePipeline(
    HgiMetal *hgi,
    HgiComputePipelineDesc const& desc)
    : HgiComputePipeline(desc)
{
    MTLComputePipelineDescriptor *stateDesc =
        [[MTLComputePipelineDescriptor alloc] init];

    // Create a new compute pipeline state object
    HGIMETAL_DEBUG_LABEL(stateDesc, _descriptor.debugName.c_str());
    
    HgiMetalShaderProgram const *metalProgram =
        static_cast<HgiMetalShaderProgram*>(_descriptor.shaderProgram.Get());
    
    stateDesc.computeFunction = metalProgram->GetComputeFunction();
    
    NSError *error = NULL;
    _computePipelineState = [hgi->GetPrimaryDevice()
        newComputePipelineStateWithDescriptor:stateDesc
                                      options:MTLPipelineOptionNone
                                   reflection:nil
                                        error:&error];
    [stateDesc release];

    if (!_computePipelineState) {
        NSString *err = [error localizedDescription];
        TF_WARN("Failed to create compute pipeline state, error %s",
            [err UTF8String]);
    }
}

HgiMetalComputePipeline::~HgiMetalComputePipeline()
{
}

void
HgiMetalComputePipeline::BindPipeline(
    id<MTLComputeCommandEncoder> computeEncoder)
{
    [computeEncoder setComputePipelineState:_computePipelineState];
}

id<MTLComputePipelineState> HgiMetalComputePipeline::GetMetalPipelineState()
{
    return _computePipelineState;
}

PXR_NAMESPACE_CLOSE_SCOPE
