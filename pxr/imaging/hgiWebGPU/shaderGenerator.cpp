//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiWebGPU/shaderGenerator.h"
#include "pxr/imaging/hgi/shaderSection.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgiWebGPU/shaderSection.h"

#include <sstream>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

static const char*
_GetPackedTypeDefinitions()
{
    return "\n"
           "struct hgi_ivec3 { int    x, y, z; };\n"
           "struct hgi_vec3  { float  x, y, z; };\n"
           "struct hgi_dvec3 { double x, y, z; };\n"
           "struct hgi_mat3  { float  m00, m01, m02,\n"
           "                          m10, m11, m12,\n"
           "                          m20, m21, m22; };\n"
           "struct hgi_dmat3 { double m00, m01, m02,\n"
           "                          m10, m11, m12,\n"
           "                          m20, m21, m22; };\n";
}

template<typename SectionType, typename... T>
SectionType*
HgiWebGPUShaderGenerator::CreateShaderSection(T&&... t)
{
    std::unique_ptr<SectionType> p =
        std::make_unique<SectionType>(std::forward<T>(t)...);
    SectionType* const result = p.get();
    GetShaderSections()->push_back(std::move(p));
    return result;
}

HgiWebGPUShaderGenerator::HgiWebGPUShaderGenerator(
    HgiWebGPU const* hgi, const HgiShaderFunctionDesc& descriptor)
    : HgiShaderGenerator(descriptor)
    , _hgi(hgi)
    , _inLocationIndex(0)
    , _outLocationIndex(0)
{
    // Write out all GL shaders and add to shader sections

    if (descriptor.shaderStage == HgiShaderStageCompute) {
        int workSizeX = descriptor.computeDescriptor.localSize[0];
        int workSizeY = descriptor.computeDescriptor.localSize[1];
        int workSizeZ = descriptor.computeDescriptor.localSize[2];

        if (workSizeX == 0 || workSizeY == 0 || workSizeZ == 0) {
            workSizeX = 1;
            workSizeY = 1;
            workSizeZ = 1;
        }

        _shaderLayoutAttributes.push_back(std::string("layout(") +
            "local_size_x = " + std::to_string(workSizeX) +
            ", "
            "local_size_y = " +
            std::to_string(workSizeY) +
            ", "
            "local_size_z = " +
            std::to_string(workSizeZ) + ") in;\n");
    }

    // The ordering here is important (buffers before textures), because we
    // need to increment the bind location for resources in the same order
    // as HgiWebGPUResourceBindings.
    // In WebGPU buffers and textures cannot have the same binding index.
    _WriteConstantParams(descriptor.constantParams);
    _WriteTextures(descriptor.textures);
    _WriteBuffers(descriptor.buffers);
    _WriteInOuts(descriptor.stageInputs, "in");
    _WriteInOutBlocks(descriptor.stageInputBlocks, "in");
    _WriteInOuts(descriptor.stageOutputs, "out");
    _WriteInOutBlocks(descriptor.stageOutputBlocks, "out");
}

void
HgiWebGPUShaderGenerator::_WriteVersion(std::ostream& ss)
{
    const int glslVersion = _hgi->GetCapabilities()->GetShaderVersion();

    ss << "#version " << std::to_string(glslVersion) << "\n";
}

