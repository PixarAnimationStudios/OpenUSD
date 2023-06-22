
//
// Copyright 2023 Pixar
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

#include "pch.h"
#include "pxr/imaging/hgiDX/shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiDXShaderSection::HgiDXShaderSection(const std::string& identifier,
                                       const HgiShaderSectionAttributeVector& attributes,
                                       const std::string& defaultValue,
                                       const std::string& arraySize,
                                       const std::string& blockInstanceIdentifier)
  : HgiShaderSection(identifier, attributes, defaultValue, arraySize, blockInstanceIdentifier)
{
}

HgiDXShaderSection::~HgiDXShaderSection() = default;

bool
HgiDXShaderSection::VisitGlobalIncludes(std::ostream &ss)
{
    return false;
}

bool
HgiDXShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    return false;
}

bool
HgiDXShaderSection::VisitGlobalStructs(std::ostream &ss)
{
    return false;
}

bool HgiDXShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    return false;
}

bool
HgiDXShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
{
    return false;
}


/// <summary>
/// This is for the stage input & output parameters
/// </summary>
HgiDXParamsShaderSection::HgiDXParamsShaderSection(const std::string& name)
   :HgiDXShaderSection(name)
{
}

void 
HgiDXParamsShaderSection::AddParamInfo(const std::string& type, const std::string& name, const std::string& semantic)
{
   ParamInfo pi{ type, name, semantic };
   _info.push_back(pi);
}

bool 
HgiDXParamsShaderSection::VisitGlobalStructs(std::ostream& ss)
{
   if (_info.size() > 0)
   {
      ss << "struct ";
      WriteIdentifier(ss);
      ss << std::endl;
      ss << "{" << std::endl;

      for (const ParamInfo& pi : _info)
      {
         ss << "   " << pi.Type << " " << pi.Name;
         
         if(pi.Semantic.empty())
            ss << ";" << std::endl;
         else
            ss << " : " << pi.Semantic << ";" << std::endl;
      }

      ss << "};" << std::endl;
   }

   return true;
}


/// <summary>
/// Macros - these are just a dumb piece of text
/// </summary>
HgiDXMacroShaderSection::HgiDXMacroShaderSection(
    const std::string &macroDeclaration,
    const std::string &macroComment)
  : HgiDXShaderSection(macroDeclaration)
  , _macroComment(macroComment)
{
}

HgiDXMacroShaderSection::~HgiDXMacroShaderSection() = default;

bool
HgiDXMacroShaderSection::VisitGlobalMacros(std::ostream &ss)
{
   WriteIdentifier(ss);
    return true;
}

/// <summary>
/// global DX buffers
/// </summary>
HgiDXBufferShaderSection::HgiDXBufferShaderSection(const std::string& identifier,
                                                   const std::string& type,
                                                   const std::string& arraySize,
                                                   const uint32_t registerIndex,
                                                   const uint32_t spaceIndex,
                                                   bool bWritable)
   : HgiDXShaderSection(identifier, {}, std::string(), arraySize)
   , _type(type)
   , _registerIdx(registerIndex)
   , _spaceIdx(spaceIndex)
   , _bWritable(bWritable)
{
}

HgiDXBufferShaderSection::~HgiDXBufferShaderSection() = default;

bool
HgiDXBufferShaderSection::VisitGlobalMemberDeclarations(std::ostream& ss)
{
   bool bDynamicArray = false;
   std::string strArraySize = GetArraySize();
   if (!strArraySize.empty())
   {
      if(strArraySize == " ")
         bDynamicArray = true;
   }

   if (_bWritable)
      ss << "RWStructuredBuffer<" << _type << "> "; // UAV
   else
   {
      if (bDynamicArray)
         ss << "StructuredBuffer<" << _type << "> "; // SRV
      else
         ss << "ConstantBuffer<" << _type << "> "; // CBV
   }

   WriteIdentifier(ss);

   if ((!strArraySize.empty()) && (strArraySize != " "))
      ss << "[" << strArraySize.c_str() << "]";
   
   ss << ": register( ";
   if (_bWritable)
      ss << "u"; // UAV
   else
   {
      if(bDynamicArray)
         ss << "t"; // SRV
      else
         ss << "b"; // CBV   
   }
   
   ss << _registerIdx << ", space" << _spaceIdx << "); " << std::endl;

   return true;
}

HgiDXMemberShaderSection::HgiDXMemberShaderSection(
    const std::string &identifier,
    const std::string &typeName,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &storageQualifier,
    const std::string &defaultValue,
    const std::string& arraySize)
  : HgiDXShaderSection(identifier,
                        attributes,
                        defaultValue)
  , _typeName(typeName)
{
}

HgiDXMemberShaderSection::~HgiDXMemberShaderSection() = default;

bool
HgiDXMemberShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    return true;
}

void
HgiDXMemberShaderSection::WriteType(std::ostream& ss) const
{
}

HgiDXConstantShaderSection::HgiDXConstantShaderSection(
   const std::string& identifier,
   const HgiShaderFunctionParamDescVector& parameters)
   : HgiDXShaderSection(identifier)
   , _parameters(parameters)
{
}

HgiDXConstantShaderSection::~HgiDXConstantShaderSection() = default;

bool
HgiDXConstantShaderSection::VisitGlobalMemberDeclarations(std::ostream & ss)
{
   
   return true;
}

const std::string HgiDXTextureShaderSection::_storageQualifier = "uniform";

HgiDXTextureShaderSection::HgiDXTextureShaderSection(
    const std::string &identifier,
    const unsigned int layoutIndex,
    const unsigned int dimensions,
    const HgiFormat format,
    const HgiShaderTextureType textureType,
    const uint32_t arraySize,
    const bool writable,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &defaultValue)
  : HgiDXShaderSection( identifier,
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

HgiDXTextureShaderSection::~HgiDXTextureShaderSection() = default;

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
HgiDXTextureShaderSection::_WriteSamplerType(std::ostream &ss) const
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
HgiDXTextureShaderSection::_WriteSampledDataType(std::ostream &ss) const
{
    if (_textureType == HgiShaderTextureTypeShadowTexture) {
        ss << "float";
    } else {
        ss << _GetTextureTypePrefix(_format) << "vec4";
    }
}

void
HgiDXTextureShaderSection::WriteType(std::ostream &ss) const
{
    if(_dimensions < 1 || _dimensions > 3) {
        TF_CODING_ERROR("Invalid texture dimension");
    }
    _WriteSamplerType(ss); // e.g. sampler<N>D, isampler<N>D, usampler<N>D
}

bool
HgiDXTextureShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    WriteDeclaration(ss);
    return true;
}

bool
HgiDXTextureShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
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

HgiDXKeywordShaderSection::HgiDXKeywordShaderSection(
    const std::string &identifier,
    const std::string &type,
    const std::string &keyword)
  : HgiDXShaderSection(identifier)
  , _type(type)
  , _keyword(keyword)
{
}

HgiDXKeywordShaderSection::~HgiDXKeywordShaderSection() = default;

void
HgiDXKeywordShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}

bool
HgiDXKeywordShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
