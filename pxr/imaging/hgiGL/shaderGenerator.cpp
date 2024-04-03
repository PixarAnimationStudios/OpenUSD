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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/shaderGenerator.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgi/capabilities.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include <unordered_map>

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
HgiGLShaderGenerator::CreateShaderSection(T && ...t)
{
    std::unique_ptr<SectionType> p =
        std::make_unique<SectionType>(std::forward<T>(t)...);
    SectionType * const result = p.get();
    GetShaderSections()->push_back(std::move(p));
    return result;
}

HgiGLShaderGenerator::HgiGLShaderGenerator(
    Hgi const *hgi,
    const HgiShaderFunctionDesc &descriptor)
  : HgiShaderGenerator(descriptor)
  , _hgi(hgi)
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

        // Determine device's compute work group local size limits
        int maxLocalSize[3] = { 0, 0, 0 };
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &maxLocalSize[0]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &maxLocalSize[1]);
        glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &maxLocalSize[2]);

        if (workSizeX > maxLocalSize[0]) {
            TF_WARN("Max size of compute work group available from device is "
                    "%i, larger than %i", maxLocalSize[0], workSizeX);
            workSizeX = maxLocalSize[0];
        }
        if (workSizeY > maxLocalSize[1]) {
            TF_WARN("Max size of compute work group available from device is "
                    "%i, larger than %i", maxLocalSize[1], workSizeY);
            workSizeY = maxLocalSize[1];
        }
        if (workSizeZ > maxLocalSize[2]) {
            TF_WARN("Max size of compute work group available from device is "
                    "%i, larger than %i", maxLocalSize[2], workSizeZ);
            workSizeZ = maxLocalSize[2];
        }
      
        _shaderLayoutAttributes.push_back(
            std::string("layout(") +
            "local_size_x = " + std::to_string(workSizeX) + ", "
            "local_size_y = " + std::to_string(workSizeY) + ", "
            "local_size_z = " + std::to_string(workSizeZ) + ") in;\n"
        );
    } else if (descriptor.shaderStage == HgiShaderStageTessellationControl) {
        _shaderLayoutAttributes.emplace_back(
            "layout (vertices = " +
                descriptor.tessellationDescriptor.numVertsPerPatchOut +
                ") out;\n");
    } else if (descriptor.shaderStage == HgiShaderStageTessellationEval) {
        if (descriptor.tessellationDescriptor.patchType ==
                HgiShaderFunctionTessellationDesc::PatchType::Triangles) {
            _shaderLayoutAttributes.emplace_back(
                "layout (triangles) in;\n");
        } else if (descriptor.tessellationDescriptor.patchType ==
                HgiShaderFunctionTessellationDesc::PatchType::Quads) {
            _shaderLayoutAttributes.emplace_back(
                "layout (quads) in;\n");
        } else if (descriptor.tessellationDescriptor.patchType ==
                HgiShaderFunctionTessellationDesc::PatchType::Isolines) {
            _shaderLayoutAttributes.emplace_back(
                "layout (isolines) in;\n");
        }
        if (descriptor.tessellationDescriptor.spacing ==
                HgiShaderFunctionTessellationDesc::Spacing::Equal) {
            _shaderLayoutAttributes.emplace_back(
                "layout (equal_spacing) in;\n");
        } else if (descriptor.tessellationDescriptor.spacing ==
                HgiShaderFunctionTessellationDesc::Spacing::FractionalEven) {
            _shaderLayoutAttributes.emplace_back(
                "layout (fractional_even_spacing) in;\n");
        } else if (descriptor.tessellationDescriptor.spacing ==
                HgiShaderFunctionTessellationDesc::Spacing::FractionalOdd) {
            _shaderLayoutAttributes.emplace_back(
                "layout (fractional_odd_spacing) in;\n");
        }
        if (descriptor.tessellationDescriptor.ordering ==
                HgiShaderFunctionTessellationDesc::Ordering::CW) {
            _shaderLayoutAttributes.emplace_back(
                "layout (cw) in;\n");
        } else if (descriptor.tessellationDescriptor.ordering ==
                HgiShaderFunctionTessellationDesc::Ordering::CCW) {
            _shaderLayoutAttributes.emplace_back(
                "layout (ccw) in;\n");
        }
    } else if (descriptor.shaderStage == HgiShaderStageGeometry) {
        if (descriptor.geometryDescriptor.inPrimitiveType ==
            HgiShaderFunctionGeometryDesc::InPrimitiveType::Points) {
            _shaderLayoutAttributes.emplace_back(
                "layout (points) in;\n");
        } else if (descriptor.geometryDescriptor.inPrimitiveType ==
            HgiShaderFunctionGeometryDesc::InPrimitiveType::Lines) {
            _shaderLayoutAttributes.emplace_back(
                "layout (lines) in;\n");
        } else if (descriptor.geometryDescriptor.inPrimitiveType ==
            HgiShaderFunctionGeometryDesc::InPrimitiveType::LinesAdjacency) {
            _shaderLayoutAttributes.emplace_back(
                "layout (lines_adjacency) in;\n");
        } else if (descriptor.geometryDescriptor.inPrimitiveType ==
            HgiShaderFunctionGeometryDesc::InPrimitiveType::Triangles) {
            _shaderLayoutAttributes.emplace_back(
                "layout (triangles) in;\n");
        } else if (descriptor.geometryDescriptor.inPrimitiveType ==
            HgiShaderFunctionGeometryDesc::InPrimitiveType::TrianglesAdjacency){
            _shaderLayoutAttributes.emplace_back(
                "layout (triangles_adjacency) in;\n");
        }

        if (descriptor.geometryDescriptor.outPrimitiveType ==
            HgiShaderFunctionGeometryDesc::OutPrimitiveType::Points) {
            _shaderLayoutAttributes.emplace_back(
                "layout (points, max_vertices = " +
                descriptor.geometryDescriptor.outMaxVertices + ") out;\n");
        } else if (descriptor.geometryDescriptor.outPrimitiveType ==
            HgiShaderFunctionGeometryDesc::OutPrimitiveType::LineStrip) {
            _shaderLayoutAttributes.emplace_back(
                "layout (line_strip, max_vertices = " +
                descriptor.geometryDescriptor.outMaxVertices + ") out;\n");
        } else if (descriptor.geometryDescriptor.outPrimitiveType ==
            HgiShaderFunctionGeometryDesc::OutPrimitiveType::TriangleStrip) {
            _shaderLayoutAttributes.emplace_back(
                "layout (triangle_strip, max_vertices = " +
                descriptor.geometryDescriptor.outMaxVertices + ") out;\n");
        }
    } else if (descriptor.shaderStage == HgiShaderStageFragment) {
        if (descriptor.fragmentDescriptor.earlyFragmentTests) {
            _shaderLayoutAttributes.emplace_back(
                "layout (early_fragment_tests) in;\n");
        }
    }

    _WriteTextures(descriptor.textures);
    _WriteBuffers(descriptor.buffers);
    _WriteInOuts(descriptor.stageInputs, "in");
    _WriteInOutBlocks(descriptor.stageInputBlocks, "in");
    _WriteConstantParams(descriptor.constantParams);
    _WriteInOuts(descriptor.stageOutputs, "out");
    _WriteInOutBlocks(descriptor.stageOutputBlocks, "out");
}

