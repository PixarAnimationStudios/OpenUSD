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
#include "pxr/imaging/hgiVulkan/conversions.h"
#include "pxr/imaging/hgiVulkan/device.h"
#include "pxr/imaging/hgiVulkan/diagnostic.h"
#include "pxr/imaging/hgiVulkan/garbageCollector.h"
#include "pxr/imaging/hgiVulkan/hgi.h"
#include "pxr/imaging/hgiVulkan/shaderCompiler.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"
#include "pxr/imaging/hgiVulkan/shaderGenerator.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiVulkanShaderFunction::HgiVulkanShaderFunction(
    HgiVulkanDevice* device,
    HgiShaderFunctionDesc const& desc)
    : HgiShaderFunction(desc)
    , _device(device)
    , _spirvByteSize(0)
    , _vkShaderModule(nullptr)
    , _inflightBits(0)
{
    VkShaderModuleCreateInfo shaderCreateInfo =
        {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};

    std::vector<unsigned int> spirv;

    const char* debugLbl = _descriptor.debugName.empty() ?
        "unknown" : _descriptor.debugName.c_str();

    HgiVulkanShaderGenerator shaderGenerator {desc};
    std::stringstream ss;
    shaderGenerator.Execute(ss);
    std::string shaderStr = ss.str();
    const char* shaderCode = shaderStr.c_str();

    // Compile shader and capture errors
    bool result = HgiVulkanCompileGLSL(
        debugLbl,
        &shaderCode,
        1,
        desc.shaderStage,
        &spirv,
        &_errors);

    // Create vulkan module if there were no errors.
    if (result) {
        _spirvByteSize = spirv.size() * sizeof(unsigned int);

        shaderCreateInfo.codeSize = _spirvByteSize;
        shaderCreateInfo.pCode = (uint32_t*) spirv.data();

        TF_VERIFY(
            vkCreateShaderModule(
                device->GetVulkanDevice(),
                &shaderCreateInfo,
                HgiVulkanAllocator(),
                &_vkShaderModule) == VK_SUCCESS
        );

        // Debug label
        if (!_descriptor.debugName.empty()) {
            std::string debugLabel = "ShaderModule " + _descriptor.debugName;
            HgiVulkanSetDebugName(
                device,
                (uint64_t)_vkShaderModule,
                VK_OBJECT_TYPE_SHADER_MODULE,
                debugLabel.c_str());
        }

        // Perform reflection on spirv to create descriptor set info for
        // this module. This will be needed during pipeline creation when
        // we know the shader modules, but not the resource bindings.
        // Hgi does not require resource bindings information to be provided
        // for its HgiPipeline descriptor, but does provide the shader program.
        // We mimic Metal where the resource binding info is inferred from the
        // Metal shader program.
        _descriptorSetInfo = HgiVulkanGatherDescriptorSetInfo(spirv);
    }

    _descriptor.shaderCode = nullptr;
}

HgiVulkanShaderFunction::~HgiVulkanShaderFunction()
{
    if (_vkShaderModule) {
        vkDestroyShaderModule(
            _device->GetVulkanDevice(),
            _vkShaderModule,
            HgiVulkanAllocator());
    }
}

VkShaderStageFlagBits
HgiVulkanShaderFunction::GetShaderStage() const
{
    return VkShaderStageFlagBits(
        HgiVulkanConversions::GetShaderStages(_descriptor.shaderStage));
}

VkShaderModule
HgiVulkanShaderFunction::GetShaderModule() const
{
    return _vkShaderModule;
}

const char*
HgiVulkanShaderFunction::GetShaderFunctionName() const
{
    static const std::string entry("main");
    return entry.c_str();
}

bool
HgiVulkanShaderFunction::IsValid() const
{
    return _errors.empty();
}

std::string const&
HgiVulkanShaderFunction::GetCompileErrors()
{
    return _errors;
}

size_t
HgiVulkanShaderFunction::GetByteSizeOfResource() const
{
    return _spirvByteSize;
}

uint64_t
HgiVulkanShaderFunction::GetRawResource() const
{
    return (uint64_t) _vkShaderModule;
}

HgiVulkanDescriptorSetInfoVector const&
HgiVulkanShaderFunction::GetDescriptorSetInfo() const
{
    return _descriptorSetInfo;
}

HgiVulkanDevice*
HgiVulkanShaderFunction::GetDevice() const
{
    return _device;
}

uint64_t &
HgiVulkanShaderFunction::GetInflightBits()
{
    return _inflightBits;
}

PXR_NAMESPACE_CLOSE_SCOPE
