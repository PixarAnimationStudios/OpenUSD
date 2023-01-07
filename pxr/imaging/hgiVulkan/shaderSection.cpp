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
    const std::string &defaultValue,
    const std::string &arraySize,
    const std::string &blockInstanceIdentifier)
  : HgiShaderSection(identifier, attributes, defaultValue,
                     arraySize, blockInstanceIdentifier)
  , _storageQualifier(storageQualifier)
  , _arraySize(arraySize)
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
    WriteArraySize(ss);
    ss << ";\n";
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
    const HgiInterpolationType interpolation,
    const HgiSamplingType sampling,
    const HgiStorageType storage,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &storageQualifier,
    const std::string &defaultValue,
    const std::string &arraySize,
    const std::string &blockInstanceIdentifier)
  : HgiVulkanShaderSection(identifier,
                           attributes,
                           storageQualifier,
                           defaultValue,
                           arraySize,
                           blockInstanceIdentifier)
  , _typeName(typeName)
  , _interpolation(interpolation)
  , _sampling(sampling)
  , _storage(storage)
{
}

HgiVulkanMemberShaderSection::~HgiVulkanMemberShaderSection() = default;

bool
HgiVulkanMemberShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    if (HasBlockInstanceIdentifier()) {
        return true;
    }

    switch (_interpolation) {
    case HgiInterpolationDefault:
        break;
    case HgiInterpolationFlat:
        ss << "flat ";
        break;
    case HgiInterpolationNoPerspective:
        ss << "noperspective ";
        break;
    }
    switch (_sampling) {
    case HgiSamplingDefault:
        break;
    case HgiSamplingCentroid:
        ss << "centroid ";
        break;
    case HgiSamplingSample:
        ss << "sample ";
        break;
    }
    switch (_storage) {
    case HgiStorageDefault:
        break;
    case HgiStoragePatch:
        ss << "patch ";
        break;
    }
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
    ss << "\n};\n";
    return true;
}

const std::string HgiVulkanTextureShaderSection::_storageQualifier = "uniform";

HgiVulkanTextureShaderSection::HgiVulkanTextureShaderSection(
    const std::string &identifier,
    const unsigned int layoutIndex,
    const unsigned int dimensions,
    const HgiFormat format,
    const HgiShaderTextureType textureType,
    const uint32_t arraySize,
    const bool writable,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &defaultValue)
  : HgiVulkanShaderSection( identifier,
                        attributes,
                        _storageQualifier,
                        defaultValue,
                        arraySize > 0 ? 
                        "[" + std::to_string(arraySize) + "]" :
                        "")
  , _dimensions(dimensions)
  , _format(format)
  , _textureType(textureType)
  , _arraySize(arraySize)
  , _writable(writable)
{
}

HgiVulkanTextureShaderSection::~HgiVulkanTextureShaderSection() = default;

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
HgiVulkanTextureShaderSection::_WriteSamplerType(std::ostream &ss) const
{
    if (_writable) {
        if (_textureType == HgiShaderTextureTypeArrayTexture) {
            ss << "image" << _dimensions << "DArray";
        } else {
            ss << "image" << _dimensions << "D";
        }
    } else {
        if (_textureType == HgiShaderTextureTypeShadowTexture) {
            ss << _GetTextureTypePrefix(_format) << "sampler"
               << _dimensions << "DShadow";
        } else if (_textureType == HgiShaderTextureTypeArrayTexture) {
            ss << _GetTextureTypePrefix(_format) << "sampler" 
               << _dimensions << "DArray";
        } else {
            ss << _GetTextureTypePrefix(_format) << "sampler" 
               << _dimensions << "D";
        }
    }
}

void
HgiVulkanTextureShaderSection::_WriteSampledDataType(std::ostream &ss) const
{
    if (_textureType == HgiShaderTextureTypeShadowTexture) {
        ss << "float";
    } else {
        ss << _GetTextureTypePrefix(_format) << "vec4";
    }
}