void
HgiGLShaderGenerator::_WriteVersion(std::ostream &ss)
{
    const int glslVersion = _hgi->GetCapabilities()->GetShaderVersion();

    ss << "#version " << std::to_string(glslVersion) << "\n";
}

void
HgiGLShaderGenerator::_WriteExtensions(std::ostream &ss)
{
    const int glslVersion = _hgi->GetCapabilities()->GetShaderVersion();
    const bool bindlessBufferEnabled = _hgi->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsBindlessBuffers);
    const bool bindlessTextureEnabled = _hgi->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsBindlessTextures);
    const bool shaderDrawParametersEnabled = _hgi->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsShaderDrawParameters);
    const bool builtinBarycentricsEnabled = _hgi->GetCapabilities()->
        IsSet(HgiDeviceCapabilitiesBitsBuiltinBarycentrics);

    if (bindlessBufferEnabled) {
        ss << "#extension GL_NV_shader_buffer_load : require\n"
           << "#extension GL_NV_gpu_shader5 : require\n";
    }
    if (bindlessTextureEnabled) {
        ss << "#extension GL_ARB_bindless_texture : require\n";
    }

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
HgiGLShaderGenerator::_WriteMacros(std::ostream &ss)
{
    // Allows Metal and GL to both handle out function params.
    // On the Metal side, the ref(space,type) parameter defines
    // if items are in device or thread domain.
    ss << "#define REF(space,type) inout type\n"
          "#define FORWARD_DECL(func_decl) func_decl;\n"
          "#define ATOMIC_LOAD(a) (a)\n"
          "#define ATOMIC_STORE(a, v) (a) = (v)\n"
          "#define ATOMIC_ADD(a, v) atomicAdd(a, v)\n"
          "#define ATOMIC_EXCHANGE(a, v) atomicExchange(a, v)\n"
          "#define ATOMIC_COMP_SWAP(a, expected, desired) atomicCompSwap(a, "
          "expected, desired)\n"
          "#define atomic_int int\n"
          "#define atomic_uint uint\n";

    // Advertise to shader code that we support double precision math
    ss << "\n"
        << "#define HGI_HAS_DOUBLE_TYPE 1\n"
        << "\n";
}

