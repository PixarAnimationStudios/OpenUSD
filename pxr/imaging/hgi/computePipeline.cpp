//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