void
HgiWebGPUShaderGenerator::_WriteExtensions(std::ostream& ss)
{
    const int glslVersion = _hgi->GetCapabilities()->GetShaderVersion();
    const bool shaderDrawParametersEnabled = _hgi->GetCapabilities()->IsSet(
        HgiDeviceCapabilitiesBitsShaderDrawParameters);
    const bool builtinBarycentricsEnabled = _hgi->GetCapabilities()->IsSet(
        HgiDeviceCapabilitiesBitsBuiltinBarycentrics);

    if (_GetShaderStage() & HgiShaderStageVertex) {
        if (glslVersion < 460 && shaderDrawParametersEnabled) {
            ss << "#extension GL_ARB_shader_draw_parameters : require\n";
        }
        if (shaderDrawParametersEnabled) {
            ss << "int HgiGetBaseVertex() {\n";
            if (glslVersion < 460) { // use ARB extension
                ss << "  return gl_BaseVertexARB;\n";
            } else {
                ss << "  return gl_BaseVertex;\n";
            }
            ss << "}\n";
        }
    }

    if (_GetShaderStage() & HgiShaderStageFragment) {
        if (builtinBarycentricsEnabled) {
            ss << "#extension GL_NV_fragment_shader_barycentric: require\n";
        }
    }
}
void
HgiWebGPUShaderGenerator::_WriteMacros(std::ostream& ss)
{

    if (!_hgi->GetPrimaryDevice().HasFeature(
            wgpu::FeatureName::PrimitiveIndex)) {
        // This is a excessively difficult thing to emulate.
        ss << "#define gl_PrimitiveID 0\n";
    }

    ss << "#define gl_PointCoord vec2(0.5)\n" // TODO: gl_PointCoord not
                                              // implemented in webgpu, faking
                                              // it for the moment
          "#define centroid\n" // TODO: avoid interpolation qualifier due to
                               // limited support
          "#define REF(space,type) inout type\n"
          "#define FORWARD_DECL(func_decl) func_decl\n"
          "#define ATOMIC_LOAD(a) (a)\n"
          "#define ATOMIC_STORE(a, v) (a) = (v)\n"
          "#define ATOMIC_ADD(a, v) atomicAdd(a, v)\n"
          "#define ATOMIC_EXCHANGE(a, v) atomicExchange(a, v)\n"
          "#define ATOMIC_COMPARE_EXCHANGE(a, expected, desired) "
          "atomicCompSwap(a, expected, desired)\n"
          "#define ATOMIC_COMP_SWAP(a, expected, desired) atomicCompSwap(a, "
          "expected, desired)\n"
          "#define atomic_int int\n"
          "#define atomic_uint uint\n"
          "#define hd_SampleMask gl_SampleMask[0]\n"
          "float dummy_PointSize = 1.0f;\n" // Define a dummy variable
          "#define gl_PointSize dummy_PointSize\n"; // Redirect gl_PointSize to
                                                    // dummy variable

    // Advertise to shader code that we support double precision math
    // and don't support IEEE float special values (NaN, +-Inf).
    ss << "\n"
       << "#define HGI_HAS_DOUBLE_TYPE 1\n"
       << "#define HGI_HAS_IEEE_FLOAT_SPECIAL_VALUES 0\n"
       << "\n";

    // Define platform independent baseInstance as 0
    ss << "#define gl_BaseInstance 0\n";
}

void
HgiWebGPUShaderGenerator::_WriteConstantParams(
    const HgiShaderFunctionParamDescVector& parameters)
{
    if (parameters.empty()) {
        return;
    }

    const HgiShaderSectionAttributeVector attrs = {
        HgiShaderSectionAttribute{"std140", ""},
        HgiShaderSectionAttribute{"binding", std::to_string(0)},
        HgiShaderSectionAttribute{"set",
            std::to_string(HgiWebGPUBufferShaderSection::constantsBindingSet)}};

    HgiWebGPUMemberShaderSectionPtrVector members;
    for (const HgiShaderFunctionParamDesc& param : parameters) {
        auto* memberSection = CreateShaderSection<HgiWebGPUMemberShaderSection>(
            /*nameInShader=*/param.nameInShader,
            /*type=*/param.type,
            /*interpolation=*/param.interpolation,
            /*sampling=*/HgiSamplingDefault,
            /*storage=*/HgiStorageDefault,
            /*attributes=*/HgiShaderSectionAttributeVector(),
            /*storageQualifier=*/std::string(),
            /*defaultValue=*/std::string(),
            /*arraySize=*/std::string(),
            /*blockInstanceIdentifier=*/param.nameInShader);
        members.push_back(memberSection);
    }

    CreateShaderSection<HgiWebGPUInterstageBlockShaderSection>(
        "ParamBuffer", "", attrs, "uniform", std::string(), members);
}

