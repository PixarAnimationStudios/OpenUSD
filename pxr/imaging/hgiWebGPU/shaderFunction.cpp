//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/debugCodes.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/shaderCompiler.h"
#include "pxr/imaging/hgiWebGPU/shaderGenerator.h"

#include <spirv-tools/libspirv.hpp>
#include <spirv/unified1/spirv.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

// tint include depends on this defines to populate the appropriate namespace
#define TINT_BUILD_SPV_READER 1
#define TINT_BUILD_WGSL_WRITER 1
#include <tint/tint.h>
// This include shouldn't be necessary, but it's missing in tint.h
#include <src/tint/lang/core/ir/module.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIWEBGPU_SHADER_CACHE_DIR, "",
    "Define a directory to cache shaders transpiled from GLSL to WGSL."
    "If not set, the shaders will be compiled at runtime."
    "In the case of emscripten, the application is responsible for"
    "mounting the directory to the virtual filesystem."
    "A use case that persists sessions is to use the indexedDB."
    "For example, in the preload.js script:"
    "FS.mkdir(\"/<HGIWEBGPU_SHADER_CACHE_DIR>\");\n"
    "    FS.mount(IDBFS, {autoPersist: true}, "
    "\"/<HGIWEBGPU_SHADER_CACHE_DIR>\");\n"
    "    return FS.syncfs(true, function (err) {\n"
    "        if (err) {\n"
    "            console.error(\"Error syncing filesystem:\", err);\n"
    "        } else {\n"
    "            console.log(\"Filesystem synced.\");\n"
    "        }\n"
    "    });"
    "Notice that the directory must match the one defined in the env setting."
    "In case of using this method, you also need to pass the -lidbfs.js linked "
    "flag to the application.");

TF_DEFINE_ENV_SETTING(HGIWEBGPU_SHADER_STRICT_MATH, false,
    "Force strict math in shaders, as opposed to fast math. This can be used "
    "to diagnose issues related to dependency on non-finite floating point "
    "values, such as NaN and infinity.");

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        HGIWEBGPU_DUMP_SHADER_SPIRV_FILE, "Dump shader SPIR-V");
}

static bool
_IsShaderStrictMath()
{
    static const bool strictMath =
        TfGetEnvSetting(HGIWEBGPU_SHADER_STRICT_MATH);
    return strictMath;
}

static std::vector<uint32_t>
GlslToSpirv(const char* shaderCode, HgiShaderStage stage, std::string* errors,
    const char* debugLbl)
{
    std::vector<uint32_t> spirvCode;
    if (!HgiWebGPUCompileGLSL(
            debugLbl, &shaderCode, 1, stage, &spirvCode, errors)) {
        return {};
    }

    if (TfDebug::IsEnabled(HGIWEBGPU_DUMP_SHADER_SPIRV_FILE)) {
        static size_t fileCounter = 0;
        const auto filename = std::string{debugLbl} + "_" +
            std::to_string(fileCounter++) + ".spv";
        if (std::ofstream spvFile{filename, std::ios::binary}) {
            spvFile.write(reinterpret_cast<const char*>(spirvCode.data()),
                spirvCode.size() * sizeof(uint32_t));
            TF_STATUS("Wrote SPIR-V to: %s\n", filename.c_str());
        } else {
            TF_RUNTIME_ERROR(
                "Couldn't open file to write SPIR-V: %s\n", filename.c_str());
        }
    }

    return spirvCode;
}

