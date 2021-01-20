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
