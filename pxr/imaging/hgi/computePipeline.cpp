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
#include "pxr/imaging/hgi/computePipeline.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiComputeShaderConstantsDesc::HgiComputeShaderConstantsDesc()
    : byteSize(0)
{
}

bool operator==(
    const HgiComputeShaderConstantsDesc& lhs,
    const HgiComputeShaderConstantsDesc& rhs)
{
    return lhs.byteSize == rhs.byteSize;
}

bool operator!=(
    const HgiComputeShaderConstantsDesc& lhs,
    const HgiComputeShaderConstantsDesc& rhs)
{
    return !(lhs == rhs);
}

HgiComputePipelineDesc::HgiComputePipelineDesc()
    : shaderProgram()
{
}

bool operator==(
    const HgiComputePipelineDesc& lhs,
    const HgiComputePipelineDesc& rhs)
{
    return lhs.debugName == rhs.debugName &&
           lhs.shaderProgram == rhs.shaderProgram &&
           lhs.shaderConstantsDesc == rhs.shaderConstantsDesc;
}

bool operator!=(
    const HgiComputePipelineDesc& lhs,
    const HgiComputePipelineDesc& rhs)
{
    return !(lhs == rhs);
}

HgiComputePipeline::HgiComputePipeline(HgiComputePipelineDesc const& desc)
    : _descriptor(desc)
{
}

HgiComputePipeline::~HgiComputePipeline() = default;

HgiComputePipelineDesc const&
HgiComputePipeline::GetDescriptor() const
{
    return _descriptor;
}

PXR_NAMESPACE_CLOSE_SCOPE
