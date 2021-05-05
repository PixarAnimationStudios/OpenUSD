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

#include "pxr/imaging/hgiGL/shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLShaderSection::HgiGLShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &storageQualifier,
    const std::string &defaultValue)
  : HgiShaderSection(identifier, attributes, defaultValue)
  , _storageQualifier(storageQualifier)
{
}

HgiGLShaderSection::~HgiGLShaderSection() = default;

void
HgiGLShaderSection::WriteDeclaration(std::ostream &ss) const
{
    //If it has attributes, write them with corresponding layout
    //identifiers and indicies
    const HgiShaderSectionAttributeVector &attributes = GetAttributes();

    if(!attributes.empty()) {
        ss << "layout(";
        for (size_t i = 0; i < attributes.size(); i++)
        {
            if (i > 0) {
                ss << ", ";
            }
            const HgiShaderSectionAttribute &a = attributes[i];
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
HgiGLShaderSection::WriteParameter(std::ostream &ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << ";";
}

bool
HgiGLShaderSection::VisitGlobalIncludes(std::ostream &ss)
{
    return false;
}

bool
HgiGLShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    return false;
}

bool
HgiGLShaderSection::VisitGlobalStructs(std::ostream &ss)
{
    return false;
}

bool HgiGLShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    return false;
}

bool
HgiGLShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
{
    return false;
}

HgiGLMacroShaderSection::HgiGLMacroShaderSection(
    const std::string &macroDeclaration,
    const std::string &macroComment)
  : HgiGLShaderSection(macroDeclaration)
  , _macroComment(macroComment)
{
}

HgiGLMacroShaderSection::~HgiGLMacroShaderSection() = default;

bool
HgiGLMacroShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    WriteIdentifier(ss);
    return true;
}

HgiGLMemberShaderSection::HgiGLMemberShaderSection(
    const std::string &identifier,
    const std::string &typeName,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &storageQualifier,
    const std::string &defaultValue)
  : HgiGLShaderSection(identifier,
                       attributes,
                       storageQualifier,
                       defaultValue)
  , _typeName(typeName)
{
}

HgiGLMemberShaderSection::~HgiGLMemberShaderSection() = default;

bool
HgiGLMemberShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteDeclaration(ss);
    return true;
}

void
HgiGLMemberShaderSection::WriteType(std::ostream& ss) const
{
    ss << _typeName;
}

HgiGLBlockShaderSection::HgiGLBlockShaderSection(
    const std::string &identifier,
    const HgiShaderFunctionParamDescVector &parameters,
    const unsigned int bindingNo)
  : HgiGLShaderSection(identifier)
  , _parameters(parameters)
  , _bindingNo(bindingNo)
{
}

HgiGLBlockShaderSection::~HgiGLBlockShaderSection() = default;

bool
HgiGLBlockShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    ss << "layout(std140, binding = "
        << _bindingNo << ") " << "uniform" << " ";
    WriteIdentifier(ss);
    ss << "\n";
    ss << "{\n";
    for(const HgiShaderFunctionParamDesc &param : _parameters) {
        ss << "        " << param.type << " " << param.nameInShader << ";\n";
    }
    ss << "\n};";
    return true;
}

const std::string HgiGLTextureShaderSection::_storageQualifier = "uniform";

HgiGLTextureShaderSection::HgiGLTextureShaderSection(
    const std::string &identifier,
    const unsigned int layoutIndex,
    const unsigned int dimensions,
    const HgiFormat format,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &defaultValue)
  : HgiGLShaderSection( identifier,
                        attributes,
                        _storageQualifier,
                        defaultValue)
  , _dimensions(dimensions)
  , _format(format)
{
}

HgiGLTextureShaderSection::~HgiGLTextureShaderSection() = default;