static std::string
SpirvToWgsl(const std::vector<uint32_t>& spirvCode, std::string* errors,
    const char* debugLbl)
{
    std::string wgslCode;
    tint::spirv::reader::Options readerOptions{};
    tint::Result<tint::core::ir::Module> irResult =
        tint::spirv::reader::ReadIR(spirvCode, readerOptions);
    if (irResult != tint::Success) {
        TF_CODING_ERROR("Tint SPIR-V reader failure:\nParser: " +
            irResult.Failure().reason + "\n");
        return wgslCode;
    }

    tint::wgsl::writer::Options writerOptions;
    // Add a directive that suppresses Chromium's unreachable-code warnings.
    // The GLSL-to-SPIR-V-to-WGSL translation pipeline (via Tint) can
    // produce control-flow patterns that Chromium's WGSL compiler flags as
    // unreachable, even though the original GLSL is valid.
    writerOptions.disable_unreachable_code_warning = true;
    writerOptions.allow_non_uniform_derivatives = true;
    const auto extensions = {tint::wgsl::Extension::kPrimitiveIndex,
        tint::wgsl::Extension::kClipDistances};
    writerOptions.allowed_features.extensions.insert(
        extensions.begin(), extensions.end());
    writerOptions.allowed_features.features.emplace(
        tint::wgsl::LanguageFeature::kReadonlyAndReadwriteStorageTextures);
    auto wgslResult =
        tint::wgsl::writer::ProgramFromIR(irResult.Get(), writerOptions);
    if (wgslResult == tint::Success) {
        tint::Program program = wgslResult.Move();
        if (program.IsValid()) {
            auto tintResult = tint::wgsl::writer::Generate(program);
            if (tintResult == tint::Success) {
                wgslCode = std::move(tintResult->wgsl);
            } else {
                *errors = tintResult.Failure().reason;
            }
        } else {
            TF_CODING_ERROR("Tint WGSL writer failure:\nParser: " +
                program.Diagnostics().Str() + "\n");
        }
    } else {
        *errors = wgslResult.Failure().reason;
    }

    return wgslCode;
}

static wgpu::ShaderModule
_CompileShaderModule(
    wgpu::Device const& device,
    const std::vector<uint32_t>& spirv,
    const char* debugName,
    std::string* errors)
{
    const std::filesystem::path cacheDir =
        TfGetEnvSetting(HGIWEBGPU_SHADER_CACHE_DIR);

    std::string wgslCode;
    if (cacheDir.empty()) {
        wgslCode = SpirvToWgsl(spirv, errors, debugName);
    } else {
        const size_t wgslHash = std::hash<std::string_view>{}(
            {reinterpret_cast<const char*>(spirv.data()),
             spirv.size() * sizeof(uint32_t)});
        const std::filesystem::path wgslCachePath =
            cacheDir / (std::to_string(wgslHash) + "_" + debugName + ".wgsl");
        try {
            if (std::filesystem::exists(wgslCachePath)) {
                std::ifstream wgslFile(wgslCachePath);
                if (wgslFile.is_open()) {
                    std::stringstream buffer;
                    buffer << wgslFile.rdbuf();
                    wgslCode = buffer.str();
                } else {
                    throw std::runtime_error("Failed to open WGSL cache file.");
                }
            } else {
                wgslCode = SpirvToWgsl(spirv, errors, debugName);
                std::ofstream wgslFile(wgslCachePath);
                if (wgslFile.is_open()) {
                    wgslFile << wgslCode;
                    wgslFile.close();
                } else {
                    throw std::runtime_error("Failed to create WGSL cache file.");
                }
            }
        } catch (const std::exception& e) {
            wgslCode = SpirvToWgsl(spirv, errors, debugName);
            TF_RUNTIME_ERROR("%s", e.what());
        }
    }

    if (TfDebug::IsEnabled(HGIWEBGPU_DUMP_SHADER_SOURCEFILE)) {
        static size_t globalDebugID = 0;
        static size_t debugShaderID = 0;
        std::stringstream fnameStream;
        fnameStream << "program" << globalDebugID++ << "_shader"
                    << debugShaderID++ << ".wgsl";
        const std::string fname = fnameStream.str();
        std::fstream output(fname.c_str(), std::ios::out);
        output << wgslCode;
        output.close();
        std::cout << "Write " << fname << " (size=" << wgslCode.size() << ")\n";
    }

    if (!errors->empty()) {
        return nullptr;
    }

    wgpu::ShaderSourceWGSL wgslDesc;
    wgslDesc.sType = wgpu::SType::ShaderSourceWGSL;
    wgslDesc.code = wgslCode.c_str();
#if !defined(ARCH_OS_WASM_VM)
    wgpu::ShaderModuleCompilationOptions compilationOptions;
    compilationOptions.strictMath = _IsShaderStrictMath();
    wgslDesc.nextInChain = &compilationOptions;
#endif
    wgpu::ShaderModuleDescriptor moduleDesc;
    moduleDesc.label = debugName;
    moduleDesc.nextInChain = &wgslDesc;
    wgpu::ShaderModule shaderModule = device.CreateShaderModule(&moduleDesc);

#if defined ARCH_OS_WASM_VM
    if (!shaderModule) {
        printf("Failed to create shader module\n");
        *errors = "Failed.";
    }
#else
    if (shaderModule) {
        shaderModule.GetCompilationInfo(wgpu::CallbackMode::AllowSpontaneous,
            [errors](wgpu::CompilationInfoRequestStatus status,
                const wgpu::CompilationInfo* compilationInfo) {
                if (status != wgpu::CompilationInfoRequestStatus::Success) {
                    std::stringstream errorss;
                    for (uint32_t i = 0; i < compilationInfo->messageCount;
                            ++i) {
                        auto& msg = compilationInfo->messages[i];
                        errorss << msg.lineNum << ": "
                                << msg.message.data << std::endl;
                    }
                    *errors = errorss.str();
                }
            });
    }
#endif
    return shaderModule;
}

