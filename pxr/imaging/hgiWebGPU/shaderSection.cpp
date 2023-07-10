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

#include "pxr/imaging/hgiWebGPU/shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

const uint32_t HgiWebGPUSamplerShaderSection::bindingSet = 2;
const std::string HgiWebGPUSamplerShaderSection::_storageQualifier = "uniform";

HgiWebGPUSamplerShaderSection::HgiWebGPUSamplerShaderSection(
        const std::string &textureSharedIdentifier,
        const uint32_t arrayOfSamplersSize,
        const HgiShaderSectionAttributeVector &attributes)
        : HgiWebGPUShaderSection(
        "samplerBind_" + textureSharedIdentifier,
        attributes,
        _storageQualifier,
        "", // defaultValue
        arrayOfSamplersSize > 0 ?
        "[" + std::to_string(arrayOfSamplersSize) + "]" :
        "")
        , _textureSharedIdentifier(textureSharedIdentifier)
{
}

HgiWebGPUSamplerShaderSection::~HgiWebGPUSamplerShaderSection() = default;

void
HgiWebGPUSamplerShaderSection::WriteType(std::ostream& ss) const
{
    ss << "sampler";
}

bool
HgiWebGPUSamplerShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteDeclaration(ss);
    return true;
}

bool
HgiWebGPUSamplerShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
{
    return true;
}

const uint32_t HgiWebGPUTextureShaderSection::bindingSet = 1;
const std::string HgiWebGPUTextureShaderSection::_storageQualifier = "uniform";

HgiWebGPUTextureShaderSection::HgiWebGPUTextureShaderSection(
    const std::string &identifier,
    const HgiWebGPUSamplerShaderSection *samplerShaderSectionDependency,
    const unsigned int dimensions,
    const HgiFormat format,
    const HgiShaderTextureType textureType,
    const uint32_t arraySize,
    const bool writable,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &defaultValue)
  : HgiWebGPUShaderSection( "textureBind_" + identifier,
                        attributes,
                        _storageQualifier,
                        defaultValue,
                        arraySize > 0 ? 
                        "[" + std::to_string(arraySize) + "]" :
                        "")
  , _samplerSharedIdentifier(identifier)
  , _dimensions(dimensions)
  , _format(format)
  , _samplerShaderSectionDependency(samplerShaderSectionDependency)
  , _textureType(textureType)
  , _arraySize(arraySize)
  , _writable(writable)
{
}

HgiWebGPUTextureShaderSection::~HgiWebGPUTextureShaderSection() = default;

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
HgiWebGPUTextureShaderSection::_WriteTextureType(std::ostream &ss) const
{
    if (_writable) {
        if (_textureType == HgiShaderTextureTypeArrayTexture) {
            TF_CODING_ERROR("Missing Implementation of writable HgiShaderTextureTypeArrayTexture");
        } else {
            TF_CODING_ERROR("Missing Implementation of writable HgiShaderTexture");
        }
    } else {
        if (_textureType == HgiShaderTextureTypeShadowTexture) {
            TF_CODING_ERROR("Missing Implementation of HgiShaderTextureTypeShadowTexture");
        } else if (_textureType == HgiShaderTextureTypeArrayTexture) {
            TF_CODING_ERROR("Missing Implementation of HgiShaderTextureTypeArrayTexture");
        } else {
            ss << "texture"
               << _dimensions << "D";
        }
    }
}

void
HgiWebGPUTextureShaderSection::_WriteSampledDataType(std::ostream &ss) const
{
    if (_textureType == HgiShaderTextureTypeShadowTexture) {
        ss << "float";
    } else {
        ss << _GetTextureTypePrefix(_format) << "vec4";
    }
}

void
HgiWebGPUTextureShaderSection::WriteType(std::ostream &ss) const
{
    if(_dimensions < 1 || _dimensions > 3) {
        TF_CODING_ERROR("Invalid texture dimension");
    }
    _WriteTextureType(ss); // e.g. texture<N>D, itexture<N>D, utexture<N>D
}

bool
HgiWebGPUTextureShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteDeclaration(ss);
    return true;
}

