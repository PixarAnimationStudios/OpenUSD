//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIWEBGPU_SHADERFUNCTION_H
#define PXR_IMAGING_HGIWEBGPU_SHADERFUNCTION_H

#include "pxr/base/tf/debugCodes.h"
#include "pxr/imaging/hgi/shaderFunction.h"
#include "pxr/imaging/hgiWebGPU/api.h"

#include <map>
#include <unordered_map>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(
    HGIWEBGPU_DUMP_SHADER_SPIRV_FILE
);

class HgiWebGPU;

using BindGroupLayoutEntryMap =
    std::unordered_map<uint32_t, wgpu::BindGroupLayoutEntry>;
using BindGroupsLayoutMap = std::map<uint32_t, BindGroupLayoutEntryMap>;

///
/// \class HgiWebGPUShaderFunction
///
/// WebGPU implementation of HgiShaderFunction
///
class ARCH_EXPORT_TYPE HgiWebGPUShaderFunction final : public HgiShaderFunction
{
public:
    HGIWEBGPU_API
    ~HgiWebGPUShaderFunction() override;

    HGIWEBGPU_API
    bool IsValid() const override;

    /// Returns shader compile errors.
    HGIWEBGPU_API
    std::string const& GetCompileErrors() override;

    HGIWEBGPU_API
    size_t GetByteSizeOfResource() const override;

    HGIWEBGPU_API
    uint64_t GetRawResource() const override;

    HGIWEBGPU_API
    const BindGroupsLayoutMap& GetBindGroups() const;

    HGIWEBGPU_API
    const char* GetShaderEntryPoint() const;

    HGIWEBGPU_API
    wgpu::ShaderModule GetShaderModule() const;

    HGIWEBGPU_API
    const std::vector<uint32_t>& GetSpirvBinary() const;

    HGIWEBGPU_API
    static wgpu::ShaderModule CreateShaderModuleFromSpirv(
        wgpu::Device const& device,
        const std::vector<uint32_t>& spirv,
        const std::string& debugName);

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUShaderFunction(HgiWebGPU* hgi, HgiShaderFunctionDesc const& desc);

    wgpu::ShaderModule _shaderModule;
    std::string _errors;


private:
    BindGroupsLayoutMap _bindGroups;
    std::unordered_set<uint32_t> _activeBindings;
    std::vector<uint32_t> _spirv;

    HgiWebGPUShaderFunction() = delete;

    HgiWebGPUShaderFunction& operator=(const HgiWebGPUShaderFunction&) = delete;

    HgiWebGPUShaderFunction(const HgiWebGPUShaderFunction&) = delete;

    void _CreateBuffersBindingGroupLayoutEntries(
        std::vector<HgiShaderFunctionBufferDesc> const& buffers,
        std::vector<HgiShaderFunctionParamDesc> const& constants,
        wgpu::ShaderStage const& stage);

    void _CreateTexturesGroupLayoutEntries(
        std::vector<HgiShaderFunctionTextureDesc> const& textures,
        wgpu::ShaderStage const& stage);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