void
HgiVulkanTextureShaderSection::WriteType(std::ostream &ss) const
{
    if(_dimensions < 1 || _dimensions > 3) {
        TF_CODING_ERROR("Invalid texture dimension");
    }
    _WriteSamplerType(ss); // e.g. sampler<N>D, isampler<N>D, usampler<N>D
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
    // Used to unify texture sampling and writing across platforms that depend 
    // on samplers and don't store textures in global space.
    const uint32_t sizeDim = 
        (_textureType == HgiShaderTextureTypeArrayTexture) ? 
        (_dimensions + 1) : _dimensions;
    const uint32_t coordDim = 
        (_textureType == HgiShaderTextureTypeShadowTexture ||
         _textureType == HgiShaderTextureTypeArrayTexture) ? 
        (_dimensions + 1) : _dimensions;

    const std::string sizeType = sizeDim == 1 ? 
        "int" :
        "ivec" + std::to_string(sizeDim);
    const std::string intCoordType = coordDim == 1 ? 
        "int" :
        "ivec" + std::to_string(coordDim);
    const std::string floatCoordType = coordDim == 1 ? 
        "float" :
        "vec" + std::to_string(coordDim);

    if (_arraySize > 0) {
        WriteType(ss);
        ss << " HgiGetSampler_";
        WriteIdentifier(ss);
        ss << "(uint index) {\n";
        ss << "    return ";
        WriteIdentifier(ss);
        ss << "[index];\n}\n";
    } else {
        ss << "#define HgiGetSampler_";
        WriteIdentifier(ss);
        ss << "() ";
        WriteIdentifier(ss);
        ss << "\n";
    }

    if (_writable) {
        // Write a function that lets you write to the texture with 
        // HgiSet_texName(uv, data).
        ss << "void HgiSet_";
        WriteIdentifier(ss);
        ss << "(" << intCoordType << " uv, vec4 data) {\n";
        ss << "    ";
        ss << "imageStore(";
        WriteIdentifier(ss);
        ss << ", uv, data);\n";
        ss << "}\n";

        // HgiGetSize_texName()
        ss << sizeType << " HgiGetSize_";
        WriteIdentifier(ss);
        ss << "() {\n";
        ss << "    ";
        ss << "return imageSize(";
        WriteIdentifier(ss);
        ss << ");\n";
        ss << "}\n";
    } else {
        const std::string arrayInput = (_arraySize > 0) ? "uint index, " : "";
        const std::string arrayIndex = (_arraySize > 0) ? "[index]" : "";
        
        // Write a function that lets you query the texture with 
        // HgiGet_texName(uv).
        _WriteSampledDataType(ss); // e.g., vec4, ivec4, uvec4
        ss << " HgiGet_";
        WriteIdentifier(ss);
        ss << "(" << arrayInput << floatCoordType << " uv) {\n";
        ss << "    ";
        _WriteSampledDataType(ss);
        ss << " result = texture(";
        WriteIdentifier(ss);
        ss << arrayIndex << ", uv);\n";
        ss << "    return result;\n";
        ss << "}\n";
        
        // HgiGetSize_texName()
        ss << sizeType << " HgiGetSize_";
        WriteIdentifier(ss);
        ss << "(" << ((_arraySize > 0) ? "uint index" : "")  << ") {\n";
        ss << "    ";
        ss << "return textureSize(";
        WriteIdentifier(ss);
        ss << arrayIndex << ", 0);\n";
        ss << "}\n";

        // HgiTextureLod_texName()
        _WriteSampledDataType(ss);
        ss << " HgiTextureLod_";
        WriteIdentifier(ss);
        ss << "(" << arrayInput << floatCoordType << " coord, float lod) {\n";
        ss << "    ";
        ss << "return textureLod(";
        WriteIdentifier(ss);
        ss << arrayIndex << ", coord, lod);\n";
        ss << "}\n";
        
        // HgiTexelFetch_texName()
        if (_textureType != HgiShaderTextureTypeShadowTexture) {
            _WriteSampledDataType(ss);
            ss << " HgiTexelFetch_";
            WriteIdentifier(ss);
            ss << "(" << arrayInput << intCoordType << " coord) {\n";
            ss << "    ";
            _WriteSampledDataType(ss);
            ss << " result = texelFetch(";
            WriteIdentifier(ss);
            ss << arrayIndex << ", coord, 0);\n";
            ss << "    return result;\n";
            ss << "}\n";
        }
    }

    return true;
}

HgiVulkanBufferShaderSection::HgiVulkanBufferShaderSection(
    const std::string &identifier,
    const uint32_t layoutIndex,
    const std::string &type,
    const HgiBindingType binding,
    const std::string arraySize,
    const bool writable,
    const HgiShaderSectionAttributeVector &attributes)
  : HgiVulkanShaderSection( identifier,
                            attributes,
                            "buffer",
                            "")
  , _type(type)
  , _binding(binding)
  , _arraySize(arraySize)
  , _writable(writable)
{
}

HgiVulkanBufferShaderSection::~HgiVulkanBufferShaderSection() = default;

void
HgiVulkanBufferShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}

bool
HgiVulkanBufferShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
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

    // If it has a storage qualifier, declare it
    if (_binding == HgiBindingTypeUniformValue ||
        _binding == HgiBindingTypeUniformArray) {
        ss << "uniform ubo_";
    } else {
        if (!_writable) {
            ss << "readonly ";
        }
        ss << "buffer ssbo_";
    }
    WriteIdentifier(ss);
    ss << " { ";
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);

    if (_binding == HgiBindingTypeValue ||
        _binding == HgiBindingTypeUniformValue) {
        ss << "; };\n";
    } else {
        ss << "[" << _arraySize << "]; };\n";
    }

    return true;
}

HgiVulkanKeywordShaderSection::HgiVulkanKeywordShaderSection(
    const std::string &identifier,
    const std::string &type,
    const std::string &keyword)
  : HgiVulkanShaderSection(identifier)
  , _type(type)
  , _keyword(keyword)
{
}

HgiVulkanKeywordShaderSection::~HgiVulkanKeywordShaderSection() = default;

void
HgiVulkanKeywordShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}

bool
HgiVulkanKeywordShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << " = ";
    ss << _keyword;
    ss << ";\n";

    return true;
}

HgiVulkanInterstageBlockShaderSection::HgiVulkanInterstageBlockShaderSection(
    const std::string &blockIdentifier,
    const std::string &blockInstanceIdentifier,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &qualifier,
    const std::string &arraySize,
    const HgiVulkanShaderSectionPtrVector &members)
    : HgiVulkanShaderSection(blockIdentifier,
                             attributes,
                             qualifier,
                             std::string(),
                             arraySize,
                             blockInstanceIdentifier)
    , _qualifier(qualifier)
    , _members(members)
{
}

bool
HgiVulkanInterstageBlockShaderSection::VisitGlobalMemberDeclarations(
    std::ostream &ss)
{
    // If it has attributes, write them with corresponding layout
    // identifiers and indices
    const HgiShaderSectionAttributeVector &attributes = GetAttributes();

    if (!attributes.empty()) {
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

    ss << _qualifier << " ";
    WriteIdentifier(ss);
    ss << " {\n";
    for (const HgiVulkanShaderSection* member : _members) {
        ss << "  ";
        member->WriteType(ss);
        ss << " ";
        member->WriteIdentifier(ss);
        ss << ";\n";
    }
    ss << "} ";
    WriteBlockInstanceIdentifier(ss);
    WriteArraySize(ss);
    ss << ";\n";
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