void
HgiWebGPUShaderGenerator::_WriteTextures(
    const HgiShaderFunctionTextureDescVector& textures)
{
    size_t bindingIdx = 0;
    for (size_t i = 0; i < textures.size(); i++) {
        const HgiShaderFunctionTextureDesc& desc = textures[i];
        const bool isShadow =
            desc.textureType == HgiShaderTextureTypeShadowTexture;
        const size_t count = std::max(size_t(1), desc.arraySize);

        if (desc.arraySize > 0) {
            // Tint's SPIR-V reader cannot represent arrays of opaque handle
            // types (texture/sampler), so we expand each array element into
            // its own individual binding and emit switch-dispatch wrappers.
            for (size_t j = 0; j < count; j++) {
                const std::string elemName =
                    desc.nameInShader + "_" + std::to_string(j);

                HgiShaderSectionAttributeVector elemAttrs = {
                    HgiShaderSectionAttribute{
                        "binding", std::to_string(bindingIdx + j)},
                    HgiShaderSectionAttribute{"set",
                        std::to_string(
                            HgiWebGPUTextureShaderSection::bindingSet)},
                };
                HgiShaderSectionAttributeVector elemSamplerAttrs = {
                    HgiShaderSectionAttribute{
                        "binding", std::to_string(bindingIdx + j)},
                    HgiShaderSectionAttribute{"set",
                        std::to_string(
                            HgiWebGPUSamplerShaderSection::bindingSet)},
                };
                if (desc.writable) {
                    elemAttrs.insert(elemAttrs.begin(),
                        HgiShaderSectionAttribute{
                            HgiWebGPUConversions::GetImageLayoutFormatQualifier(
                                desc.format),
                            ""});
                }

                HgiWebGPUSamplerShaderSection* const samplerSection =
                    CreateShaderSection<HgiWebGPUSamplerShaderSection>(
                        elemName, 0u, isShadow, elemSamplerAttrs);

                CreateShaderSection<HgiWebGPUTextureShaderSection>(elemName,
                    samplerSection, desc.dimensions, desc.format,
                    desc.textureType, 0u, desc.writable, elemAttrs);
            }

            // Emit switch-dispatch wrappers for array access.
            // These wrap HgiGet_name_N() calls behind a single index.
            const uint32_t coordDim =
                (isShadow ||
                    desc.textureType == HgiShaderTextureTypeArrayTexture ||
                    desc.textureType == HgiShaderTextureTypeCubemapTexture) ?
                (desc.dimensions + 1) :
                desc.dimensions;
            const std::string floatCoordType =
                coordDim == 1 ? "float" : "vec" + std::to_string(coordDim);
            const std::string intCoordType =
                coordDim == 1 ? "int" : "ivec" + std::to_string(coordDim);
            const std::string resultType = isShadow ? "float" : "vec4";

            auto emitSwitch = [&](std::ostream& ss, const std::string& fn,
                                  const std::string& args,
                                  const std::string& callArgs) {
                ss << resultType << " " << fn << "_" << desc.nameInShader
                   << "(uint index, " << args << ") {\n";
                for (size_t j = 0; j < count; j++) {
                    ss << "  if (index == " << j << "u) return " << fn << "_"
                       << desc.nameInShader << "_" << j << "(" << callArgs
                       << ");\n";
                }
                ss << "  return " << resultType << "(0);\n}\n";
            };

            // HgiGetSampler_ macro: stays in Global Macros scope
            CreateShaderSection<HgiWebGPUMacroShaderSection>(
                "#define HgiGetSampler_" + desc.nameInShader +
                    "(index) textureBind_" + desc.nameInShader + "_0\n",
                "");

            // Switch-dispatch wrappers: must be in Global Function Definitions
            // so they can call the per-element HgiGet_name_N() functions that
            // are emitted there by HgiWebGPUTextureShaderSection.
            std::ostringstream fns;
            emitSwitch(fns, "HgiGet", floatCoordType + " uv", "uv");
            emitSwitch(fns, "HgiTextureLod",
                floatCoordType + " coord, float lod", "coord, lod");
            if (!isShadow &&
                desc.textureType != HgiShaderTextureTypeCubemapTexture) {
                emitSwitch(
                    fns, "HgiTexelFetch", intCoordType + " coord", "coord");
            }

            CreateShaderSection<HgiWebGPUFunctionDefShaderSection>(fns.str());
        } else {
            HgiShaderSectionAttributeVector attrs = {
                HgiShaderSectionAttribute{
                    "binding", std::to_string(bindingIdx)},
                HgiShaderSectionAttribute{"set",
                    std::to_string(HgiWebGPUTextureShaderSection::bindingSet)},
            };
            HgiShaderSectionAttributeVector samplerAttributes = {
                HgiShaderSectionAttribute{
                    "binding", std::to_string(bindingIdx)},
                HgiShaderSectionAttribute{"set",
                    std::to_string(HgiWebGPUSamplerShaderSection::bindingSet)},
            };
            if (desc.writable) {
                attrs.insert(attrs.begin(),
                    HgiShaderSectionAttribute{
                        HgiWebGPUConversions::GetImageLayoutFormatQualifier(
                            desc.format),
                        ""});
            }

            HgiWebGPUSamplerShaderSection* const samplerSection =
                CreateShaderSection<HgiWebGPUSamplerShaderSection>(
                    desc.nameInShader, 0u, isShadow, samplerAttributes);

            CreateShaderSection<HgiWebGPUTextureShaderSection>(
                desc.nameInShader, samplerSection, desc.dimensions, desc.format,
                desc.textureType, 0u, desc.writable, attrs);
        }

        bindingIdx += count;
    }
}