void
HgiWebGPUShaderFunction::_CreateBuffersBindingGroupLayoutEntries(
    std::vector<HgiShaderFunctionBufferDesc> const& buffers,
    std::vector<HgiShaderFunctionParamDesc> const& constants,
    wgpu::ShaderStage const& stage)
{
    BindGroupLayoutEntryMap bufferBindGroupEntries;
    BindGroupLayoutEntryMap constantBindGroupEntries;

    if (constants.size() > 0) {
        wgpu::BindGroupLayoutEntry entry;
        // TODO: bindIndex create a static var or derive it from somewhere
        entry.binding = 0;
        entry.visibility = stage;
        entry.buffer.type = wgpu::BufferBindingType::Uniform;
        constantBindGroupEntries.insert(std::make_pair(0, entry));
    }
    _bindGroups.insert(
        std::make_pair(HgiWebGPUBufferShaderSection::constantsBindingSet,
            constantBindGroupEntries));

    for (HgiShaderFunctionBufferDesc const& b : buffers) {
        if (!_activeBindings.count(b.bindIndex)) {
            continue;
        }
        wgpu::BindGroupLayoutEntry entry;
        wgpu::BufferBindingLayout bufferLayout;
        bufferLayout.type =
            HgiWebGPUConversions::GetBufferBindingType(b.binding, b.writable);

        if (stage & wgpu::ShaderStage::Vertex && b.writable &&
            bufferLayout.type == wgpu::BufferBindingType::Storage) {
            // Even though webgpu supports read-write buffers for Fragment
            // shaders, we need to unify the shader code declaration between the
            // two stages
            TF_WARN("No support for writable buffer named %s in vertex stage",
                b.nameInShader.c_str());
        }
        entry.binding = b.bindIndex;
        entry.buffer = bufferLayout;
        entry.visibility = stage;
        bufferBindGroupEntries.insert(std::make_pair(b.bindIndex, entry));
    }
    _bindGroups.insert(std::make_pair(
        HgiWebGPUBufferShaderSection::bindingSet, bufferBindGroupEntries));
}

