//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiWebGPU/shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiWebGPUShaderSection::HgiWebGPUShaderSection(
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

HgiWebGPUShaderSection::~HgiWebGPUShaderSection() = default;

void
HgiWebGPUShaderSection::WriteDeclaration(std::ostream &ss) const
{
    const HgiShaderSectionAttributeVector &attributes = GetAttributes();

    if (!attributes.empty()) {
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
    if (!_storageQualifier.empty()) {
        ss << _storageQualifier << " ";
    }
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    WriteArraySize(ss);
    ss << ";\n";
}

void
HgiWebGPUShaderSection::WriteParameter(std::ostream &ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << ";";
}

bool
HgiWebGPUShaderSection::VisitGlobalIncludes(std::ostream &ss)
{
    return false;
}

bool
HgiWebGPUShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    return false;
}

bool
HgiWebGPUShaderSection::VisitGlobalStructs(std::ostream &ss)
{
    return false;
}

bool HgiWebGPUShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    return false;
}

bool
HgiWebGPUShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
{
    return false;
}

HgiWebGPUMacroShaderSection::HgiWebGPUMacroShaderSection(
    const std::string& macroDeclaration, const std::string& macroComment)
    : HgiWebGPUShaderSection(macroDeclaration)
    , _macroComment(macroComment)
{
}

HgiWebGPUMacroShaderSection::~HgiWebGPUMacroShaderSection() = default;

bool
HgiWebGPUMacroShaderSection::VisitGlobalMacros(std::ostream& ss)
{
    WriteIdentifier(ss);
    return true;
}

HgiWebGPUFunctionDefShaderSection::HgiWebGPUFunctionDefShaderSection(
    const std::string& src)
    : HgiWebGPUShaderSection(src)
{
}

HgiWebGPUFunctionDefShaderSection::~HgiWebGPUFunctionDefShaderSection() =
    default;

bool
HgiWebGPUFunctionDefShaderSection::VisitGlobalFunctionDefinitions(
    std::ostream& ss)
{
    WriteIdentifier(ss);
    return true;
}

const uint32_t HgiWebGPUSamplerShaderSection::bindingSet = 2;
const std::string HgiWebGPUSamplerShaderSection::_storageQualifier = "uniform";

HgiWebGPUSamplerShaderSection::HgiWebGPUSamplerShaderSection(
    const std::string& textureSharedIdentifier,
    const uint32_t arrayOfSamplersSize, const bool isShadow,
    const HgiShaderSectionAttributeVector& attributes)
    : HgiWebGPUShaderSection("samplerBind_" + textureSharedIdentifier,
          attributes, _storageQualifier,
          "", // defaultValue
          arrayOfSamplersSize > 0 ? std::to_string(arrayOfSamplersSize) : "")
    , _arrayOfSamplersSize(arrayOfSamplersSize)
    , _isShadow(isShadow)
    , _textureSharedIdentifier(textureSharedIdentifier)
{
}

HgiWebGPUSamplerShaderSection::~HgiWebGPUSamplerShaderSection() = default;

void
HgiWebGPUSamplerShaderSection::WriteType(std::ostream& ss) const
{
    ss << (_isShadow ? "samplerShadow" : "sampler");
}

bool
HgiWebGPUSamplerShaderSection::VisitGlobalMemberDeclarations(std::ostream& ss)
{
    WriteDeclaration(ss);
    return true;
}

bool
HgiWebGPUSamplerShaderSection::VisitGlobalFunctionDefinitions(std::ostream& ss)
{
    return true;
}

const uint32_t HgiWebGPUTextureShaderSection::bindingSet = 1;
const std::string HgiWebGPUTextureShaderSection::_storageQualifier = "uniform";

HgiWebGPUTextureShaderSection::HgiWebGPUTextureShaderSection(
    const std::string& identifier,
    const HgiWebGPUSamplerShaderSection* samplerShaderSectionDependency,
    const unsigned int dimensions, const HgiFormat format,
    const HgiShaderTextureType textureType, const uint32_t arraySize,
    const bool writable, const HgiShaderSectionAttributeVector& attributes,
    const std::string& defaultValue)
    : HgiWebGPUShaderSection("textureBind_" + identifier, attributes,
          _storageQualifier, defaultValue,
          arraySize > 0 ? std::to_string(arraySize) : "")
    , _samplerSharedIdentifier(identifier)
    , _dimensions(dimensions)
    , _format(format)
    , _textureType(textureType)
    , _arraySize(arraySize)
    , _writable(writable)
    , _samplerShaderSectionDependency(samplerShaderSectionDependency)
{
}