void
HgiWebGPUShaderGenerator::_WriteBuffers(
    const HgiShaderFunctionBufferDescVector& buffers)
{
    // Extract buffer descriptors and add appropriate buffer sections
    for (size_t i = 0; i < buffers.size(); i++) {
        const HgiShaderFunctionBufferDesc& bufferDescription = buffers[i];

        const bool isUniformBufferBinding =
            (bufferDescription.binding == HgiBindingTypeUniformValue) ||
            (bufferDescription.binding == HgiBindingTypeUniformArray);

        std::string arraySize = (bufferDescription.arraySize > 0) ?
            std::to_string(bufferDescription.arraySize) :
            std::string();
        if (isUniformBufferBinding) {
            const HgiShaderSectionAttributeVector attrs = {
                HgiShaderSectionAttribute{"std140", ""},
                HgiShaderSectionAttribute{
                    "binding", std::to_string(bufferDescription.bindIndex)},
                HgiShaderSectionAttribute{"set",
                    std::to_string(HgiWebGPUBufferShaderSection::bindingSet)}};
            HgiBindingType bindingType = bufferDescription.binding;
            CreateShaderSection<HgiWebGPUBufferShaderSection>(
                bufferDescription.nameInShader, bufferDescription.writable,
                bufferDescription.type, bindingType, arraySize, attrs);
        } else {
            bool writable = bufferDescription.writable;
            if (writable && _GetShaderStage() & HgiShaderStageVertex) {
                TF_WARN("No support for writable buffers in vertex stage.");
            }
            const HgiShaderSectionAttributeVector attrs = {
                HgiShaderSectionAttribute{"std430", ""},
                HgiShaderSectionAttribute{
                    "binding", std::to_string(bufferDescription.bindIndex)},
                HgiShaderSectionAttribute{"set",
                    std::to_string(HgiWebGPUBufferShaderSection::bindingSet)}};

            CreateShaderSection<HgiWebGPUBufferShaderSection>(
                bufferDescription.nameInShader, writable,
                bufferDescription.type, bufferDescription.binding, arraySize,
                attrs);
        }
    }
}

