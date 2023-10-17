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

#include "pxr/imaging/hgiWebGPU/shaderGenerator.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/shaderSection.h"
#include "pxr/imaging/hgi/shaderSection.h"
#include "pxr/imaging/hgiWebGPU/shaderSection.h"
#include "pxr/imaging/hgiWebGPU/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include <unordered_map>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE

static const char *
_GetPackedTypeDefinitions()
{
    return
        "\n"
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

template<typename SectionType, typename ...T>
SectionType *
HgiWebGPUShaderGenerator::CreateShaderSection(T && ...t)
{
    std::unique_ptr<SectionType> p =
        std::make_unique<SectionType>(std::forward<T>(t)...);
    SectionType * const result = p.get();
    GetShaderSections()->push_back(std::move(p));
    return result;
}

HgiWebGPUShaderGenerator::HgiWebGPUShaderGenerator(
    Hgi const *hgi,
    const HgiShaderFunctionDesc &descriptor)
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
      
        _shaderLayoutAttributes.push_back(
            std::string("layout(") +
            "local_size_x = " + std::to_string(workSizeX) + ", "
            "local_size_y = " + std::to_string(workSizeY) + ", "
            "local_size_z = " + std::to_string(workSizeZ) + ") in;\n"
        );
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
HgiWebGPUShaderGenerator::_WriteVersion(std::ostream &ss)
{
    const int glslVersion = _hgi->GetCapabilities()->GetShaderVersion();

    ss << "#version " << std::to_string(glslVersion) << "\n";
}

void
HgiWebGPUShaderGenerator::_WriteExtensions(std::ostream &ss)
{
    const int glslVersion = _hgi->GetCapabilities()->GetShaderVersion();
    const bool shaderDrawParametersEnabled = _hgi->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsShaderDrawParameters);
    const bool builtinBarycentricsEnabled = _hgi->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsBuiltinBarycentrics);

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
HgiWebGPUShaderGenerator::_WriteMacros(std::ostream &ss)
{
    ss << "#define gl_PrimitiveID 1\n" // TODO: gl_PrimitiveID not implemented in webgpu, faking it for the moment
          "#define centroid\n" // TODO: avoid interpolation qualifier due to limited support
          "#define REF(space,type) inout type\n"
          "#define FORWARD_DECL(func_decl) func_decl\n"
          "#define ATOMIC_LOAD(a) (a)\n"
          "#define ATOMIC_STORE(a, v) (a) = (v)\n"
          "#define ATOMIC_ADD(a, v) atomicAdd(a, v)\n"
          "#define ATOMIC_EXCHANGE(a, v) atomicExchange(a, v)\n"
          "#define atomic_int int\n"
          "#define atomic_uint uint\n";

    // Advertise to shader code that we support double precision math
    ss << "\n"
        << "#define HGI_HAS_DOUBLE_TYPE 1\n"
        << "\n";

    // Define platform independent baseInstance as 0
    ss << "#define gl_BaseInstance 0\n";
}

void
HgiWebGPUShaderGenerator::_WriteConstantParams(
    const HgiShaderFunctionParamDescVector &parameters)
{
    if (parameters.empty()) {
        return;
    }
    CreateShaderSection<HgiGLBlockShaderSection>(
        "ParamBuffer",
        parameters);
}

void
HgiWebGPUShaderGenerator::_WriteTextures(
    const HgiShaderFunctionTextureDescVector& textures)
{
    for (size_t i=0; i<textures.size(); i++) {
        const HgiShaderFunctionTextureDesc &desc = textures[i];
        HgiShaderSectionAttributeVector attrs = {
            HgiShaderSectionAttribute{"binding", std::to_string(i)},
            HgiShaderSectionAttribute{"set", std::to_string(HgiWebGPUTextureShaderSection::bindingSet)},
        };

        const HgiShaderSectionAttributeVector samplerAttributes = {
            HgiShaderSectionAttribute{"binding", std::to_string(i)},
            HgiShaderSectionAttribute{"set", std::to_string(HgiWebGPUSamplerShaderSection::bindingSet)},
        };
        if (desc.writable) {
            attrs.insert(attrs.begin(), HgiShaderSectionAttribute{
                    HgiGLConversions::GetImageLayoutFormatQualifier(
                    desc.format), 
                ""});
        }

        HgiWebGPUSamplerShaderSection * const samplerSection =
                CreateShaderSection<HgiWebGPUSamplerShaderSection>(
                        desc.nameInShader,
                        desc.arraySize,
                        samplerAttributes);

        CreateShaderSection<HgiWebGPUTextureShaderSection>(
            desc.nameInShader,
            samplerSection,
            desc.dimensions,
            desc.format,
            desc.textureType,
            desc.arraySize,
            desc.writable,
            attrs);
    }
}