void
HgiWebGPUShaderFunction::_CreateTexturesGroupLayoutEntries(
    std::vector<HgiShaderFunctionTextureDesc> const& textures,
    wgpu::ShaderStage const& stage)
{
    BindGroupLayoutEntryMap texturesBindGroupEntries;
    BindGroupLayoutEntryMap samplersBindGroupEntries;
    size_t bindingIdx = 0;
    for (size_t i = 0; i < textures.size(); i++) {
        HgiShaderFunctionTextureDesc const& t = textures[i];
        const size_t count = std::max(size_t(1), t.arraySize);
        const bool isShadow =
            t.textureType == HgiShaderTextureTypeShadowTexture;

        for (size_t j = 0; j < count; j++) {
            wgpu::BindGroupLayoutEntry textureEntry;
            wgpu::BindGroupLayoutEntry samplerEntry;
            textureEntry.visibility = stage;
            if (t.writable) {
                textureEntry.storageTexture.access =
                    wgpu::StorageTextureAccess::ReadWrite;
                if (t.textureType == HgiShaderTextureTypeCubemapTexture) {
                    textureEntry.storageTexture.viewDimension =
                        wgpu::TextureViewDimension::e2DArray;
                } else {
                    textureEntry.storageTexture.viewDimension =
                        HgiWebGPUConversions::GetTextureViewDimension(
                            t.dimensions, t.textureType);
                }
                textureEntry.storageTexture.format =
                    HgiWebGPUConversions::GetPixelFormat(t.format);
            } else {
                textureEntry.texture.viewDimension =
                    HgiWebGPUConversions::GetTextureViewDimension(
                        t.dimensions, t.textureType);
                if (isShadow) {
                    textureEntry.texture.sampleType =
                        wgpu::TextureSampleType::Depth;
                } else if (t.textureType == HgiShaderTextureTypeDepth) {
                    textureEntry.texture.sampleType =
                        wgpu::TextureSampleType::UnfilterableFloat;
                } else {
                    textureEntry.texture.sampleType =
                        HgiWebGPUConversions::GetTextureSampleType(t.format);
                }
            }
            samplerEntry.visibility = stage;
            textureEntry.binding = bindingIdx + j;
            samplerEntry.binding = bindingIdx + j;
            samplerEntry.sampler.type = isShadow ?
                wgpu::SamplerBindingType::Comparison :
                wgpu::SamplerBindingType::Filtering;
            texturesBindGroupEntries.insert(
                std::make_pair(bindingIdx + j, textureEntry));
            samplersBindGroupEntries.insert(
                std::make_pair(bindingIdx + j, samplerEntry));
        }
        bindingIdx += count;
    }
    _bindGroups.insert(std::make_pair(
        HgiWebGPUTextureShaderSection::bindingSet, texturesBindGroupEntries));
    _bindGroups.insert(std::make_pair(
        HgiWebGPUSamplerShaderSection::bindingSet, samplersBindGroupEntries));
}

static std::unordered_set<uint32_t>
_ReflectActiveBindings(const std::vector<uint32_t>& spirv)
{
    std::unordered_map<uint32_t, uint32_t> idToBinding;
    std::unordered_map<uint32_t, uint32_t> idToSet;

    spvtools::SpirvTools tools(SPV_ENV_VULKAN_1_0);
    tools.Parse(
        spirv,
        [](const spv_endianness_t, const spv_parsed_header_t&) {
            return SPV_SUCCESS;
        },
        [&](const spv_parsed_instruction_t& inst) -> spv_result_t {
            if (inst.opcode == SpvOpDecorate && inst.num_operands >= 3) {
                const uint32_t id = inst.words[inst.operands[0].offset];
                const uint32_t decoration = inst.words[inst.operands[1].offset];
                const uint32_t value = inst.words[inst.operands[2].offset];
                if (decoration == SpvDecorationBinding) {
                    idToBinding[id] = value;
                } else if (decoration == SpvDecorationDescriptorSet) {
                    idToSet[id] = value;
                }
            }
            return SPV_SUCCESS;
        });

    std::unordered_set<uint32_t> result;
    for (const auto& [id, binding] : idToBinding) {
        if (const auto it = idToSet.find(id); it != idToSet.end()) {
            if (it->second == HgiWebGPUBufferShaderSection::bindingSet) {
                result.insert(binding);
            }
        }
    }
    return result;
}

