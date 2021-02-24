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

#include "pxr/imaging/hgiVulkan/shaderGenerator.h"

PXR_NAMESPACE_OPEN_SCOPE

static const std::string &
_GetMacroBlob()
{
    // Allows metal and GL to both handle out function params.
    // On the metal side, the ref(space,type) parameter defines
    // if items are in device or thread domain.
    const static std::string header = R"(#define REF(space,type) inout type)";
    return header;
}

HgiVulkanShaderGenerator::HgiVulkanShaderGenerator(
    const HgiShaderFunctionDesc &descriptor)
  : HgiShaderGenerator(descriptor)
  , _bindIndex(0)
{
    //Write out all GL shaders and add to shader sections
    GetShaderSections()->push_back(
        std::make_unique<HgiVulkanMacroShaderSection>(_GetMacroBlob(), ""));

    // The ordering here is important (buffers before textures), because we
    // need to increment the bind location for resources in the same order
    // as HgiVulkanResourceBindings.
    // In Vulkan buffers and textures cannot have the same binding index.
    _WriteConstantParams(descriptor.constantParams);
    _WriteTextures(descriptor.textures);
    /*XXX _WriteBuffers goes here XXX */
    _WriteInOuts(descriptor.stageInputs, "in");
    _WriteInOuts(descriptor.stageOutputs, "out");
}

void
HgiVulkanShaderGenerator::_WriteConstantParams(
    const HgiShaderFunctionParamDescVector &parameters)
{
    if (parameters.empty()) {
        return;
    }
    GetShaderSections()->push_back(
        std::make_unique<HgiVulkanBlockShaderSection>(
            "ParamBuffer",
            parameters));
}

void
HgiVulkanShaderGenerator::_WriteTextures(
    const HgiShaderFunctionTextureDescVector& textures)
{
    for(const HgiShaderFunctionTextureDesc& desc : textures) {
        const HgiShaderSectionAttributeVector attrs = {
            HgiShaderSectionAttribute{
                "binding",
                std::to_string(_bindIndex)}};

        GetShaderSections()->push_back(
            std::make_unique<HgiVulkanTextureShaderSection>(
                desc.nameInShader,
                _bindIndex,
                desc.dimensions,
                attrs));

        // In Vulkan buffers and textures cannot have the same binding index.
        _bindIndex++;
    }
}

void
HgiVulkanShaderGenerator::_WriteInOuts(
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
        "gl_FragDepth"};
    const static std::set<std::string> takenInParams {
        "hd_Position"};

    for(const HgiShaderFunctionParamDesc &param : parameters) {
        //Skip writing out taken parameter names
        const std::string &paramName = param.nameInShader;
        if(qualifier == "out" && takenOutParams.count(paramName) > 0) {
            continue;
        }
        if(qualifier == "in" && takenInParams.count(paramName) > 0) {
            continue;
        }

        const HgiShaderSectionAttributeVector attrs {
            HgiShaderSectionAttribute{
                "location", std::to_string(counter) }
        };

        GetShaderSections()->push_back(
            std::make_unique<HgiVulkanMemberShaderSection>(
                paramName,
                param.type,
                attrs,
                qualifier));
        counter++;
    }
}

void
HgiVulkanShaderGenerator::_Execute(
    std::ostream &ss,
    const std::string &originalShaderShader) 
{
    ss << _GetVersion() << " \n";

    HgiVulkanShaderSectionUniquePtrVector* shaderSections = GetShaderSections();
    //For all shader sections, visit the areas defined for all
    //shader apis. We assume all shader apis have a global space
    //section, capabilities to define macros in global space,
    //and abilities to declare some members or functions there
    
    for (const std::unique_ptr<HgiVulkanShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalIncludes(ss);
        ss << "\n";
    }

    for (const std::unique_ptr<HgiVulkanShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalMacros(ss);
        ss << "\n";
    }

    for (const std::unique_ptr<HgiVulkanShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalStructs(ss);
        ss << "\n";
    }

    for (const std::unique_ptr<HgiVulkanShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalMemberDeclarations(ss);
        ss << "\n";
    }

    for (const std::unique_ptr<HgiVulkanShaderSection>
            &shaderSection : *shaderSections) {
        shaderSection->VisitGlobalFunctionDefinitions(ss);
        ss << "\n";
    }

    ss << "\n";
    const char* cstr = originalShaderShader.c_str();

    //write all the original shader except the version string
    ss.write(
        cstr + _GetVersion().length(),
        originalShaderShader.length() - _GetVersion().length());
}

HgiVulkanShaderSectionUniquePtrVector*
HgiVulkanShaderGenerator::GetShaderSections()
{
    return &_shaderSections;
}

PXR_NAMESPACE_CLOSE_SCOPE
