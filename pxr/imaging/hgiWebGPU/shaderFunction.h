//
// Copyright 2022 Pixar
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
#ifndef PXR_IMAGING_HGIWEBGPU_SHADERFUNCTION_H
#define PXR_IMAGING_HGIWEBGPU_SHADERFUNCTION_H

#include "pxr/imaging/hgi/shaderFunction.h"
#include "pxr/imaging/hgiWebGPU/api.h"

#include <map>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

    class HgiWebGPU;

    using BindGroupLayoutEntryMap = std::unordered_map<uint32_t, wgpu::BindGroupLayoutEntry>;
    using BindGroupsLayoutMap = std::map<uint32_t, BindGroupLayoutEntryMap>;

///
/// \class HgiWebGPUShaderFunction
///
/// WebGPU implementation of HgiShaderFunction
///
    class HgiWebGPUShaderFunction final : public HgiShaderFunction {
    public:
        HGIWEBGPU_API
        ~HgiWebGPUShaderFunction() override;

        HGIWEBGPU_API
        bool IsValid() const override;

        /// Returns shader compile errors.
        HGIWEBGPU_API
        std::string const &GetCompileErrors() override;

        HGIWEBGPU_API
        size_t GetByteSizeOfResource() const override;

        HGIWEBGPU_API
        uint64_t GetRawResource() const override;

        HGIWEBGPU_API
        const BindGroupsLayoutMap &GetBindGroups() const;

        HGIWEBGPU_API
        const char *GetShaderEntryPoint() const;

        HGIWEBGPU_API
        wgpu::ShaderModule GetShaderModule() const;

    protected:
        friend class HgiWebGPU;

        HGIWEBGPU_API
        HgiWebGPUShaderFunction(
                HgiWebGPU *hgi,
                HgiShaderFunctionDesc const &desc);

        wgpu::ShaderModule _shaderModule;
        std::string _errors;


    private:
        BindGroupsLayoutMap _bindGroups;

        HgiWebGPUShaderFunction() = delete;

        HgiWebGPUShaderFunction &operator=(const HgiWebGPUShaderFunction &) = delete;

        HgiWebGPUShaderFunction(const HgiWebGPUShaderFunction &) = delete;

        void _CreateBuffersBindingGroupLayoutEntries(std::vector<HgiShaderFunctionBufferDesc> const &buffers,
                                                     std::vector<HgiShaderFunctionParamDesc> const &constants,
                                                     wgpu::ShaderStage const &stage);

        void _CreateTexturesGroupLayoutEntries(std::vector<HgiShaderFunctionTextureDesc> const &textures,
                                               wgpu::ShaderStage const &stage);
    };

PXR_NAMESPACE_CLOSE_SCOPE

#endif