HgiWebGPUTextureShaderSection::~HgiWebGPUTextureShaderSection() = default;

static std::string
_GetTextureTypePrefix(HgiFormat const& format)
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
HgiWebGPUTextureShaderSection::_WriteTextureType(std::ostream& ss) const
{
    if (_writable) {
        if (_textureType == HgiShaderTextureTypeArrayTexture) {
            ss << "image" << _dimensions << "DArray";
        } else if (_textureType == HgiShaderTextureTypeCubemapTexture) {
            ss << "image2DArray"; // WebGPU doesn't support writable cube maps
        } else {
            ss << "image" << _dimensions << "D";
        }
    } else {
        if (_textureType == HgiShaderTextureTypeShadowTexture) {
            ss << _GetTextureTypePrefix(_format) << "texture" << _dimensions
               << "D";
        } else if (_textureType == HgiShaderTextureTypeArrayTexture) {
            ss << _GetTextureTypePrefix(_format) << "texture" << _dimensions
               << "DArray";
        } else if (_textureType == HgiShaderTextureTypeCubemapTexture) {
            ss << _GetTextureTypePrefix(_format) << "textureCube";
        } else {
            ss << _GetTextureTypePrefix(_format) << "texture" << _dimensions
               << "D";
        }
    }
}

void
HgiWebGPUTextureShaderSection::_WriteSampledDataType(std::ostream& ss) const
{
    if (_textureType == HgiShaderTextureTypeShadowTexture) {
        ss << "float";
    } else {
        ss << _GetTextureTypePrefix(_format) << "vec4";
    }
}

void
HgiWebGPUTextureShaderSection::WriteType(std::ostream& ss) const
{
    if (_dimensions < 1 || _dimensions > 3) {
        TF_CODING_ERROR("Invalid texture dimension");
    }
    _WriteTextureType(ss); // e.g. texture<N>D, itexture<N>D, utexture<N>D
}

bool
HgiWebGPUTextureShaderSection::VisitGlobalMemberDeclarations(std::ostream& ss)
{
    WriteDeclaration(ss);
    return true;
}

