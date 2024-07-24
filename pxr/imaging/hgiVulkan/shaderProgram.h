//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIVULKAN_SHADERPROGRAM_H
#define PXR_IMAGING_HGIVULKAN_SHADERPROGRAM_H

#include <vector>

#include "pxr/imaging/hgi/shaderProgram.h"

#include "pxr/imaging/hgiVulkan/api.h"
#include "pxr/imaging/hgiVulkan/shaderFunction.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiVulkanDevice;


///
/// \class HgiVulkanShaderProgram
///
/// Vulkan implementation of HgiShaderProgram
///
class HgiVulkanShaderProgram final : public HgiShaderProgram
{
public:
    HGIVULKAN_API
    ~HgiVulkanShaderProgram() override = default;

    HGIVULKAN_API
    bool IsValid() const override;

    HGIVULKAN_API
    std::string const& GetCompileErrors() override;

    HGIVULKAN_API
    size_t GetByteSizeOfResource() const override;

    HGIVULKAN_API
    uint64_t GetRawResource() const override;

    /// Returns the shader functions that are part of this program.
    HGIVULKAN_API
    HgiShaderFunctionHandleVector const& GetShaderFunctions() const;

    /// Returns the device used to create this object.
    HGIVULKAN_API
    HgiVulkanDevice* GetDevice() const;

    /// Returns the (writable) inflight bits of when this object was trashed.
    HGIVULKAN_API
    uint64_t & GetInflightBits();

protected:
    friend class HgiVulkan;

    HGIVULKAN_API
    HgiVulkanShaderProgram(
        HgiVulkanDevice* device,
        HgiShaderProgramDesc const& desc);

private:
    HgiVulkanShaderProgram() = delete;
    HgiVulkanShaderProgram & operator=(const HgiVulkanShaderProgram&) = delete;
    HgiVulkanShaderProgram(const HgiVulkanShaderProgram&) = delete;

    HgiVulkanDevice* _device;
    uint64_t _inflightBits;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif