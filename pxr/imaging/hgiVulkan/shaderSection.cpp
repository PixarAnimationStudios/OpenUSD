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

#include "pxr/imaging/hgiVulkan/shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiVulkanShaderSection::HgiVulkanShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &storageQualifier,
    const std::string &defaultValue)
  : HgiShaderSection(identifier, attributes, defaultValue)
  , _storageQualifier(storageQualifier)
{
}

HgiVulkanShaderSection::~HgiVulkanShaderSection() = default;

void
HgiVulkanShaderSection::WriteDeclaration(std::ostream &ss) const
{
    //If it has attributes, write them with corresponding layout
    //identifiers and indicies
    const HgiShaderSectionAttributeVector &attributes = GetAttributes();

    if(!attributes.empty()) {
        ss << "layout(";
        for (size_t i = 0; i < attributes.size(); ++i) {
            const HgiShaderSectionAttribute &a = attributes[i];
            if (i > 0) {
                ss << ", ";
            }
            ss << a.identifier;
            if(!a.index.empty()) {
                ss << " = " << a.index;
            }
        }
        ss << ") ";
    }
    //If it has a storage qualifier, declare it
    if(!_storageQualifier.empty()) {
        ss << _storageQualifier << " ";
    }
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << ";";
}

void
HgiVulkanShaderSection::WriteParameter(std::ostream &ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << ";";
}

bool
HgiVulkanShaderSection::VisitGlobalIncludes(std::ostream &ss)
{
    return false;
}

bool
HgiVulkanShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    return false;
}

bool
HgiVulkanShaderSection::VisitGlobalStructs(std::ostream &ss)
{
    return false;
}

bool HgiVulkanShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    return false;
}

bool
HgiVulkanShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
{
    return false;
}

HgiVulkanMacroShaderSection::HgiVulkanMacroShaderSection(
    const std::string &macroDeclaration,
    const std::string &macroComment)
  : HgiVulkanShaderSection(macroDeclaration)
  , _macroComment(macroComment)
{
}

HgiVulkanMacroShaderSection::~HgiVulkanMacroShaderSection() = default;

bool
HgiVulkanMacroShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    WriteIdentifier(ss);
    return true;
}

HgiVulkanMemberShaderSection::HgiVulkanMemberShaderSection(
    const std::string &identifier,
    const std::string &typeName,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &storageQualifier,
    const std::string &defaultValue)
  : HgiVulkanShaderSection(identifier,
                           attributes,
                           storageQualifier,
                           defaultValue)
  , _typeName(typeName)
{
}

HgiVulkanMemberShaderSection::~HgiVulkanMemberShaderSection() = default;

bool
HgiVulkanMemberShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteDeclaration(ss);
    return true;
}

void
HgiVulkanMemberShaderSection::WriteType(std::ostream& ss) const
{
    ss << _typeName;
}

HgiVulkanBlockShaderSection::HgiVulkanBlockShaderSection(
    const std::string &identifier,
    const HgiShaderFunctionParamDescVector &parameters)
  : HgiVulkanShaderSection(identifier)
  , _parameters(parameters)
{
}

HgiVulkanBlockShaderSection::~HgiVulkanBlockShaderSection() = default;

bool
HgiVulkanBlockShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    ss << "layout(push_constant) " << "uniform" << " ";
    WriteIdentifier(ss);
    ss << "\n";
    ss << "{\n";
    for(const HgiShaderFunctionParamDesc &param : _parameters) {
        ss << "    " << param.type << " " << param.nameInShader << ";\n";
    }
    ss << "\n};";
    return true;
}

HgiVulkanTextureShaderSection::HgiVulkanTextureShaderSection(
    const std::string &identifier,
    const unsigned int layoutIndex,
    const unsigned int dimensions,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &defaultValue)
  : HgiVulkanShaderSection( identifier,
                            attributes,
                            "uniform",
                            defaultValue)
  , _dimensions(dimensions)
{
}

HgiVulkanTextureShaderSection::~HgiVulkanTextureShaderSection() = default;

void
HgiVulkanTextureShaderSection::WriteType(std::ostream &ss) const
{
    if(_dimensions < 1 || _dimensions > 3) {
        TF_CODING_ERROR("Invalid texture dimension");
    }
    ss << "sampler" << _dimensions << "D";
}

bool
HgiVulkanTextureShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteDeclaration(ss);
    return true;
}

bool
HgiVulkanTextureShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
{
    //Write a function that let's you query the texture with HDGet_texName(uv)
    //Used to unify texture sampling across platforms that depend on samplers
    //and don't store textures in global space
    ss << "vec4 HdGet_";
    WriteIdentifier(ss);
    ss << "(vec" << _dimensions
             << " uv) {\n";
    ss << "    vec4 result = texture(";
    WriteIdentifier(ss);
    ss << ", uv);\n";
    ss << "    return result;\n";
    ss << "}";

    //Same except for texelfetch
    if(_dimensions != 2) {
        return true;
    }
    
    ss << "vec4 HdTexelFetch_";
    WriteIdentifier(ss);
    ss << "(ivec2 coord) {\n";
    ss << "    vec4 result = texelFetch(";
    WriteIdentifier(ss);
    ss << ", coord, 0);\n";
    ss << "    return result;\n";
    ss << "}\n";

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