bool
HgiWebGPUTextureShaderSection::VisitGlobalFunctionDefinitions(std::ostream& ss)
{
    // Used to unify texture sampling and writing across platforms that depend
    // on samplers and don't store textures in global space.
    uint32_t sizeDim;
    if (_textureType == HgiShaderTextureTypeArrayTexture) {
        sizeDim = _dimensions + 1;
    } else if (_textureType == HgiShaderTextureTypeCubemapTexture &&
        _writable) {
        sizeDim = 3;
    } else {
        sizeDim = _dimensions;
    }
    const uint32_t coordDim =
        (_textureType == HgiShaderTextureTypeShadowTexture ||
            _textureType == HgiShaderTextureTypeArrayTexture ||
            _textureType == HgiShaderTextureTypeCubemapTexture) ?
        (_dimensions + 1) :
        _dimensions;

    const std::string sizeType =
        sizeDim == 1 ? "int" : "ivec" + std::to_string(sizeDim);
    const std::string intCoordType =
        coordDim == 1 ? "int" : "ivec" + std::to_string(coordDim);
    const std::string floatCoordType =
        coordDim == 1 ? "float" : "vec" + std::to_string(coordDim);

    std::string formatSuffix;
    if (_textureType == HgiShaderTextureTypeCubemapTexture) {
        formatSuffix = "Cube";
    } else if (_textureType == HgiShaderTextureTypeShadowTexture) {
        formatSuffix = std::to_string(_dimensions) + "DShadow";
    } else if (_textureType == HgiShaderTextureTypeArrayTexture) {
        formatSuffix = std::to_string(_dimensions) + "DArray";
    } else {
        formatSuffix = std::to_string(_dimensions) + "D";
    }

    const std::string formatPrefix = _GetTextureTypePrefix(_format);
    if (_arraySize > 0) {
        ss << "#define HgiGetSampler_" << _samplerSharedIdentifier;
        ss << "(index) ";
        WriteIdentifier(ss);
        ss << "[index]\n";
    } else {
        ss << "#define HgiGetSampler_" << _samplerSharedIdentifier;
        ss << "() ";
        ss << formatPrefix << "sampler" << formatSuffix << "(";
        WriteIdentifier(ss);
        ss << ", ";
        _samplerShaderSectionDependency->WriteIdentifier(ss);
        ss << ")";
        ss << "\n";
    }

    if (_writable) {
        // Write a function that lets you write to the texture with
        // HgiSet_texName(uv, data).
        ss << "void HgiSet_";
        ss << _samplerSharedIdentifier;
        ss << "(" << intCoordType << " uv, vec4 data) {\n";
        ss << "    ";
        ss << "imageStore(";
        WriteIdentifier(ss);
        ss << ", uv, data);\n";
        ss << "}\n";

        // HgiGetSize_texName()
        ss << sizeType << " HgiGetSize_";
        ss << _samplerSharedIdentifier;
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
        ss << _samplerSharedIdentifier;
        ss << "(" << arrayInput << floatCoordType << " uv) {\n";
        ss << "    ";
        _WriteSampledDataType(ss);
        ss << " result = texture(" << formatPrefix << "sampler" << formatSuffix
           << "(";
        WriteIdentifier(ss);
        ss << arrayIndex << ", ";
        _samplerShaderSectionDependency->WriteIdentifier(ss);
        ss << arrayIndex << "), uv);\n";
        ss << "    return result;\n";
        ss << "}\n";

        // HgiGetSize_texName()
        ss << sizeType << " HgiGetSize_";
        ss << _samplerSharedIdentifier;
        ss << "(" << ((_arraySize > 0) ? "uint index" : "") << ") {\n";
        ss << "    ";
        ss << "return textureSize(" << formatPrefix << "sampler" << formatSuffix
           << "(";
        WriteIdentifier(ss);
        ss << arrayIndex << ", ";
        _samplerShaderSectionDependency->WriteIdentifier(ss);
        ss << arrayIndex << "), 0);\n";
        ss << "}\n";

        // HgiTextureLod_texName()
        _WriteSampledDataType(ss);
        ss << " HgiTextureLod_";
        ss << _samplerSharedIdentifier;
        ss << "(" << arrayInput << floatCoordType << " coord, float lod) {\n";
        ss << "    ";
        ss << "return textureLod(" << formatPrefix << "sampler" << formatSuffix
           << "(";
        WriteIdentifier(ss);
        ss << arrayIndex << ", ";
        _samplerShaderSectionDependency->WriteIdentifier(ss);
        ss << arrayIndex << "), coord, lod);\n";
        ss << "}\n";

        // HgiTexelFetch_texName()
        if (_textureType != HgiShaderTextureTypeShadowTexture &&
            _textureType != HgiShaderTextureTypeCubemapTexture) {
            _WriteSampledDataType(ss);
            ss << " HgiTexelFetch_";
            ss << _samplerSharedIdentifier;
            ss << "(" << arrayInput << intCoordType << " coord) {\n";

            // Storm depends on specific OOB access behaviour for Udims, they
            // should return 0. WebGPU instead clamps the access for memory
            // safety reasons, so we need to recreate this behaviour here.
            if (coordDim == 1) {
                ss << "    if (coord < 0 || coord >= textureSize(";
            } else {
                ss << "    if (any(lessThan(coord, " << intCoordType << "(0)))"
                   << " || any(greaterThanEqual(coord, textureSize(";
            }
            ss << formatPrefix << "sampler" << formatSuffix << "(";
            WriteIdentifier(ss);
            ss << ", ";
            _samplerShaderSectionDependency->WriteIdentifier(ss);
            ss << "), 0)";
            if (coordDim != 1) {
                ss << "))";
            }
            ss << ") { return ";
            _WriteSampledDataType(ss);
            ss << "(0); }\n";

            ss << "    ";
            _WriteSampledDataType(ss);
            ss << " result = texelFetch(" << formatPrefix << "sampler"
               << formatSuffix << "(";
            WriteIdentifier(ss);
            ss << arrayIndex << ", ";
            _samplerShaderSectionDependency->WriteIdentifier(ss);
            ss << arrayIndex << "), coord, 0);\n";
            ss << "    return result;\n";
            ss << "}\n";
        }
    }

    return true;
}

