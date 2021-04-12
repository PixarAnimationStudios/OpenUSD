//
// Copyright 2020 Pixar
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