void
HgiWebGPUShaderGenerator::_WriteBuffers(
    const HgiShaderFunctionBufferDescVector &buffers)
{
    //Extract buffer descriptors and add appropriate buffer sections
    for(size_t i=0; i<buffers.size(); i++) {
        const HgiShaderFunctionBufferDesc &bufferDescription = buffers[i];

        const bool isUniformBufferBinding =
            (bufferDescription.binding == HgiBindingTypeUniformValue) ||
            (bufferDescription.binding == HgiBindingTypeUniformArray);

        std::string arraySize =
            (bufferDescription.arraySize > 0)
                ? std::to_string(bufferDescription.arraySize)
                : std::string();
        if (isUniformBufferBinding) {
            const HgiShaderSectionAttributeVector attrs = {
                HgiShaderSectionAttribute{"std140", ""},
                HgiShaderSectionAttribute{"binding", std::to_string(bufferDescription.bindIndex)},
                HgiShaderSectionAttribute{"set", std::to_string(HgiWebGPUBufferShaderSection::bindingSet)}};
            HgiBindingType bindingType = bufferDescription.binding;
            CreateShaderSection<HgiWebGPUBufferShaderSection>(
                bufferDescription.nameInShader,
                bufferDescription.writable,
                bufferDescription.type,
                bindingType,
                arraySize,
                attrs);
        } else {
            bool writable = bufferDescription.writable;
            if (writable && _GetShaderStage() & HgiShaderStageVertex) {
                TF_WARN("No support for writable buffers in vertex stage.");
            }
            const HgiShaderSectionAttributeVector attrs = {
                HgiShaderSectionAttribute{"std430", ""},
                HgiShaderSectionAttribute{"binding", std::to_string(bufferDescription.bindIndex)},
                HgiShaderSectionAttribute{"set", std::to_string(HgiWebGPUBufferShaderSection::bindingSet)}};

            CreateShaderSection<HgiWebGPUBufferShaderSection>(
                bufferDescription.nameInShader,
                writable,
                bufferDescription.type,
                bufferDescription.binding,
                arraySize,
                attrs);
        }
    }
}

void
HgiWebGPUShaderGenerator::_WriteInOuts(
    const HgiShaderFunctionParamDescVector &parameters,
    const std::string &qualifier)
{
    //To unify glslfx across different apis, other apis
    //may want these to be defined, but since they are
    //taken in opengl we ignore them
    const static std::set<std::string> takenOutParams {
        "gl_Position",
        "gl_FragColor",
        "gl_FragDepth",
        "gl_PointSize",
        "gl_ClipDistance",
        "gl_CullDistance",
    };

    const static std::unordered_map<std::string, std::string> takenInParams {
        { HgiShaderKeywordTokens->hdPosition, "gl_Position"},
        { HgiShaderKeywordTokens->hdPointCoord, "gl_PointCoord"},
        { HgiShaderKeywordTokens->hdClipDistance, "gl_ClipDistance"},
        { HgiShaderKeywordTokens->hdCullDistance, "gl_CullDistance"},
        { HgiShaderKeywordTokens->hdVertexID, "gl_VertexIndex"},
        { HgiShaderKeywordTokens->hdInstanceID, "gl_InstanceIndex"},
        { HgiShaderKeywordTokens->hdPrimitiveID, "gl_PrimitiveID"},
        { HgiShaderKeywordTokens->hdSampleID, "gl_SampleID"},
        { HgiShaderKeywordTokens->hdSamplePosition, "gl_SamplePosition"},
        { HgiShaderKeywordTokens->hdFragCoord, "gl_FragCoord"},
        { HgiShaderKeywordTokens->hdBaseVertex, "gl_BaseVertex"},
        { HgiShaderKeywordTokens->hdBaseInstance, "gl_BaseInstance"},
        { HgiShaderKeywordTokens->hdFrontFacing, "gl_FrontFacing"},
        { HgiShaderKeywordTokens->hdLayer, "gl_Layer"},
        { HgiShaderKeywordTokens->hdViewportIndex, "gl_ViewportIndex"},
        { HgiShaderKeywordTokens->hdGlobalInvocationID, "gl_GlobalInvocationID"},
        { HgiShaderKeywordTokens->hdBaryCoordNoPerspNV, "gl_BaryCoordNoPerspNV"},
    };

    const bool in_qualifier = qualifier == "in";
    const bool out_qualifier = qualifier == "out";
    for(const HgiShaderFunctionParamDesc &param : parameters) {
        //Skip writing out taken parameter names
        const std::string &paramName = param.nameInShader;
        if (out_qualifier &&
                takenOutParams.find(paramName) != takenOutParams.end()) {
            continue;
        }
        if (in_qualifier) {
            const std::string &role = param.role;
            auto const& keyword = takenInParams.find(role);
            if (keyword != takenInParams.end()) {
                if (role == HgiShaderKeywordTokens->hdGlobalInvocationID) {
                    CreateShaderSection<HgiGLKeywordShaderSection>(
                            paramName,
                            param.type,
                            keyword->second);
                } else if (role == HgiShaderKeywordTokens->hdVertexID) {
                    CreateShaderSection<HgiGLKeywordShaderSection>(
                            paramName,
                            param.type,
                            keyword->second);
                } else if (role == HgiShaderKeywordTokens->hdInstanceID) {
                    CreateShaderSection<HgiGLKeywordShaderSection>(
                            paramName,
                            param.type,
                            keyword->second);
                } else if (role == HgiShaderKeywordTokens->hdBaseInstance) {
                    CreateShaderSection<HgiGLKeywordShaderSection>(
                            paramName,
                            param.type,
                            keyword->second);
                }
                continue;
            }
        }

        // If a location has been specified then add it to the attributes.
        const int32_t locationIndex =
                param.location >= 0
                ? param.location
                : (in_qualifier ? _inLocationIndex++ : _outLocationIndex++);

        const HgiShaderSectionAttributeVector attrs {
                HgiShaderSectionAttribute{
                        "location", std::to_string(locationIndex) }
        };

        CreateShaderSection<HgiGLMemberShaderSection>(
                paramName,
                param.type,
                param.interpolation,
                param.sampling,
                param.storage,
                attrs,
                qualifier,
                std::string(),
                param.arraySize);
    }
}