static std::string
_GetTextureTypePrefix(HgiFormat const &format)
{
    if (format >= HgiFormatUInt16 && format <= HgiFormatUInt16Vec4) {
        return "u"; // e.g., usampler, uvec4
    }
    if (format >= HgiFormatInt32 && format <= HgiFormatInt32Vec4) {
        return "i"; // e.g., isampler, ivec4
    }
    return ""; // e.g., sampler, vec4
}

void
HgiGLTextureShaderSection::_WriteSamplerType(std::ostream &ss) const
{
    ss << _GetTextureTypePrefix(_format) << "sampler" << _dimensions << "D";
}

void
HgiGLTextureShaderSection::_WriteSampledDataType(std::ostream &ss) const
{
    ss << _GetTextureTypePrefix(_format) << "vec4";
}

void
HgiGLTextureShaderSection::WriteType(std::ostream &ss) const
{
    if(_dimensions < 1 || _dimensions > 3) {
        TF_CODING_ERROR("Invalid texture dimension");
    }
    _WriteSamplerType(ss); // e.g. sampler<N>D, isampler<N>D, usampler<N>D
}

bool
HgiGLTextureShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteDeclaration(ss);
    return true;
}

bool
HgiGLTextureShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
{
    //Write a function that let's you query the texture with HdGet_texName(uv)
    //Used to unify texture sampling across platforms that depend on samplers
    //and don't store textures in global space
    _WriteSampledDataType(ss); // e.g., vec4, ivec4, uvec4
    ss << " HdGet_";
    WriteIdentifier(ss);
    ss << "(vec" << _dimensions
             << " uv) {\n";
    ss << "    ";
    _WriteSampledDataType(ss);
    ss << " result = texture(";
    WriteIdentifier(ss);
    ss << ", uv);\n";
    ss << "    return result;\n";
    ss << "}";

    //Same except for texelfetch
    if(_dimensions != 2) {
        return true;
    }
    
    _WriteSampledDataType(ss);
    ss << " HdTexelFetch_";
    WriteIdentifier(ss);
    ss << "(ivec2 coord) {\n";
    ss << "    ";
    _WriteSampledDataType(ss);
    ss << " result = texelFetch(";
    WriteIdentifier(ss);
    ss << ", coord, 0);\n";
    ss << "    return result;\n";
    ss << "}\n";

    return true;
}

HgiGLBufferShaderSection::HgiGLBufferShaderSection(
    const std::string &identifier,
    const uint32_t layoutIndex,
    const std::string &type,
    const HgiShaderSectionAttributeVector &attributes)
  : HgiGLShaderSection( identifier,
                        attributes,
                        "buffer",
                        "")
  , _type(type)
{
}

HgiGLBufferShaderSection::~HgiGLBufferShaderSection() = default;

void
HgiGLBufferShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}

bool
HgiGLBufferShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    //If it has attributes, write them with corresponding layout
    //identifiers and indicies
    const HgiShaderSectionAttributeVector &attributes = GetAttributes();

    if(!attributes.empty()) {
        ss << "layout(";
        for (size_t i = 0; i < attributes.size(); i++)
        {
            if (i > 0) {
                ss << ", ";
            }
            const HgiShaderSectionAttribute &a = attributes[i];
            ss << a.identifier;
            if(!a.index.empty()) {
                ss << " = " << a.index;
            }
        }
        ss << ") ";
    }
    //If it has a storage qualifier, declare it
    ss << " buffer _";
    WriteIdentifier(ss);
    ss << " { ";
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << "[]; };";

    return true;
}

HgiGLKeywordShaderSection::HgiGLKeywordShaderSection(
    const std::string &identifier,
    const std::string &type,
    const std::string &keyword)
  : HgiGLShaderSection(identifier)
  , _type(type)
  , _keyword(keyword)
{
}

HgiGLKeywordShaderSection::~HgiGLKeywordShaderSection() = default;

void
HgiGLKeywordShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}

bool
HgiGLKeywordShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << " = ";
    ss << _keyword;
    ss << ";";

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
