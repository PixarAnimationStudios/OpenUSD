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

PXR_NAMESPACE_OPEN_SCOPE

static const std::string &
_GetMacroBlob()
{
    // Allows metal and GL to both handle out function params.
    // On the metal side, the ref(space,type) parameter defines
    // if items are in device or thread domain.
    const static std::string header =
        "#define REF(space,type) inout type\n"
        "#define HD_NEEDS_FORWARD_DECL\n"
        "#define HD_FWD_DECL(decl) decl\n"
        ;
    return header;
}

HgiGLShaderGenerator::HgiGLShaderGenerator(
    Hgi const *hgi,
    const HgiShaderFunctionDesc &descriptor)
  : HgiShaderGenerator(descriptor)
  , _hgi(hgi)
{
    // Write out all GL shaders and add to shader sections
    GetShaderSections()->push_back(
        std::make_unique<HgiGLMacroShaderSection>(
            _GetMacroBlob(), ""));

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
    }

    _WriteTextures(descriptor.textures);
    _WriteBuffers(descriptor.buffers);
    _WriteInOuts(descriptor.stageInputs, "in");
    _WriteConstantParams(descriptor.constantParams);
    _WriteInOuts(descriptor.stageOutputs, "out");
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

            GetShaderSections()->push_back(
                std::make_unique<HgiGLBufferShaderSection>(
                    bufferDescription.nameInShader,
                    bufferDescription.bindIndex,
                    bufferDescription.type,
                    bufferDescription.binding,
                    arraySize,
                    attrs));
        } else {
            const HgiShaderSectionAttributeVector attrs = {
                HgiShaderSectionAttribute{"std430", ""},
                HgiShaderSectionAttribute{"binding",
                    std::to_string(bufferDescription.bindIndex)}};

            GetShaderSections()->push_back(
                std::make_unique<HgiGLBufferShaderSection>(
                    bufferDescription.nameInShader,
                    bufferDescription.bindIndex,
                    bufferDescription.type,
                    bufferDescription.binding,
                    arraySize,
                    attrs));
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
    GetShaderSections()->push_back(
        std::make_unique<HgiGLBlockShaderSection>(
            "ParamBuffer",
            parameters,
            0));
}

void
HgiGLShaderGenerator::_WriteInOuts(
    const HgiShaderFunctionParamDescVector &parameters,
    const std::string &qualifier) 
{
    uint32_t counter = 0;

    //To unify glslfx across different apis, other apis
    //may want these to be defined, but since they are
    //taken in opengl we ignore them
    const static std::set<std::string> takenOutParams {
        "gl_Position",
        "gl_FragColor",
        "gl_FragDepth"
    };
    const static std::map<std::string, std::string> takenInParams {
        { HgiShaderKeywordTokens->hdPosition, "gl_Position"},
        { HgiShaderKeywordTokens->hdGlobalInvocationID, "gl_GlobalInvocationID"}
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
                GetShaderSections()->push_back(
                    std::make_unique<HgiGLKeywordShaderSection>(
                        paramName,
                        param.type,
                        keyword->second));
                continue;
            }
        }

        const HgiShaderSectionAttributeVector attrs {
            HgiShaderSectionAttribute{"location", std::to_string(counter)}
        };

        GetShaderSections()->push_back(
            std::make_unique<HgiGLMemberShaderSection>(
                paramName,
                param.type,
                attrs,
                qualifier));
        counter++;
    }
}

void
HgiGLShaderGenerator::_Execute(std::ostream &ss)
{
    // Version number must be first line in glsl shader
    _WriteVersion(ss);

    _WriteExtensions(ss);

    ss << _GetShaderCodeDeclarations() << "\n";

    for (const std::string &attr : _shaderLayoutAttributes) {
        ss << attr;
    }

    HgiGLShaderSectionUniquePtrVector* shaderSections = GetShaderSections();
    //For all shader sections, visit the areas defined for all
    //shader apis. We assume all shader apis have a global space
    //section, capabilities to define macros in global space,
    //and abilities to declare some members or functions there
    
    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalIncludes(ss);
        ss << "\n";
    }

    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalMacros(ss);
        ss << "\n";
    }

    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalStructs(ss);
        ss << "\n";
    }

    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalMemberDeclarations(ss);
        ss << "\n";
    }

    for (const std::unique_ptr<HgiGLShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalFunctionDefinitions(ss);
        ss << "\n";
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