HgiWebGPUShaderFunction::HgiWebGPUShaderFunction(
    HgiWebGPU* hgi, HgiShaderFunctionDesc const& desc)
    : HgiShaderFunction(desc)
    , _shaderModule(nullptr)
{
    HgiWebGPUShaderGenerator shaderGenerator(hgi, desc);

    shaderGenerator.Execute();
    const char* shaderCode = shaderGenerator.GetGeneratedShaderCode();

    wgpu::ShaderStage stage =
        HgiWebGPUConversions::GetShaderStages(desc.shaderStage);

    const char* debugLbl = _descriptor.debugName.empty() ?
        "unknown" :
        _descriptor.debugName.c_str();

    const std::vector<uint32_t> spirvCode =
        GlslToSpirv(shaderCode, desc.shaderStage, &_errors, debugLbl);
    if (spirvCode.empty()) {
        return;
    }

    _activeBindings = _ReflectActiveBindings(spirvCode);
    _spirv = spirvCode;

    _CreateBuffersBindingGroupLayoutEntries(
        desc.buffers, desc.constantParams, stage);
    _CreateTexturesGroupLayoutEntries(desc.textures, stage);

    _shaderModule = _CompileShaderModule(
        hgi->GetPrimaryDevice(), spirvCode, debugLbl, &_errors);

    // Clear these pointers in our copy of the descriptor since we
    // have to assume they could become invalid after we return.
    _descriptor.shaderCodeDeclarations = nullptr;
    _descriptor.shaderCode = nullptr;
    _descriptor.generatedShaderCodeOut = nullptr;
}

HgiWebGPUShaderFunction::~HgiWebGPUShaderFunction()
{
    if (_shaderModule)
        _shaderModule = nullptr;
}

bool
HgiWebGPUShaderFunction::IsValid() const
{
    return _errors.empty();
}

std::string const&
HgiWebGPUShaderFunction::GetCompileErrors()
{
    return _errors;
}

size_t
HgiWebGPUShaderFunction::GetByteSizeOfResource() const
{
    // TODO: I'm not really sure what this should be, in Vulkan this is the
    // SPIRV code size which doesn't seem like a particularly useful thing and I
    // don't think there is a WGSL equivalent of compiled code size
    return 1u;
}

uint64_t
HgiWebGPUShaderFunction::GetRawResource() const
{
    return reinterpret_cast<uint64_t>(_shaderModule.Get());
}

const BindGroupsLayoutMap&
HgiWebGPUShaderFunction::GetBindGroups() const
{
    return _bindGroups;
}


const char*
HgiWebGPUShaderFunction::GetShaderEntryPoint() const
{
    // TODO: I hope your shaders use 'main' as the entrypoint ;)
    // Use reflection to get this
    return "main";
}

wgpu::ShaderModule
HgiWebGPUShaderFunction::GetShaderModule() const
{
    return _shaderModule;
}

const std::vector<uint32_t>&
HgiWebGPUShaderFunction::GetSpirvBinary() const
{
    return _spirv;
}

wgpu::ShaderModule
HgiWebGPUShaderFunction::CreateShaderModuleFromSpirv(
    wgpu::Device const& device,
    const std::vector<uint32_t>& spirv,
    const std::string& debugName)
{
    std::string errors;
    wgpu::ShaderModule module =
        _CompileShaderModule(device, spirv, debugName.c_str(), &errors);
    if (!errors.empty()) {
        TF_CODING_ERROR("Failed to compile modified SPIR-V shader module: %s",
            errors.c_str());
    }
    return module;
}

PXR_NAMESPACE_CLOSE_SCOPE
