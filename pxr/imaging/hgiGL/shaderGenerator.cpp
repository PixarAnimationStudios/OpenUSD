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

#include "pxr/imaging/hgiGL/shaderGenerator.h"

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

HgiGLShaderGenerator::HgiGLShaderGenerator(
    const HgiShaderFunctionDesc &descriptor)
  : HgiShaderGenerator(descriptor)
{
    //Write out all GL shaders and add to shader sections
    GetShaderSections()->push_back(
        std::make_unique<HgiGLMacroShaderSection>(
            _GetMacroBlob(), ""));

    _WriteTextures(descriptor.textures);
    _WriteInOuts(descriptor.stageInputs, "in");
    _WriteConstantParams(descriptor.constantParams);
    _WriteInOuts(descriptor.stageOutputs, "out");
}

void
HgiGLShaderGenerator::_WriteTextures(
    const HgiShaderFunctionTextureDescVector &textures)
{
    //Extract texture descriptors and add appropriate texture sections
    for(size_t i=0; i<textures.size(); i++) {
        const HgiShaderFunctionTextureDesc &textureDescription = textures[i];
        const HgiShaderSectionAttributeVector attrs = {
            HgiShaderSectionAttribute{"binding", std::to_string(i)}};

        GetShaderSections()->push_back(
            std::make_unique<HgiGLTextureShaderSection>(
                textureDescription.nameInShader,
                i,
                textureDescription.dimensions,
                attrs));
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
        "gl_FragDepth"};
    const static std::set<std::string> takenInParams {
        "hd_Position"};

    for(const HgiShaderFunctionParamDesc &param : parameters) {
        //Skip writing out taken parameter names
        std::string paramName = param.nameInShader;
        if(qualifier == "out" &&
                takenOutParams.find(param.nameInShader) != takenOutParams.end()) {
            continue;
        }
        if(qualifier == "in" &&
                takenInParams.find(param.nameInShader) != takenInParams.end()) {
            continue;
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
HgiGLShaderGenerator::_Execute(
    std::ostream &ss,
    const std::string &originalShaderShader) 
{
    ss << _GetVersion() << " \n";

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
    const char* cstr = originalShaderShader.c_str();

    //write all the original shader except the version string
    ss.write(
        cstr + _GetVersion().length(),
        originalShaderShader.length() - _GetVersion().length());
}

HgiGLShaderSectionUniquePtrVector*
HgiGLShaderGenerator::GetShaderSections()
{
    return &_shaderSections;
}

PXR_NAMESPACE_CLOSE_SCOPE