void
HgiGLShaderGenerator::_WriteTextures(
    const HgiShaderFunctionTextureDescVector &textures)
{
    // Extract texture descriptors and add appropriate texture sections
    size_t binding = 0;
    for (size_t i=0; i<textures.size(); i++) {
        const HgiShaderFunctionTextureDesc &textureDescription = textures[i];
        HgiShaderSectionAttributeVector attrs = {
            HgiShaderSectionAttribute{"binding", std::to_string(binding)}};

        if (textureDescription.writable) {
            attrs.insert(attrs.begin(), HgiShaderSectionAttribute{
                HgiGLConversions::GetImageLayoutFormatQualifier(
                    textureDescription.format), 
                ""});
        }

        GetShaderSections()->push_back(
            std::make_unique<HgiGLTextureShaderSection>(
                textureDescription.nameInShader,
                i,
                textureDescription.dimensions,
                textureDescription.format,
                textureDescription.textureType,
                textureDescription.arraySize,
                textureDescription.writable,
                attrs));

        if (textureDescription.arraySize > 0) {
            binding += textureDescription.arraySize;
        } else {
            binding++;
        }
    }
}

void
HgiGLShaderGenerator::_WriteBuffers(
    const HgiShaderFunctionBufferDescVector &buffers)
{
    //Extract buffer descriptors and add appropriate buffer sections
    for(size_t i=0; i<buffers.size(); i++) {
        const HgiShaderFunctionBufferDesc &bufferDescription = buffers[i];

        const bool isUniformBufferBinding =
            (bufferDescription.binding == HgiBindingTypeUniformValue) ||
            (bufferDescription.binding == HgiBindingTypeUniformArray);

        const std::string arraySize =
            (bufferDescription.arraySize > 0)
                ? std::to_string(bufferDescription.arraySize)
                : std::string();

        if (isUniformBufferBinding) {
            const HgiShaderSectionAttributeVector attrs = {
                HgiShaderSectionAttribute{"std140", ""},
                HgiShaderSectionAttribute{"binding",
                    std::to_string(bufferDescription.bindIndex)}};

            CreateShaderSection<HgiGLBufferShaderSection>(
                bufferDescription.nameInShader,
                bufferDescription.bindIndex,
                bufferDescription.type,
                bufferDescription.binding,
                arraySize,
                attrs);
        } else {
            const HgiShaderSectionAttributeVector attrs = {
                HgiShaderSectionAttribute{"std430", ""},
                HgiShaderSectionAttribute{"binding",
                    std::to_string(bufferDescription.bindIndex)}};

            CreateShaderSection<HgiGLBufferShaderSection>(
                bufferDescription.nameInShader,
                bufferDescription.bindIndex,
                bufferDescription.type,
                bufferDescription.binding,
                arraySize,
                attrs);
        }
    }
}

void
HgiGLShaderGenerator::_WriteConstantParams(
    const HgiShaderFunctionParamDescVector &parameters)
{
    if (parameters.size() < 1) {
        return;
    }
    CreateShaderSection<HgiGLBlockShaderSection>(
        "ParamBuffer",
        parameters,
        0);
}

