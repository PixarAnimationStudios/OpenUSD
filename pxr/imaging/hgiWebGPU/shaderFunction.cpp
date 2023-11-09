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
#include "pxr/imaging/hgiWebGPU/shaderCompiler.h"
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"
#include "pxr/imaging/hgiWebGPU/shaderGenerator.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/base/tf/envSetting.h"

#include <sstream>

#if defined EMSCRIPTEN
#include <emscripten.h>
#endif

// tint include depends on this defines to populate the appropriate namespace
#define TINT_BUILD_SPV_READER 1
#define TINT_BUILD_WGSL_WRITER 1
#include <tint/tint.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIWEBGPU_ENABLE_WGSL, 0,
    "Enable WGSL shader code compilation");

void HgiWebGPUShaderFunction::_CreateBuffersBindingGroupLayoutEntries(
        std::vector<HgiShaderFunctionBufferDesc> const &buffers,
        std::vector<HgiShaderFunctionParamDesc> const &constants,
        wgpu::ShaderStage const &stage)
{
    BindGroupLayoutEntryMap bufferBindGroupEntries;

    if (constants.size() > 0) {
        wgpu::BindGroupLayoutEntry entry;
        // TODO: bindIndex create a static var or derive it from somewhere
        entry.binding = 0;
        entry.visibility = stage;
        entry.buffer.type = wgpu::BufferBindingType::Uniform;
        bufferBindGroupEntries.insert(std::make_pair(0,entry));
    }

    for (HgiShaderFunctionBufferDesc const &b : buffers)
    {
        wgpu::BindGroupLayoutEntry entry;
        wgpu::BufferBindingLayout bufferLayout;
        bufferLayout.type = HgiWebGPUConversions::GetBufferBindingType(b.binding, b.writable);

        if (stage & wgpu::ShaderStage::Vertex && b.writable && bufferLayout.type == wgpu::BufferBindingType::Storage) {
            // Even though webgpu supports read-write buffers for Fragment shaders, we need to unify the
            // shader code declaration between the two stages
            TF_WARN("No support for writable buffer named %s in vertex stage", b.nameInShader.c_str());
        }
        entry.binding = b.bindIndex;
        entry.buffer = bufferLayout;
        entry.visibility = stage;
        bufferBindGroupEntries.insert(std::make_pair(b.bindIndex, entry));
    }
    _bindGroups.insert(std::make_pair(HgiWebGPUBufferShaderSection::bindingSet, bufferBindGroupEntries));
}

void HgiWebGPUShaderFunction::_CreateTexturesGroupLayoutEntries(
        std::vector<HgiShaderFunctionTextureDesc> const &textures,
        wgpu::ShaderStage const &stage)
{
    BindGroupLayoutEntryMap texturesBindGroupEntries;
    BindGroupLayoutEntryMap samplersBindGroupEntries;
    for (size_t i=0; i<textures.size(); i++)
    {
        HgiShaderFunctionTextureDesc const &t = textures[i];

        wgpu::BindGroupLayoutEntry textureEntry;
        wgpu::BindGroupLayoutEntry samplerEntry;
        textureEntry.visibility = stage;
        if (t.writable) {
            // TODO: This is the only access storage for now
            textureEntry.storageTexture.access = wgpu::StorageTextureAccess::WriteOnly;
            textureEntry.storageTexture.viewDimension = HgiWebGPUConversions::GetTextureViewDimension(t.dimensions);
            textureEntry.storageTexture.format = HgiWebGPUConversions::GetPixelFormat(t.format);
        } else {
            textureEntry.texture.viewDimension = HgiWebGPUConversions::GetTextureViewDimension(t.dimensions);
            textureEntry.texture.sampleType = HgiWebGPUConversions::GetTextureSampleType(t.format);
        }
        samplerEntry.visibility = stage;
        textureEntry.binding = i;
        samplerEntry.binding = i;
        // TODO: How to derive this?
        samplerEntry.sampler.type = wgpu::SamplerBindingType::Filtering;
        texturesBindGroupEntries.insert(std::make_pair(i,textureEntry));
        samplersBindGroupEntries.insert(std::make_pair(i,samplerEntry));
    }
    _bindGroups.insert(std::make_pair(HgiWebGPUTextureShaderSection::bindingSet, texturesBindGroupEntries));
    _bindGroups.insert(std::make_pair(HgiWebGPUSamplerShaderSection::bindingSet, samplersBindGroupEntries));
}

