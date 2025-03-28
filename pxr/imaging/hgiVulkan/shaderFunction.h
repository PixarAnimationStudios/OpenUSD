//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_SHADERFUNCTION_H
#define PXR_IMAGING_HGIVULKAN_SHADERFUNCTION_H

#include "pxr/imaging/hgi/shaderFunction.h"
#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/shaderCompiler.h"
#include "pxr/imaging/hgiVulkan/vulkan.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HgiVulkan;
class HgiVulkanDevice;


///
/// \class HgiVulkanShaderFunction
///
/// Vulkan implementation of HgiShaderFunction
///
class HgiVulkanShaderFunction final : public HgiShaderFunction
{
public:
    HGIVULKAN_API
    ~HgiVulkanShaderFunction() override;

    HGIVULKAN_API
    bool IsValid() const override;

    HGIVULKAN_API
    std::string const& GetCompileErrors() override;

    HGIVULKAN_API
    size_t GetByteSizeOfResource() const override;

    HGIVULKAN_API
    uint64_t GetRawResource() const override;

    /// Returns the shader stage this function operates in.
    HGIVULKAN_API
    VkShaderStageFlagBits GetShaderStage() const;

    /// Returns the binary shader module of the shader function.
    HGIVULKAN_API
    VkShaderModule GetShaderModule() const;

    /// Returns the shader entry function name (usually "main").
    HGIVULKAN_API
    const char* GetShaderFunctionName() const;

    /// Returns the descriptor set layout information that describe the
    /// resource bindings for this module. The returned info would usually be
    /// merged with info of other shader modules to create a VkPipelineLayout.
    HGIVULKAN_API
    HgiVulkanDescriptorSetInfoVector const& GetDescriptorSetInfo() const;

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanShaderFunction(
        HgiVulkanDevice* device,
        Hgi const* hgi,
        HgiShaderFunctionDesc const& desc,
        int shaderVersion);

private:
    HgiVulkanShaderFunction() = delete;
    HgiVulkanShaderFunction& operator=(const HgiVulkanShaderFunction&) = delete;
    HgiVulkanShaderFunction(const HgiVulkanShaderFunction&) = delete;

    HgiVulkanDevice* _device;
    std::string _errors;
    size_t _spirvByteSize;
    VkShaderModule _vkShaderModule;
    HgiVulkanDescriptorSetInfoVector _descriptorSetInfo;
    uint64_t _inflightBits;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