void
HgiWebGPUShaderGenerator::_WriteInOuts(
    const HgiShaderFunctionParamDescVector& parameters,
    const std::string& qualifier)
{
    // To unify glslfx across different apis, other apis may want these to be
    // defined, but since they are taken in opengl we ignore them.
    const static std::set<std::string> takenOutParams{
        "gl_Position",
        "gl_FragColor",
        "gl_FragDepth",
        "gl_PointSize",
        "gl_CullDistance",
        "hd_SampleMask",
    };

    // Some params are built-in, but we may want to declare them in the shader
    // anyway, such as to declare their array size.
    const static std::set<std::string> takenOutParamsToDeclare{
        "gl_ClipDistance"};

    const static std::unordered_map<std::string, std::string> takenInParams{
        {HgiShaderKeywordTokens->hdPosition, "gl_Position"},
        {HgiShaderKeywordTokens->hdPointCoord, "gl_PointCoord"},
        {HgiShaderKeywordTokens->hdClipDistance, "gl_ClipDistance"},
        {HgiShaderKeywordTokens->hdCullDistance, "gl_CullDistance"},
        {HgiShaderKeywordTokens->hdVertexID, "gl_VertexIndex"},
        {HgiShaderKeywordTokens->hdInstanceID, "gl_InstanceIndex"},
        {HgiShaderKeywordTokens->hdPrimitiveID, "gl_PrimitiveID"},
        {HgiShaderKeywordTokens->hdSampleID, "gl_SampleID"},
        {HgiShaderKeywordTokens->hdSamplePosition, "gl_SamplePosition"},
        {HgiShaderKeywordTokens->hdFragCoord, "gl_FragCoord"},
        {HgiShaderKeywordTokens->hdBaseVertex, "gl_BaseVertex"},
        {HgiShaderKeywordTokens->hdBaseInstance, "gl_BaseInstance"},
        {HgiShaderKeywordTokens->hdFrontFacing, "gl_FrontFacing"},
        {HgiShaderKeywordTokens->hdLayer, "gl_Layer"},
        {HgiShaderKeywordTokens->hdViewportIndex, "gl_ViewportIndex"},
        {HgiShaderKeywordTokens->hdGlobalInvocationID, "gl_GlobalInvocationID"},
        {HgiShaderKeywordTokens->hdBaryCoordNoPersp, "gl_BaryCoordNoPerspEXT"},
        {HgiShaderKeywordTokens->hdSampleMaskIn, "gl_SampleMaskIn[0]"}};

    const bool in_qualifier = qualifier == "in";
    const bool out_qualifier = qualifier == "out";
    for (const HgiShaderFunctionParamDesc& param : parameters) {
        // Skip writing out taken parameter names
        const std::string& paramName = param.nameInShader;
        if (out_qualifier &&
            takenOutParams.find(paramName) != takenOutParams.end()) {
            continue;
        }
        if (out_qualifier &&
            takenOutParamsToDeclare.find(paramName) !=
                takenOutParamsToDeclare.end()) {
            CreateShaderSection<HgiWebGPUMemberShaderSection>(paramName, param.type,
                param.interpolation, param.sampling, param.storage,
                HgiShaderSectionAttributeVector(), qualifier, std::string(),
                param.arraySize);
            continue;
        }
        if (in_qualifier) {
            const std::string& role = param.role;
            auto const& keyword = takenInParams.find(role);
            if (keyword != takenInParams.end()) {
                if (paramName != keyword->second) {
                    CreateShaderSection<HgiWebGPUKeywordShaderSection>(
                        paramName, param.type, keyword->second);
                }
                continue;
            }
        }

        // If a location or interstage slot has been specified then add it to
        // the attributes.
        HgiShaderSectionAttributeVector attrs;
        if (param.location != -1) {
            // If a location has been specified then add it to the attributes.
            attrs.push_back({"location", std::to_string(param.location)});
        } else if (param.interstageSlot != -1) {
            // For interstage parameters use the interstageSlot for location.
            attrs.push_back({"location", std::to_string(param.interstageSlot)});
        } else {
            // Otherwise use shader generator's counter sytem.
            const int32_t locationIndex =
                in_qualifier ? _inLocationIndex++ : _outLocationIndex++;
            attrs.push_back({"location", std::to_string(locationIndex)});
        }

        CreateShaderSection<HgiWebGPUMemberShaderSection>(paramName, param.type,
            param.interpolation, param.sampling, param.storage, attrs,
            qualifier, std::string(), param.arraySize);
    }
}