void
HgiWebGPUShaderGenerator::_WriteInOutBlocks(
    const HgiShaderFunctionParamBlockDescVector &parameterBlocks,
    const std::string &qualifier)
{
    const bool in_qualifier = qualifier == "in";
    const bool out_qualifier = qualifier == "out";

    for(const HgiShaderFunctionParamBlockDesc &p : parameterBlocks) {
        const uint32_t locationIndex = in_qualifier ?
            _inLocationIndex : _outLocationIndex;

        HgiBaseGLShaderSectionPtrVector members;
        for(const HgiShaderFunctionParamBlockDesc::Member &member : p.members) {

            HgiGLMemberShaderSection *memberSection =
                CreateShaderSection<HgiGLMemberShaderSection>(
                    member.name,
                    member.type,
                    HgiInterpolationDefault,
                    HgiSamplingDefault,
                    HgiStorageDefault,
                    HgiShaderSectionAttributeVector(),
                    qualifier,
                    std::string(),
                    std::string(),
                    p.instanceName);
            members.push_back(memberSection);

            if (in_qualifier) {
                _inLocationIndex++;
            } else if (out_qualifier) {
                _outLocationIndex++;
            }
        }

        const HgiShaderSectionAttributeVector attrs {
                HgiShaderSectionAttribute{
                        "location", std::to_string(locationIndex) }
        };

        CreateShaderSection<HgiWebGPUInterstageBlockShaderSection>(
            p.blockName,
            p.instanceName,
            attrs,
            qualifier,
            p.arraySize,
            members);
    }
}

std::string removeLine(const std::string& input, const std::string& toRemove) {
    std::istringstream iss(input);
    std::ostringstream oss;
    std::string line;
    while (std::getline(iss, line)) {
        if (line.find(toRemove) == std::string::npos) {
            oss << line << std::endl;
        }
    }
    return oss.str();
}

void
HgiWebGPUShaderGenerator::_Execute(std::ostream &ss)
{
    // Version number must be first line in glsl shader
    _WriteVersion(ss);

    _WriteExtensions(ss);

    // Write out all GL shaders and add to shader sections
    _WriteMacros(ss);

    ss << _GetPackedTypeDefinitions() << "\n";

    ss << _GetShaderCodeDeclarations() << "\n";

    for (const std::string &attr : _shaderLayoutAttributes) {
        ss << attr;
    }

    HgiBaseGLShaderSectionUniquePtrVector* shaderSections = GetShaderSections();
    //For all shader sections, visit the areas defined for all
    //shader apis. We assume all shader apis have a global space
    //section, capabilities to define macros in global space,
    //and abilities to declare some members or functions there
    
    ss << "\n// //////// Global Includes ////////\n";
    for (const std::unique_ptr<HgiBaseGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalIncludes(ss);
    }

    ss << "\n// //////// Global Macros ////////\n";
    for (const std::unique_ptr<HgiBaseGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalMacros(ss);
    }

    ss << "\n// //////// Global Structs ////////\n";
    for (const std::unique_ptr<HgiBaseGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalStructs(ss);
    }

    ss << "\n// //////// Global Member Declarations ////////\n";
    for (const std::unique_ptr<HgiBaseGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalMemberDeclarations(ss);
    }

    ss << "\n// //////// Global Function Definitions ////////\n";
    for (const std::unique_ptr<HgiBaseGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalFunctionDefinitions(ss);
    }

    ss << "\n";

    // write all the original shader
    // TODO: we do a nasty hack of removing any line using or assigning gl_PointSize
    std::string preprocessedShader = removeLine(_GetShaderCode(), "gl_PointSize");
    ss << preprocessedShader;
}

HgiBaseGLShaderSectionUniquePtrVector*
HgiWebGPUShaderGenerator::GetShaderSections()
{
    return &_shaderSections;
}

PXR_NAMESPACE_CLOSE_SCOPE