const uint32_t HgiWebGPUBufferShaderSection::bindingSet = 0;
const uint32_t HgiWebGPUBufferShaderSection::constantsBindingSet = 3;
HgiWebGPUBufferShaderSection::HgiWebGPUBufferShaderSection(
    const std::string& identifier, const bool writable, const std::string& type,
    const HgiBindingType binding, const std::string arraySize,
    const HgiShaderSectionAttributeVector& attributes)
    : HgiWebGPUShaderSection(
          identifier, attributes, writable ? "buffer" : "readonly buffer", "")
    , _type(type)
    , _binding(binding)
    , _arraySize(arraySize)
{
}

HgiWebGPUBufferShaderSection::~HgiWebGPUBufferShaderSection() = default;

void
HgiWebGPUBufferShaderSection::WriteType(std::ostream& ss) const
{
    ss << _type;
}

bool
HgiWebGPUBufferShaderSection::VisitGlobalMemberDeclarations(std::ostream& ss)
{
    // If it has attributes, write them with corresponding layout
    // identifiers and indicies
    const HgiShaderSectionAttributeVector& attributes = GetAttributes();

    if (!attributes.empty()) {
        ss << "layout(";
        for (size_t i = 0; i < attributes.size(); i++) {
            if (i > 0) {
                ss << ", ";
            }
            const HgiShaderSectionAttribute& a = attributes[i];
            ss << a.identifier;
            if (!a.index.empty()) {
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
    } else {
        ss << "[" << _arraySize << "]; };\n";
    }

    return true;
}

HgiWebGPUInterstageBlockShaderSection::HgiWebGPUInterstageBlockShaderSection(
    const std::string& blockIdentifier,
    const std::string& blockInstanceIdentifier,
    const HgiShaderSectionAttributeVector& attributes,
    const std::string& qualifier, const std::string& arraySize,
    const HgiWebGPUMemberShaderSectionPtrVector& members)
    : HgiWebGPUShaderSection(blockIdentifier, attributes, qualifier,
          std::string(), arraySize, blockInstanceIdentifier)
    , _qualifier(qualifier)
    , _members(members)
{
}

bool
HgiWebGPUInterstageBlockShaderSection::VisitGlobalMemberDeclarations(
    std::ostream& ss)
{
    // If it has attributes, write them with corresponding layout
    // identifiers and indices
    const HgiShaderSectionAttributeVector& attributes = GetAttributes();

    if (!attributes.empty()) {
        ss << "layout(";
        for (size_t i = 0; i < attributes.size(); ++i) {
            const HgiShaderSectionAttribute& a = attributes[i];
            if (i > 0) {
                ss << ", ";
            }
            ss << a.identifier;
            if (!a.index.empty()) {
                ss << " = " << a.index;
            }
        }
        ss << ") ";
    }

    ss << _qualifier << " ";
    WriteIdentifier(ss);
    ss << " {\n";
    for (const HgiWebGPUMemberShaderSection* member : _members) {
        ss << "  ";
        member->WriteInterpolation(ss);
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

HgiWebGPUMemberShaderSection::HgiWebGPUMemberShaderSection(
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
    : HgiWebGPUShaderSection(identifier,
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

HgiWebGPUMemberShaderSection::~HgiWebGPUMemberShaderSection() = default;

bool
HgiWebGPUMemberShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    if (HasBlockInstanceIdentifier()) {
        return true;
    }

    WriteInterpolation(ss);
    WriteSampling(ss);
    WriteStorage(ss);
    WriteDeclaration(ss);
    return true;
}

void
HgiWebGPUMemberShaderSection::WriteType(std::ostream& ss) const
{
    ss << _typeName;
}

void
HgiWebGPUMemberShaderSection::WriteInterpolation(std::ostream& ss) const
{
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
}

void
HgiWebGPUMemberShaderSection::WriteSampling(std::ostream& ss) const
{
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
}

void
HgiWebGPUMemberShaderSection::WriteStorage(std::ostream& ss) const
{
    switch (_storage) {
    case HgiStorageDefault:
        break;
    case HgiStoragePatch:
        ss << "patch ";
        break;
    }
}

HgiWebGPUKeywordShaderSection::HgiWebGPUKeywordShaderSection(
    const std::string &identifier,
    const std::string &type,
    const std::string &keyword)
  : HgiWebGPUShaderSection(identifier)
  , _type(type)
  , _keyword(keyword)
{
}

HgiWebGPUKeywordShaderSection::~HgiWebGPUKeywordShaderSection() = default;

void
HgiWebGPUKeywordShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}

bool
HgiWebGPUKeywordShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << " = ";
    ss << _keyword;
    ss << ";\n";

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
