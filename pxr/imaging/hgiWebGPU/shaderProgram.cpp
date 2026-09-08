//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiWebGPU/shaderProgram.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiWebGPUShaderProgram::HgiWebGPUShaderProgram(HgiShaderProgramDesc const& desc)
    : HgiShaderProgram(desc)
{
}

bool
HgiWebGPUShaderProgram::IsValid() const
{
    return true;
}

std::string const&
HgiWebGPUShaderProgram::GetCompileErrors()
{
    static const std::string empty;
    return empty;
}

size_t
HgiWebGPUShaderProgram::GetByteSizeOfResource() const
{
    size_t  byteSize = 0;
    for (HgiShaderFunctionHandle const& fn : _descriptor.shaderFunctions) {
        byteSize += fn->GetByteSizeOfResource();
    }
    return byteSize;
}

uint64_t
HgiWebGPUShaderProgram::GetRawResource() const
{
    return 0;
}

HgiShaderFunctionHandleVector const&
HgiWebGPUShaderProgram::GetShaderFunctions() const
{
    return _descriptor.shaderFunctions;
}

PXR_NAMESPACE_CLOSE_SCOPE
