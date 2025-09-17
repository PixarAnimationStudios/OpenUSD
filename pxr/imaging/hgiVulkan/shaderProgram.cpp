//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiVulkan/shaderProgram.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiVulkanShaderProgram::HgiVulkanShaderProgram(
    HgiVulkanDevice* device,
    HgiShaderProgramDesc const& desc)
    : HgiShaderProgram(desc)
    , _device(device)
    , _inflightBits(0)
{
}

bool
HgiVulkanShaderProgram::IsValid() const
{
    return true;
}

std::string const&
HgiVulkanShaderProgram::GetCompileErrors()
{
    static const std::string empty;
    return empty;
}

size_t
HgiVulkanShaderProgram::GetByteSizeOfResource() const
{
    size_t  byteSize = 0;
    for (HgiShaderFunctionHandle const& fn : _descriptor.shaderFunctions) {
        byteSize += fn->GetByteSizeOfResource();
    }
    return byteSize;
}

uint64_t
HgiVulkanShaderProgram::GetRawResource() const
{
    return 0; // No vulkan resource for programs
}

HgiShaderFunctionHandleVector const&
HgiVulkanShaderProgram::GetShaderFunctions() const
{
    return _descriptor.shaderFunctions;
}

HgiVulkanDevice*
HgiVulkanShaderProgram::GetDevice() const
{
    return _device;
}

uint64_t &
HgiVulkanShaderProgram::GetInflightBits()
{
    return _inflightBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