void
HgiGLShaderGenerator::_WriteInOuts(
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
    };

    const static std::unordered_map<std::string, std::string> takenInParams {
        { HgiShaderKeywordTokens->hdPosition, "gl_Position"},
        { HgiShaderKeywordTokens->hdPointCoord, "gl_PointCoord"},
        { HgiShaderKeywordTokens->hdClipDistance, "gl_ClipDistance"},
        { HgiShaderKeywordTokens->hdCullDistance, "gl_CullDistance"},
        { HgiShaderKeywordTokens->hdVertexID, "gl_VertexID"},
        { HgiShaderKeywordTokens->hdInstanceID, "gl_InstanceID"},
        { HgiShaderKeywordTokens->hdPrimitiveID, "gl_PrimitiveID"},
        { HgiShaderKeywordTokens->hdSampleID, "gl_SampleID"},
        { HgiShaderKeywordTokens->hdSamplePosition, "gl_SamplePosition"},
        { HgiShaderKeywordTokens->hdFragCoord, "gl_FragCoord"},
        { HgiShaderKeywordTokens->hdBaseVertex, "gl_BaseVertex"},
        { HgiShaderKeywordTokens->hdBaseInstance, "0"},
        { HgiShaderKeywordTokens->hdFrontFacing, "gl_FrontFacing"},
        { HgiShaderKeywordTokens->hdLayer, "gl_Layer"},
        { HgiShaderKeywordTokens->hdViewportIndex, "gl_ViewportIndex"},
        { HgiShaderKeywordTokens->hdGlobalInvocationID, "gl_GlobalInvocationID"},
        { HgiShaderKeywordTokens->hdBaryCoordNoPersp, "gl_BaryCoordNoPerspNV"},
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
                if (role == HgiShaderKeywordTokens->hdGlobalInvocationID ||
                    role == HgiShaderKeywordTokens->hdVertexID ||
                    role == HgiShaderKeywordTokens->hdInstanceID ||
                    role == HgiShaderKeywordTokens->hdBaseInstance ||
                    role == HgiShaderKeywordTokens->hdBaryCoordNoPersp) {
                    CreateShaderSection<HgiGLKeywordShaderSection>(
                        paramName,
                        param.type,
                        keyword->second);
                }
                continue;
            }
        }

        HgiShaderSectionAttributeVector attrs;

        // Currently, all interstage variables and blocks are matched by name
        const bool useInterstageSlot = false;

        if (param.location != -1) {
            // If a location has been specified then add it to the attributes.
            attrs.push_back({"location", std::to_string(param.location)});
        } else if (useInterstageSlot && (param.interstageSlot != -1)) {
            // For interstage parameters use the interstageSlot for location.
            attrs.push_back({"location", std::to_string(param.interstageSlot)});
        }

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
HgiGLShaderGenerator::_WriteInOutBlocks(
    const HgiShaderFunctionParamBlockDescVector &parameterBlocks,
    const std::string &qualifier)
{
    for(const HgiShaderFunctionParamBlockDesc &p : parameterBlocks) {

        HgiGLShaderSectionPtrVector members;
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
        }

        CreateShaderSection<HgiGLInterstageBlockShaderSection>(
            p.blockName,
            p.instanceName,
            qualifier,
            p.arraySize,
            members);
    }
}

void
HgiGLShaderGenerator::_Execute(std::ostream &ss)
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

    HgiGLShaderSectionUniquePtrVector* shaderSections = GetShaderSections();
    //For all shader sections, visit the areas defined for all
    //shader apis. We assume all shader apis have a global space
    //section, capabilities to define macros in global space,
    //and abilities to declare some members or functions there
    
    ss << "\n// //////// Global Includes ////////\n";
    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalIncludes(ss);
    }

    ss << "\n// //////// Global Macros ////////\n";
    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalMacros(ss);
    }

    ss << "\n// //////// Global Structs ////////\n";
    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalStructs(ss);
    }

    ss << "\n// //////// Global Member Declarations ////////\n";
    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalMemberDeclarations(ss);
    }

    ss << "\n// //////// Global Function Definitions ////////\n";
    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalFunctionDefinitions(ss);
    }

    ss << "\n";

    // write all the original shader
    ss << _GetShaderCode();
}

HgiGLShaderSectionUniquePtrVector*
HgiGLShaderGenerator::GetShaderSections()
{
    return &_shaderSections;
}

PXR_NAMESPACE_CLOSE_SCOPE