bool
HgiWebGPUTextureShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
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
        ss << " HgiGetSampler_" << _samplerSharedIdentifier;
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
        TF_CODING_ERROR("Missing Implementation of writable globalFunction for TextureShaderSection");
    } else {
        const std::string arrayInput = (_arraySize > 0) ? "uint index, " : "";
        const std::string arrayIndex = (_arraySize > 0) ? "[index]" : "";
        
        // Write a function that lets you query the texture with 
        // HgiGet_texName(uv).
        _WriteSampledDataType(ss); // e.g., vec4, ivec4, uvec4
        ss << " HgiGet_";
        ss << _samplerSharedIdentifier;
        ss << "(" << arrayInput << floatCoordType << " uv) {\n";
        ss << "    ";
        _WriteSampledDataType(ss);
        ss << " result = texture(sampler" << _dimensions << "D(";
        WriteIdentifier(ss);
        ss << ", ";
        _samplerShaderSectionDependency->WriteIdentifier(ss);
        ss << ")";
        ss << arrayIndex << ", uv);\n";
        ss << "    return result;\n";
        ss << "}\n";
        
        // HgiGetSize_texName()
        ss << sizeType << " HgiGetSize_";
        ss << _samplerSharedIdentifier;
        ss << "(" << ((_arraySize > 0) ? "uint index" : "")  << ") {\n";
        ss << "    ";
        ss << "return textureSize(sampler" << _dimensions << "D(";
        WriteIdentifier(ss);
        ss << ", ";
        _samplerShaderSectionDependency->WriteIdentifier(ss);
        ss << ")";
        ss << arrayIndex << ", 0);\n";
        ss << "}\n";

        // HgiTextureLod_texName()
        _WriteSampledDataType(ss);
        ss << " HgiTextureLod_";
        ss << _samplerSharedIdentifier;
        ss << "(" << arrayInput << floatCoordType << " coord, float lod) {\n";
        ss << "    ";
        ss << "return textureLod(sampler" << _dimensions << "D(";
        WriteIdentifier(ss);
        ss << ", ";
        _samplerShaderSectionDependency->WriteIdentifier(ss);
        ss << ")";
        ss << arrayIndex << ", coord, lod);\n";
        ss << "}\n";
        
        // HgiTexelFetch_texName()
        if (_textureType != HgiShaderTextureTypeShadowTexture) {
            _WriteSampledDataType(ss);
            ss << " HgiTexelFetch_";
            ss << _samplerSharedIdentifier;
            ss << "(" << arrayInput << intCoordType << " coord) {\n";
            ss << "    ";
            _WriteSampledDataType(ss);
            ss << " result = texelFetch(sampler" << _dimensions << "D(";
            WriteIdentifier(ss);
            ss << ", ";
            _samplerShaderSectionDependency->WriteIdentifier(ss);
            ss << ")";
            ss << arrayIndex << ", coord, 0);\n";
            ss << "    return result;\n";
            ss << "}\n";
        }
    }

    return true;
}

const uint32_t HgiWebGPUBufferShaderSection::bindingSet = 0;
HgiWebGPUBufferShaderSection::HgiWebGPUBufferShaderSection(
    const std::string &identifier,
    const bool writable,
    const std::string &type,
    const HgiBindingType binding,
    const std::string arraySize,
    const HgiShaderSectionAttributeVector &attributes)
  : HgiWebGPUShaderSection( identifier,
                        attributes,
                        writable ? "buffer" : "readonly buffer",
                        "")
  , _type(type)
  , _binding(binding)
  , _arraySize(arraySize)
{
}

HgiWebGPUBufferShaderSection::~HgiWebGPUBufferShaderSection() = default;

void
HgiWebGPUBufferShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}

bool
HgiWebGPUBufferShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
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
    if (_binding == HgiBindingTypeUniformValue ||
        _binding == HgiBindingTypeUniformArray) {
        ss << "uniform ubo_";
    } else {
        ss << _storageQualifier << " ssbo_";
    }
    WriteIdentifier(ss);
    ss << " { ";
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);

    if (_binding == HgiBindingTypeValue ||
        _binding == HgiBindingTypeUniformValue) {
        ss << "; };\n";
    }
    else {
        ss << "[" << _arraySize << "]; };\n";
    }

    return true;
}

HgiWebGPUInterstageBlockShaderSection::HgiWebGPUInterstageBlockShaderSection(
    const std::string &blockIdentifier,
    const std::string &blockInstanceIdentifier,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &qualifier,
    const std::string &arraySize,
    const HgiBaseGLShaderSectionPtrVector &members)
    : HgiWebGPUShaderSection(blockIdentifier,
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
HgiWebGPUInterstageBlockShaderSection::VisitGlobalMemberDeclarations(
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
    for (const HgiBaseGLShaderSection* member : _members) {
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