HgiWebGPUShaderFunction::HgiWebGPUShaderFunction(
    HgiWebGPU *hgi,
    HgiShaderFunctionDesc const& desc)
    : HgiShaderFunction(desc)
    , _shaderModule(nullptr)
{
    HgiWebGPUShaderGenerator shaderGenerator(hgi, desc);

    shaderGenerator.Execute();
    const char *shaderCode = shaderGenerator.GetGeneratedShaderCode();

    wgpu::ShaderStage stage = HgiWebGPUConversions::GetShaderStages(desc.shaderStage);

    _CreateBuffersBindingGroupLayoutEntries(desc.buffers, desc.constantParams, stage);
    _CreateTexturesGroupLayoutEntries(desc.textures, stage);

    wgpu::ShaderModuleWGSLDescriptor wgslDesc;
    wgpu::ShaderModuleDescriptor shaderModuleDesc;
    wgslDesc.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
    shaderModuleDesc.label = _descriptor.debugName.c_str();
    shaderModuleDesc.nextInChain = &wgslDesc;
    std::string wgslCode;

    if (TfGetEnvSetting(HGIWEBGPU_ENABLE_WGSL)) {
        wgslDesc.code = desc.shaderCode;
    } else {
        const char* debugLbl = _descriptor.debugName.empty() ?
            "unknown" : _descriptor.debugName.c_str();
        // Compile shader and capture errors
        std::vector<unsigned int> spirvData;
        bool result = HgiWebGPUCompileGLSL(
                debugLbl,
                &shaderCode,
                1,
                desc.shaderStage,
                &spirvData,
                &_errors);

        if (result) {
            //// SPIR-V
            tint::spirv::reader::Options readerOptions{};
            readerOptions.allow_non_uniform_derivatives = true;
            tint::Program program = tint::spirv::reader::Read(spirvData, readerOptions);
            if (!program.IsValid()) {
                TF_CODING_ERROR("Tint SPIR-V reader failure:\nParser: " + program.Diagnostics().str() + "\n");
                return;
            };

            tint::wgsl::writer::Options options{};
            auto tintResult = tint::wgsl::writer::Generate(program, options);
            if (tintResult) {
                wgslCode = tintResult->wgsl;
            } else {
                _errors = tintResult.Failure().reason.str();
            }

            wgslDesc.code = wgslCode.c_str();
        }
    }

    if (_errors.empty()) {
        _shaderModule = hgi->GetPrimaryDevice().CreateShaderModule(&shaderModuleDesc);
        // Get the compilation details
#if defined EMSCRIPTEN
        // Getting unimplemented in Chrome
    if( !_shaderModule )
    {
        printf("Failed to create shader module\n");
        _errors = "Failed.";
    }
#else
        _shaderModule.GetCompilationInfo(
                [](WGPUCompilationInfoRequestStatus status, WGPUCompilationInfo const *compilationInfo, void *userdata) {
                    if (status != WGPUCompilationInfoRequestStatus_Success) {
                        std::stringstream errorss;
                        for (uint32_t i = 0; i < compilationInfo->messageCount; ++i) {
                            auto &compilationMessage = compilationInfo->messages[i];
                            errorss << compilationMessage.lineNum << ": " << compilationMessage.message << std::endl;
                        }
                        auto *errors = static_cast<std::string *>(userdata);
                        *errors = errorss.str();
                    }
                }, &_errors);
#endif
    }
    
    // Clear these pointers in our copy of the descriptor since we
    // have to assume they could become invalid after we return.
    _descriptor.shaderCodeDeclarations = nullptr;
    _descriptor.shaderCode = nullptr;
    _descriptor.generatedShaderCodeOut = nullptr;
}

HgiWebGPUShaderFunction::~HgiWebGPUShaderFunction()
{
    if( _shaderModule )
        _shaderModule = nullptr;
}

bool HgiWebGPUShaderFunction::IsValid() const
{
    return _errors.empty();
}

std::string const& HgiWebGPUShaderFunction::GetCompileErrors()
{
    return _errors;
}

size_t HgiWebGPUShaderFunction::GetByteSizeOfResource() const
{
    // TODO: I'm not really sure what this should be, in Vulkan this is the SPIRV code size
    // which doesn't seem like a particularly useful thing and I don't think there is a WGSL
    // equivalent of compiled code size
    return 1u;
}

uint64_t HgiWebGPUShaderFunction::GetRawResource() const
{
    return reinterpret_cast<uint64_t>(_shaderModule.Get());
}

const BindGroupsLayoutMap &HgiWebGPUShaderFunction::GetBindGroups() const {
    return _bindGroups;
}


const char *HgiWebGPUShaderFunction::GetShaderEntryPoint() const
{
    // TODO: I hope your shaders use 'main' as the entrypoint ;)
    // Use reflection to get this
    return "main";
}

wgpu::ShaderModule HgiWebGPUShaderFunction::GetShaderModule() const
{
    return _shaderModule;
}

PXR_NAMESPACE_CLOSE_SCOPE