void
HgiWebGPUShaderGenerator::_WriteInOutBlocks(
    const HgiShaderFunctionParamBlockDescVector& parameterBlocks,
    const std::string& qualifier)
{
    const bool in_qualifier = qualifier == "in";
    const bool out_qualifier = qualifier == "out";

    for (const HgiShaderFunctionParamBlockDesc& p : parameterBlocks) {
        const uint32_t locationIndex =
            in_qualifier ? _inLocationIndex : _outLocationIndex;

        HgiWebGPUMemberShaderSectionPtrVector members;
        for (const HgiShaderFunctionParamBlockDesc::Member& member :
            p.members) {

            HgiWebGPUMemberShaderSection* memberSection =
                CreateShaderSection<HgiWebGPUMemberShaderSection>(member.name,
                    member.type, member.interpolation, HgiSamplingDefault,
                    HgiStorageDefault, HgiShaderSectionAttributeVector(),
                    qualifier, std::string(), std::string(), p.instanceName);
            members.push_back(memberSection);

            if (in_qualifier) {
                _inLocationIndex++;
            } else if (out_qualifier) {
                _outLocationIndex++;
            }
        }

        // If interstage slot has been specified then add it to the attributes.
        HgiShaderSectionAttributeVector attrs;
        if (p.interstageSlot != -1) {
            // For interstage parameters use the interstageSlot for location.
            attrs.push_back({"location", std::to_string(p.interstageSlot)});
        } else {
            // Otherwise use shader generator's counter sytem.
            attrs.push_back({"location", std::to_string(locationIndex)});
        }

        CreateShaderSection<HgiWebGPUInterstageBlockShaderSection>(p.blockName,
            p.instanceName, attrs, qualifier, p.arraySize, members);
    }
}

void
HgiWebGPUShaderGenerator::_Execute(std::ostream& ss)
{
    // Version number must be first line in glsl shader
    _WriteVersion(ss);

    _WriteExtensions(ss);

    // Write out all GL shaders and add to shader sections
    _WriteMacros(ss);

    ss << _GetPackedTypeDefinitions() << "\n";

    ss << _GetShaderCodeDeclarations() << "\n";

    for (const std::string& attr : _shaderLayoutAttributes) {
        ss << attr;
    }

    HgiWebGPUShaderSectionUniquePtrVector* shaderSections = GetShaderSections();
    // For all shader sections, visit the areas defined for all
    // shader apis. We assume all shader apis have a global space
    // section, capabilities to define macros in global space,
    // and abilities to declare some members or functions there

    ss << "\n// //////// Global Includes ////////\n";
    for (const std::unique_ptr<HgiWebGPUShaderSection>& shaderSection :
        *shaderSections) {
        shaderSection->VisitGlobalIncludes(ss);
    }

    ss << "\n// //////// Global Macros ////////\n";
    for (const std::unique_ptr<HgiWebGPUShaderSection>& shaderSection :
        *shaderSections) {
        shaderSection->VisitGlobalMacros(ss);
    }

    ss << "\n// //////// Global Structs ////////\n";
    for (const std::unique_ptr<HgiWebGPUShaderSection>& shaderSection :
        *shaderSections) {
        shaderSection->VisitGlobalStructs(ss);
    }

    ss << "\n// //////// Global Member Declarations ////////\n";
    for (const std::unique_ptr<HgiWebGPUShaderSection>& shaderSection :
        *shaderSections) {
        shaderSection->VisitGlobalMemberDeclarations(ss);
    }

    ss << "\n// //////// Global Function Definitions ////////\n";
    for (const std::unique_ptr<HgiWebGPUShaderSection>& shaderSection :
        *shaderSections) {
        shaderSection->VisitGlobalFunctionDefinitions(ss);
    }

    ss << "\n";

    // write all the original shader
    ss << _GetShaderCode();
}

HgiWebGPUShaderSectionUniquePtrVector*
HgiWebGPUShaderGenerator::GetShaderSections()
{
    return &_shaderSections;
}

PXR_NAMESPACE_CLOSE_SCOPE
