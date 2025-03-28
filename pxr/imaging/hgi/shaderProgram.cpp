//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgi/shaderProgram.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiShaderProgram::HgiShaderProgram(HgiShaderProgramDesc const& desc)
    : _descriptor(desc)
{
}

HgiShaderProgram::~HgiShaderProgram() = default;

HgiShaderProgramDesc const&
HgiShaderProgram::GetDescriptor() const
{
    return _descriptor;
}

HgiShaderProgramDesc::HgiShaderProgramDesc()
    : shaderFunctions(HgiShaderFunctionHandleVector())
{
}

bool operator==(
    const HgiShaderProgramDesc& lhs,
    const HgiShaderProgramDesc& rhs)
{
    return lhs.debugName == rhs.debugName &&
           lhs.shaderFunctions == rhs.shaderFunctions;
}

bool operator!=(
    const HgiShaderProgramDesc& lhs,
    const HgiShaderProgramDesc& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
