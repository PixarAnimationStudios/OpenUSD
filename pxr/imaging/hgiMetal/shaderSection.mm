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

#include "pxr/imaging/hgiMetal/shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiMetalShaderSection::~HgiMetalShaderSection() = default;

HgiMetalShaderSection::HgiMetalShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector& attributes,
    const std::string &defaultValue,
    const std::string &arraySize,
    const std::string &blockInstanceIdentifier)
  : HgiShaderSection(identifier
  , attributes
  , defaultValue
  , arraySize
  , blockInstanceIdentifier)
{
}

bool
HgiMetalShaderSection::VisitScopeStructs(std::ostream &ss)
{
    return false;
}

bool
HgiMetalShaderSection::VisitScopeMemberDeclarations(std::ostream &ss)
{
    return false;
}

bool
HgiMetalShaderSection::VisitScopeFunctionDefinitions(std::ostream &ss)
{
    return false;
}

bool
HgiMetalShaderSection::VisitScopeConstructorDeclarations(std::ostream &ss)
{
    return false;
}

bool
HgiMetalShaderSection::VisitScopeConstructorInitialization(std::ostream &ss)
{
    return false;
}

bool
HgiMetalShaderSection::VisitEntryPointParameterDeclarations(std::ostream &ss)
{
    return false;
}

bool
HgiMetalShaderSection::VisitScopeConstructorInstantiation(std::ostream &ss)
{
    return false;
}

bool
HgiMetalShaderSection::VisitEntryPointFunctionExecutions(
    std::ostream& ss,
    const std::string &scopeInstanceName)
{
    return false;
}

void
HgiMetalShaderSection::WriteAttributesWithIndex(std::ostream& ss) const
{
    const HgiShaderSectionAttributeVector &attributes = GetAttributes();
    if (attributes.size() > 0) {
        ss << "[[";
    }
    for (size_t i = 0; i < attributes.size(); i++) {
        if (i > 0) {
            ss << ", ";
        }
        
        const HgiShaderSectionAttribute &a = attributes[i];
        ss << a.identifier;
        if (!a.index.empty()) {
            ss << "(" << a.index << ")";
        }
    }
    if (attributes.size() > 0) {
        ss << "]]";
    }
}

void
HgiMetalShaderSection::WriteAttributesOnlyWithoutIndex(std::ostream& ss) const
{
    const HgiShaderSectionAttributeVector &att = GetAttributes();
    HgiShaderSectionAttributeVector attributes;
    for (size_t i = 0; i < att.size(); i++) {
        const HgiShaderSectionAttribute &a = att[i];
        if (a.identifier.find("user") == std::string::npos) {
            attributes.push_back(a);
        }
    }
    if (attributes.size() > 0) {
        ss << "[[";
    }
    for (size_t i = 0; i < attributes.size(); i++) {
        if (i > 0) {
            ss << ", ";
        }
        
        const HgiShaderSectionAttribute &a = attributes[i];
        ss << a.identifier;
        if (!a.index.empty()) {
            ss << "(" << a.index << ")";
        }
    }
    if (attributes.size() > 0) {
        ss << "]]";
    }
}

HgiMetalMemberShaderSection::HgiMetalMemberShaderSection(
    const std::string &identifier,
    const std::string &type,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string arraySize,
    const std::string &blockInstanceIdentifier)
  : HgiMetalShaderSection(identifier, attributes,
                          std::string(), arraySize,
                          blockInstanceIdentifier)
  , _type{type}
{
}

HgiMetalMemberShaderSection::~HgiMetalMemberShaderSection() = default;

void
HgiMetalMemberShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}

bool
HgiMetalMemberShaderSection::VisitScopeMemberDeclarations(std::ostream &ss)
{
    if (!HasBlockInstanceIdentifier()) {
        WriteDeclaration(ss);
        ss << std::endl;
    }
    return true;
}

HgiMetalRawShaderSection::HgiMetalRawShaderSection(
    const std::string &identifier,
    const std::string &type,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string arraySize,
    const std::string &blockInstanceIdentifier)
  : HgiMetalShaderSection(identifier, attributes,
                          std::string(), arraySize,
                          blockInstanceIdentifier)
  , _type{type}
{
}

HgiMetalRawShaderSection::~HgiMetalRawShaderSection() = default;

void
HgiMetalRawShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}

HgiMetalMeshShaderSection::HgiMetalMeshShaderSection(
    const std::string &identifier,
    const std::string &type)
  : HgiMetalShaderSection(identifier, {},
                          std::string(), std::string(),
                          std::string())
  , _type{type}
{
}

HgiMetalMeshShaderSection::~HgiMetalMeshShaderSection() = default;

void
HgiMetalMeshShaderSection::WriteType(std::ostream &ss) const
{
    ss << _type;
}


bool
HgiMetalMeshShaderSection::VisitScopeConstructorDeclarations(
    std::ostream &ss)
{
    ss << "thread ";
    WriteType(ss);
    ss << " &_";
    WriteIdentifier(ss);
    return true;
}

bool
HgiMetalMeshShaderSection::VisitScopeConstructorInitialization(
    std::ostream &ss)
{
    WriteIdentifier(ss);
    ss << "(_";
    WriteIdentifier(ss);
    ss << ")";
    return true;
}

bool
HgiMetalMeshShaderSection::VisitScopeConstructorInstantiation(
    std::ostream &ss)
{
    WriteIdentifier(ss);
    return true;
}

bool
HgiMetalMeshShaderSection::VisitScopeMemberDeclarations(std::ostream &ss)
{
    ss << "thread ";
    WriteType(ss);
    ss << " &";
    WriteIdentifier(ss);
    ss << ";\n";
    return true;
}

bool
HgiMetalMeshShaderSection::VisitEntryPointParameterDeclarations(
    std::ostream &ss)
{
    WriteParameter(ss);
    return true;
}

HgiMetalSamplerShaderSection::HgiMetalSamplerShaderSection(
    const std::string &textureSharedIdentifier,
    const std::string &parentScopeIdentifier,
    const uint32_t arrayOfSamplersSize,
    const HgiShaderSectionAttributeVector &attributes)
  : HgiMetalShaderSection(
      "samplerBind_" + textureSharedIdentifier,
      attributes)
  , _textureSharedIdentifier(textureSharedIdentifier)
  , _arrayOfSamplersSize(arrayOfSamplersSize)
  , _parentScopeIdentifier(parentScopeIdentifier)
{
}

void
HgiMetalSamplerShaderSection::WriteType(std::ostream& ss) const
{
    if (_arrayOfSamplersSize > 0) {
        ss << "array<sampler, " << _textureSharedIdentifier << "_SIZE>";
    } else {
        ss << "sampler";
    }
}

void
HgiMetalSamplerShaderSection::WriteParameter(std::ostream& ss) const
{
    if (_arrayOfSamplersSize > 0) {
        ss << "#define " << _textureSharedIdentifier << "_SIZE " 
           << _arrayOfSamplersSize << "\n";
    }
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
}

bool
HgiMetalSamplerShaderSection::VisitScopeConstructorDeclarations(
    std::ostream &ss)
{
    WriteType(ss);
    ss << " _";
    WriteIdentifier(ss);
    return true;
}

bool
HgiMetalSamplerShaderSection::VisitScopeConstructorInitialization(
    std::ostream &ss)
{
    WriteIdentifier(ss);
    ss << "(_";
    WriteIdentifier(ss);
    ss << ")";
    return true;
}

bool
HgiMetalSamplerShaderSection::VisitScopeConstructorInstantiation(
    std::ostream &ss)
{
    if (!_parentScopeIdentifier.empty()) {
        ss << _parentScopeIdentifier << "->";
    }

    WriteIdentifier(ss);
    return true;
}

bool
HgiMetalSamplerShaderSection::VisitScopeMemberDeclarations(std::ostream &ss)
{
    WriteDeclaration(ss);
    ss << std::endl;
    return true;
}

HgiMetalTextureShaderSection::HgiMetalTextureShaderSection(
    const std::string &samplerSharedIdentifier,
    const std::string &parentScopeIdentifier,
    const HgiShaderSectionAttributeVector &attributes,
    const HgiMetalSamplerShaderSection *samplerShaderSectionDependency,
    uint32_t dimensions,
    HgiFormat format,
    bool textureArray,
    uint32_t arrayOfTexturesSize,
    bool shadow,
    bool writable,
    const std::string &defaultValue)
  : HgiMetalShaderSection(
      "textureBind_" + samplerSharedIdentifier, 
      attributes,
      defaultValue)
  , _samplerSharedIdentifier(samplerSharedIdentifier)
  , _samplerShaderSectionDependency(samplerShaderSectionDependency)
  , _dimensionsVar(dimensions)
  , _format(format)
  , _textureArray(textureArray)
  , _arrayOfTexturesSize(arrayOfTexturesSize)
  , _shadow(shadow)
  , _writable(writable)
  , _parentScopeIdentifier(parentScopeIdentifier)
{
    HgiFormat baseFormat = HgiGetComponentBaseFormat(_format);

    switch (baseFormat) {
    case HgiFormatFloat32:
        _baseType = "float";
        _returnType = "vec";
        break;
    case HgiFormatFloat16:
        _baseType = "half";
        _returnType = "vec";
        break;
    case HgiFormatInt32:
        _baseType = "int32_t";
        _returnType = "ivec";
        break;
    case HgiFormatInt16:
        _baseType = "int16_t";
        _returnType = "ivec";
        break;
    case HgiFormatUInt16:
        _baseType = "uint16_t";
        _returnType = "uvec";
        break;
    case HgiFormatUNorm8:
        _baseType = "float";
        _returnType = "vec";
        break;
    default:
        TF_CODING_ERROR("Invalid Format");
        _baseType = "float";
        _returnType = "vec";
        break;
    }

    if (shadow) {
        _baseType = "float";
        _returnType = "float";
    }
}

void
HgiMetalTextureShaderSection::WriteType(std::ostream& ss) const
{
    if (_arrayOfTexturesSize > 0) {
        if (_shadow) {
            ss << "array<depth" << _dimensionsVar << "d";
            ss << "<" << _baseType;
            ss << ">, " << _samplerSharedIdentifier << "_SIZE>";
        } else {
            ss << "array<texture" << _dimensionsVar << "d";
            ss << "<" << _baseType;
            ss << ">, " << _samplerSharedIdentifier << "_SIZE>";
        }
    } else {
        if (_shadow) {
            ss << "depth" << _dimensionsVar << "d";
            if (_textureArray) {
                ss << "_array";
            }
            ss << "<" << _baseType;
            if (_writable) {
                ss  << ", access::write";
            }
            ss << ">";
        } else {
            ss << "texture" << _dimensionsVar << "d";
            if (_textureArray) {
                ss << "_array";
            }
            ss << "<" << _baseType;
            if (_writable) {
                ss  << ", access::write";
            }
            ss << ">";
        }
    }
}

void
HgiMetalTextureShaderSection::WriteParameter(std::ostream& ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
}

bool
HgiMetalTextureShaderSection::VisitScopeConstructorDeclarations(
    std::ostream &ss)
{
    WriteType(ss);
    ss << " _";
    WriteIdentifier(ss);
    return true;
}

bool
HgiMetalTextureShaderSection::VisitScopeConstructorInitialization(
    std::ostream &ss)
{
    WriteIdentifier(ss);
    ss << "(_";
    WriteIdentifier(ss);
    ss << ")";
    return true;
}

bool
HgiMetalTextureShaderSection::VisitScopeConstructorInstantiation(
    std::ostream &ss)
{
    if (!_parentScopeIdentifier.empty()) {
        ss << _parentScopeIdentifier << "->";
    }

    WriteIdentifier(ss);
    return true;
}

bool
HgiMetalTextureShaderSection::VisitScopeMemberDeclarations(std::ostream &ss)
{
    WriteDeclaration(ss);
    ss << std::endl;
    return true;
}

bool
HgiMetalTextureShaderSection::VisitScopeFunctionDefinitions(std::ostream &ss)
{
    const std::string defValue = _GetDefaultValue().empty()
        ? "0"
        : _GetDefaultValue();

    if (_arrayOfTexturesSize > 0) {
        if (_shadow) {
            ss << "depth" << _dimensionsVar << "d";
            ss << "<" << _baseType;
            ss << ">";
        } else {
            ss << "texture" << _dimensionsVar << "d";
            ss << "<" << _baseType;
            ss << ">";
        }
        ss << " HgiGetSampler_" << _samplerSharedIdentifier
        << "(uint index) {\n"
        << "    return ";
        WriteIdentifier(ss);
        ss << "[index];\n}\n";
    } else {
        ss << "#define HgiGetSampler_" << _samplerSharedIdentifier
        << "() ";
        WriteIdentifier(ss);
        ss << "\n";
    }
   
    uint32_t dimensions = _textureArray ? (_dimensionsVar + 1) : _dimensionsVar;
   
    const std::string intType = _dimensionsVar == 1 ?
        "int" :
        "ivec" + std::to_string(_dimensionsVar);

    if (_writable) {
        // Write a function that lets you write to the texture with
        // HgiSet_texName(uv, data).
        ss << "void HgiSet_";
        ss << _samplerSharedIdentifier;
        ss << "(" << intType << " uv, vec4 data) {\n";
        ss << "    ";
        ss << "imageStore(";
        WriteIdentifier(ss);
        ss << ", uv, ";
        ss << _baseType;
        ss << "4(data));\n";
        ss << "}\n";

        // HgiGetSize_texName()
        ss << intType << " HgiGetSize_";
        ss << _samplerSharedIdentifier;
        ss << "() {\n";
        ss << "    ";
        ss << "return imageSize";
        ss << _dimensionsVar;
        ss << "d(";
        WriteIdentifier(ss);
        ss << ");\n";
        ss << "}\n";
        return true;
    }
   
    // Generated code looks like this for texture 'diffuse':
    //
    // vec4 HgiGet_diffuse(vec2 coord) {
    //     vec4 result = is_null_texture(textureBind_diffuse) ? 0 :
    //           textureBind_diffuse.sample(samplerBind_diffuse, coord);
    //     return result;
    // }
   
    const std::string returnType = _shadow ? "float" : _returnType + "4";
    const std::string coordType = 
        dimensions > 1 ? "vec" + std::to_string(dimensions) : "float";
    const std::string shadowCoordType = "vec" + std::to_string(dimensions + 1);

    if (_shadow) {
        if (_arrayOfTexturesSize > 0) {
            ss << returnType << " HgiGet_" << _samplerSharedIdentifier;
            ss << "(uint index, ";
            ss << shadowCoordType << " coord) {\n";
            ss << "    " << returnType << " result = is_null_texture(";
            WriteIdentifier(ss);
            ss << "[index]) ? " << defValue << " : "
               << returnType << "(";
            WriteIdentifier(ss);
            ss << "[index].sample_compare(";
            _samplerShaderSectionDependency->WriteIdentifier(ss);
            ss << "[index], coord.xy, coord.z" << "));\n";
        } else {
            ss << returnType << " HgiGet_" << _samplerSharedIdentifier;
            ss << "(" << shadowCoordType << " coord) {\n";
            ss << "    " << returnType << " result = is_null_texture(";
            WriteIdentifier(ss);
            ss << ") ? " << defValue << " : "
               << returnType << "(";
            WriteIdentifier(ss);
            ss << ".sample_compare(";
            _samplerShaderSectionDependency->WriteIdentifier(ss);
            ss << ", coord.xy, coord.z));\n";
        }
    } else {
        if (_arrayOfTexturesSize > 0) {
            ss << returnType << " HgiGet_" << _samplerSharedIdentifier;
            ss << "(uint index, ";
            ss << coordType << " coord) {\n";
            ss << "    " << returnType << " result = is_null_texture(";
            WriteIdentifier(ss);
            ss << "[index]) ? " << defValue << " : "
               << returnType << "(";
            WriteIdentifier(ss);
            ss << "[index].sample(";
            _samplerShaderSectionDependency->WriteIdentifier(ss);
            ss << "[index], coord));\n";
        } else {
            ss << returnType << " HgiGet_" << _samplerSharedIdentifier;
            ss << "(" << coordType << " coord) {\n";
            ss << "    " << returnType << " result = is_null_texture(";
            WriteIdentifier(ss);
            ss << ") ? " << defValue << " : "
               << returnType << "(";
            WriteIdentifier(ss);
            ss << ".sample(";
            _samplerShaderSectionDependency->WriteIdentifier(ss);
            if (_textureArray) {
                if (_dimensionsVar == 2) {
                    ss << ", coord.xy, coord.z));\n";
                }
                else {
                    ss << ", coord.x, coord.y));\n";
                }
            }
            else {
                ss << ", coord));\n";
            }
        }
    }
    ss << "    return result;\n";
    ss << "}\n";

    // HgiGetSize_texName()
    ss << intType << " HgiGetSize_";
    ss << _samplerSharedIdentifier;
    if (_arrayOfTexturesSize > 0) {
        ss << "(uint index) {\n";
    } else {
        ss << "() {\n";
    }
    ss << "    ";
    ss << "return textureSize";
    ss << _dimensionsVar;
    ss << "d(";
    WriteIdentifier(ss);
    if (_arrayOfTexturesSize > 0) {
        ss << "[index]";
    }
    ss << ", 0);\n";
    ss << "}\n";
   
    // Generated code looks like this for texture 'diffuse':
    //
    // vec4 HgiTexelFetch_diffuse(ivec2 coord) {
    //     vec4 result =  textureBind_diffuse.read(ushort2(coord.x, coord.y));
    //     return result;
    // }
    const std::string intCoordType = 
        dimensions > 1 ? "ivec" + std::to_string(dimensions) : "int";
    ss << returnType << " HgiTexelFetch_" << _samplerSharedIdentifier;
    if (_arrayOfTexturesSize > 0) {
        ss << "(uint index, " << intCoordType << " coord) {\n";
    } else {
        ss << "(" << intCoordType << " coord) {\n";
    }
    ss << "    " << returnType
       << " result = " << returnType << "(textureBind_"
       << _samplerSharedIdentifier;
    if (_arrayOfTexturesSize > 0) {
        ss << "[index]";
    }
    if (_textureArray) {
        if (_dimensionsVar == 2) {
            ss << ".read(ushort2(coord.x, coord.y), coord.z));\n";
        }
        else {
            ss << ".read(ushort(coord.x), coord.y));\n";
        }
    }
    else {
        if (_dimensionsVar == 3) {
            ss << ".read(ushort3(coord.x, coord.y, coord.z)));\n";
        }
        else if (_dimensionsVar == 2) {
            ss << ".read(ushort2(coord.x, coord.y)));\n";
        }
        else {
            ss << ".read(ushort(coord)));\n";
        }
    }
    ss << "    return result;\n";
    ss << "}\n";
   
    // Generated code looks like this for texture 'diffuse':
    //
    // vec4 HgiTextureLod_diffuse(vec2 coord, float lod) {
    //     vec4 result =  textureBind_diffuse.sample(coord, level(lod));
    //     return result;
    // }
    if (_shadow) {
        ss << returnType << " HgiTextureLod_" << _samplerSharedIdentifier;
        if (_arrayOfTexturesSize > 0) {
            ss << "(uint index, " << shadowCoordType << " coord";
        } else {
            ss << "(" << shadowCoordType << " coord";
        }
        ss << ", float lod) {\n"
           << "    " << returnType << " result = "
           << returnType << "(textureBind_"
           << _samplerSharedIdentifier;
        if (_arrayOfTexturesSize > 0) {
            ss << "[index]";
        }
        ss << ".sample(";
        _samplerShaderSectionDependency->WriteIdentifier(ss);
        if (_arrayOfTexturesSize > 0) {
            ss << "[index]";
        }
        ss << ", coord.xy, level(lod)));\n"
           << "    return result;\n"
           << "}\n";
    } else {
        ss << returnType << " HgiTextureLod_" << _samplerSharedIdentifier;
        if (_arrayOfTexturesSize > 0) {
            ss << "(uint index, " << coordType << " coord";
        } else {
            ss << "(" << coordType << " coord";
        }
        ss << ", float lod) {\n"
           << "    " << returnType << " result = "
           << returnType << "(textureBind_"
           << _samplerSharedIdentifier;
        if (_arrayOfTexturesSize > 0) {
            ss << "[index]";
        }
        ss << ".sample(";
        _samplerShaderSectionDependency->WriteIdentifier(ss);
        if (_arrayOfTexturesSize > 0) {
            ss << "[index]";
        }
        if (_textureArray) {
            if (_dimensionsVar == 2) {
                ss << ", coord.xy, coord.z";
            }
            else {
                ss << ", coord.x, coord.y";
            }
        } else {
            ss << ", coord";
        }
        if (_dimensionsVar > 1) {
            ss << ", level(lod)";
        }
        ss << "));\n"
           << "    return result;\n"
           << "}\n";
    }

    return true;
}

HgiMetalBufferShaderSection::HgiMetalBufferShaderSection(
    const std::string &samplerSharedIdentifier,
    const std::string &parentScopeIdentifier,
    const std::string &type,
    const HgiBindingType binding,
    const bool writable,
    const HgiShaderSectionAttributeVector &attributes)
  : HgiMetalShaderSection(
      samplerSharedIdentifier,
      attributes,
      "")
  , _type(type)
  , _binding(binding)
  , _writable(writable)
  , _unused(false)
  , _samplerSharedIdentifier(samplerSharedIdentifier)
  , _parentScopeIdentifier(parentScopeIdentifier)
{
}

HgiMetalBufferShaderSection::HgiMetalBufferShaderSection(
    const std::string &samplerSharedIdentifier,
    const HgiShaderSectionAttributeVector &attributes)
  : HgiMetalShaderSection(
      samplerSharedIdentifier,
      attributes,
      "")
  , _type("void")
  , _binding(HgiBindingTypePointer)
  , _writable(false)
  , _unused(true)
{
}

void
HgiMetalBufferShaderSection::WriteType(std::ostream& ss) const
{
    ss << _type;
}

void
HgiMetalBufferShaderSection::WriteParameter(std::ostream& ss) const
{
    //TODO resolve -> meshlet shader buffers can't be declared in constant space
    //if (!_writable) {
    //    ss << "constant ";
    //} else {
        ss << "device ";
    //}
    WriteType(ss);
    
    switch (_binding) {
    case HgiBindingTypeValue:
    case HgiBindingTypeUniformValue:
        ss << "& ";
        break;
    case HgiBindingTypeArray:
    case HgiBindingTypeUniformArray:
    case HgiBindingTypePointer:
        ss << "* ";
        break;
    }

    WriteIdentifier(ss);
}

bool
HgiMetalBufferShaderSection::VisitScopeMemberDeclarations(std::ostream &ss)
{
    if (_unused) return false;

    //if (!_writable) {
    //    ss << "constant ";
    //} else {
        ss << "device ";
    //}
    WriteType(ss);

    switch (_binding) {
    case HgiBindingTypeValue:
    case HgiBindingTypeUniformValue:
        ss << "& ";
        break;
    case HgiBindingTypeArray:
    case HgiBindingTypeUniformArray:
    case HgiBindingTypePointer:
        ss << "* ";
        break;
    }

    WriteIdentifier(ss);
    ss << ";\n";
    return true;
}

bool
HgiMetalBufferShaderSection::VisitScopeConstructorDeclarations(
    std::ostream &ss)
{
    if (_unused) return false;
    
    //TODO resolve -> meshlet shader buffers can't be declared in constant space
    //if (!_writable) {
    //    ss << "const ";
    //    ss << "constant ";
    //} else {
        ss << "device ";
    //}
    WriteType(ss);
    ss << "* _";
    WriteIdentifier(ss);
    return true;
}

bool
HgiMetalBufferShaderSection::VisitScopeConstructorInitialization(
    std::ostream &ss)
{
    if (_unused) return false;

    WriteIdentifier(ss);
    switch (_binding) {
    case HgiBindingTypeValue:
    case HgiBindingTypeUniformValue:
        ss << "(*_";
        break;
    case HgiBindingTypeArray:
    case HgiBindingTypeUniformArray:
    case HgiBindingTypePointer:
        ss << "(_";
        break;
    }
    WriteIdentifier(ss);
    ss << ")";
    return true;
}

bool
HgiMetalBufferShaderSection::VisitScopeConstructorInstantiation(
    std::ostream &ss)
{
    if (_unused) return false;

    if (!_parentScopeIdentifier.empty()) {
        if (_binding == HgiBindingTypeValue ||
            _binding == HgiBindingTypeUniformValue) {
            ss << "&";
        }
        ss << _parentScopeIdentifier << "->";
    }

    WriteIdentifier(ss);
    return true;
}

HgiMetalStructTypeDeclarationShaderSection::HgiMetalStructTypeDeclarationShaderSection(
    const std::string &identifier,
    const HgiMetalShaderSectionPtrVector &members,
    const std::string &templateWrapper,
    const std::string &templateWrapperParameters,
    const bool useAttributes)
  : HgiMetalShaderSection(identifier)
  , _members(members)
  , _templateWrapper(templateWrapper)
  , _templateWrapperParameters(templateWrapperParameters)
  , _useAttributes(useAttributes)
{
}

void
HgiMetalStructTypeDeclarationShaderSection::WriteType(std::ostream &ss) const
{
    ss << "struct";
}

void
HgiMetalStructTypeDeclarationShaderSection::WriteDeclaration(
    std::ostream &ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << " {\n";
    for (HgiMetalShaderSection* member : _members) {
        member->WriteParameter(ss);
        if (!member->HasBlockInstanceIdentifier()) {
            if (_useAttributes) {
                member->WriteAttributesWithIndex(ss);
            } else {
                member->WriteAttributesOnlyWithoutIndex(ss);
            }
        }
        member->WriteArraySize(ss);
        ss << ";\n";
    }
    ss << "};\n";
}

void
HgiMetalStructTypeDeclarationShaderSection::WriteParameter(
    std::ostream &ss) const
{
}

void
HgiMetalStructTypeDeclarationShaderSection::WriteTemplateWrapper(
    std::ostream &ss) const
{
    if (!_templateWrapper.empty()) {
        ss << _templateWrapper << "<";
        WriteIdentifier(ss);
        if (!_templateWrapperParameters.empty()) {
            ss << ", ";
        }
        ss << _templateWrapperParameters << ">";
    } else {
        WriteIdentifier(ss);
    }
    
}

const HgiMetalShaderSectionPtrVector&
HgiMetalStructTypeDeclarationShaderSection::GetMembers() const
{
    return _members;
}

HgiMetalStructInstanceShaderSection::HgiMetalStructInstanceShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector &attributes,
    HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration,
    const std::string &defaultValue)
  : HgiMetalShaderSection (
      identifier,
      attributes,
      defaultValue)
  , _structTypeDeclaration(structTypeDeclaration)
{
}

void
HgiMetalStructInstanceShaderSection::WriteType(std::ostream &ss) const
{
    _structTypeDeclaration->WriteIdentifier(ss);
}

const HgiMetalStructTypeDeclarationShaderSection*
HgiMetalStructInstanceShaderSection::GetStructTypeDeclaration() const
{
    return _structTypeDeclaration;
}

HgiMetalParameterInputShaderSection::HgiMetalParameterInputShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &addressSpace,
    const bool isPointer,
    HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration)
  : HgiMetalStructInstanceShaderSection(
      identifier,
      attributes,
      structTypeDeclaration)
  , _addressSpace(addressSpace)
  , _isPointer(isPointer)
{
}

void
HgiMetalParameterInputShaderSection::WriteParameter(std::ostream& ss) const
{
    GetStructTypeDeclaration()->WriteTemplateWrapper(ss);
    ss << " ";
    if(_isPointer) {
        ss << "*";
    }
    WriteIdentifier(ss);
}

bool
HgiMetalParameterInputShaderSection::VisitEntryPointParameterDeclarations(
    std::ostream &ss)
{
    if(!_addressSpace.empty()) {
        ss << _addressSpace << " ";
    }
    
    WriteParameter(ss);
    WriteAttributesWithIndex(ss);
    return true;
}

bool
HgiMetalParameterInputShaderSection::VisitEntryPointFunctionExecutions(
    std::ostream& ss,
    const std::string &scopeInstanceName)
{
    const auto &structDeclMembers = GetStructTypeDeclaration()->GetMembers();
    for (size_t i = 0; i < structDeclMembers.size(); ++i) {
        if (i > 0) {
            ss << "\n";
        }
        HgiShaderSection *member = structDeclMembers[i];
        const std::string &arraySize = member->GetArraySize();
        if (!arraySize.empty()) {
            ss << "for (int arrInd = 0; arrInd < ";
            ss << arraySize;
            ss << "; arrInd++) {\n";
            ss << scopeInstanceName << ".";
            member->WriteIdentifier(ss);
            ss << "[arrInd] = ";
            WriteIdentifier(ss);
            ss << "[arrInd]"
               << (_isPointer ? "->" : ".");
            member->WriteIdentifier(ss);
            ss << ";\n}";
        } else {
            ss << scopeInstanceName << ".";
            if (member->HasBlockInstanceIdentifier()) {
                member->WriteBlockInstanceIdentifier(ss);
                ss << ".";
            }
            member->WriteIdentifier(ss);
            ss << " = ";
            WriteIdentifier(ss);
            ss << (_isPointer ? "->" : ".");
            member->WriteIdentifier(ss);
            ss << ";";
        }
    }
    return true;
}

bool
HgiMetalParameterInputShaderSection::VisitGlobalMemberDeclarations(
    std::ostream &ss)
{
    GetStructTypeDeclaration()->WriteDeclaration(ss);
    ss << "\n";
    return true;
}

HgiMetalParameterMeshInputShaderSection::HgiMetalParameterMeshInputShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &addressSpace,
    const bool isPointer,
    HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration)
  : HgiMetalStructInstanceShaderSection(
      identifier,
      attributes,
      structTypeDeclaration)
  , _addressSpace(addressSpace)
  , _isPointer(isPointer)
{
}

void
HgiMetalParameterMeshInputShaderSection::WriteParameter(std::ostream& ss) const
{
    GetStructTypeDeclaration()->WriteTemplateWrapper(ss);
    ss << " ";
    if(_isPointer) {
        ss << "*";
    }
    WriteIdentifier(ss);
}

bool
HgiMetalParameterMeshInputShaderSection::VisitEntryPointParameterDeclarations(
    std::ostream &ss)
{
    if(!_addressSpace.empty()) {
        ss << _addressSpace << " ";
    }
    
    WriteParameter(ss);
    WriteAttributesWithIndex(ss);
    return true;
}

bool
HgiMetalParameterMeshInputShaderSection::VisitEntryPointFunctionExecutions(
    std::ostream& ss,
    const std::string &scopeInstanceName)
{
    const auto &structDeclMembers = GetStructTypeDeclaration()->GetMembers();
    for (size_t i = 0; i < structDeclMembers.size(); ++i) {
        if (i > 0) {
            ss << "\n";
        }
        HgiShaderSection *member = structDeclMembers[i];
        const std::string &arraySize = member->GetArraySize();
        if (!arraySize.empty()) {
            ss << "for (int arrInd = 0; arrInd < ";
            ss << arraySize;
            ss << "; arrInd++) {\n";
            ss << scopeInstanceName << ".";
            member->WriteIdentifier(ss);
            ss << "[arrInd] = ";
            WriteIdentifier(ss);
            ss << "[arrInd]"
               << (_isPointer ? "->" : ".");
            member->WriteIdentifier(ss);
            ss << ";\n}";
        } else {
            ss << scopeInstanceName << ".";
            if (member->HasBlockInstanceIdentifier()) {
                member->WriteBlockInstanceIdentifier(ss);
                ss << ".";
            }
            member->WriteIdentifier(ss);
            ss << " = ";
            WriteIdentifier(ss);
            ss << (_isPointer ? "->" : ".");
            ss << "vertexOut.";
            member->WriteIdentifier(ss);
            ss << ";";
        }
    }
    return true;
}

bool
HgiMetalParameterMeshInputShaderSection::VisitGlobalMemberDeclarations(
    std::ostream &ss)
{
    ss << "struct PrimOut { };\n";
    ss << "struct VertexOut {\n";
    
    for (auto &member : GetStructTypeDeclaration()->GetMembers()) {
        //member->WriteParameter(ss);
        member->WriteType(ss);
        ss << " ";
        member->WriteIdentifier(ss);
        ss << ";\n";
    }
    ss << "};\n";
    
    ss << "struct ";
    GetStructTypeDeclaration()->WriteIdentifier(ss);
    ss << "{\n";
    ss << "VertexOut vertexOut;\n";
    ss << "PrimOut primOut;\n";
    ss << "};\n";
    return true;
}


HgiMetalArgumentBufferInputShaderSection::HgiMetalArgumentBufferInputShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &addressSpace,
    const bool isPointer,
    HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration)
  : HgiMetalStructInstanceShaderSection(
      identifier,
      attributes,
      structTypeDeclaration)
  , _addressSpace(addressSpace)
  , _isPointer(isPointer)
{
}

void
HgiMetalArgumentBufferInputShaderSection::WriteParameter(std::ostream& ss) const
{
    WriteType(ss);
    ss << " ";
    if(_isPointer) {
        ss << "*";
    }
    WriteIdentifier(ss);
}

bool
HgiMetalArgumentBufferInputShaderSection::VisitEntryPointParameterDeclarations(
    std::ostream &ss)
{
    if(!_addressSpace.empty()) {
        ss << _addressSpace << " ";
    }
    
    WriteParameter(ss);
    WriteAttributesWithIndex(ss);
    return true;
}

bool
HgiMetalArgumentBufferInputShaderSection::VisitGlobalMemberDeclarations(
    std::ostream &ss)
{
    GetStructTypeDeclaration()->WriteDeclaration(ss);
    ss << "\n";
    return true;
}

HgiMetalPayloadShaderSection::HgiMetalPayloadShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector &attributes,
    const std::string &addressSpace,
    const bool isPointer,
    const bool isConstParameter,
    HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration)
  : HgiMetalStructInstanceShaderSection(
      identifier,
      attributes,
      structTypeDeclaration)
  , _addressSpace(addressSpace)
  , _isConstParameter(isConstParameter)
{
}

void
HgiMetalPayloadShaderSection::WriteParameter(std::ostream& ss) const
{
    WriteType(ss);
    ss << " &";
    WriteIdentifier(ss);
}

bool
HgiMetalPayloadShaderSection::VisitEntryPointParameterDeclarations(
    std::ostream &ss)
{
    if(!_addressSpace.empty()) {
        if (_isConstParameter) {
            ss << "const ";
        }
        ss << _addressSpace << " ";
    }
    
    WriteParameter(ss);
    WriteAttributesWithIndex(ss);
    return true;
}

bool
HgiMetalPayloadShaderSection::VisitGlobalMemberDeclarations(
    std::ostream &ss)
{
    GetStructTypeDeclaration()->WriteDeclaration(ss);
    ss << "\n";
    return true;
}

bool
HgiMetalPayloadShaderSection::VisitScopeMemberDeclarations(
    std::ostream &ss)
{
    if (_isConstParameter) {
        ss << "const ";
    }
    ss << "object_data ";
    WriteType(ss);
    ss << "& ";
    WriteIdentifier(ss);
    ss << ";\n";
    return true;
}

bool
HgiMetalPayloadShaderSection::VisitScopeConstructorDeclarations(
    std::ostream &ss)
{
    if (_isConstParameter) {
        ss << "const ";
    }
    ss << "object_data ";
        WriteType(ss);
        ss << "& _";
    WriteIdentifier(ss);
    return true;
}

bool
HgiMetalPayloadShaderSection::VisitScopeConstructorInitialization(
    std::ostream &ss)
{
    WriteIdentifier(ss);
    ss << "(_";
    WriteIdentifier(ss);
    ss << ")";
    return true;
}

bool
HgiMetalPayloadShaderSection::VisitScopeConstructorInstantiation(
    std::ostream &ss)
{
    WriteIdentifier(ss);
    return true;
}

HgiMetalKeywordInputShaderSection::HgiMetalKeywordInputShaderSection(
    const std::string &identifier,
    const std::string &type,
    const HgiShaderSectionAttributeVector &attributes,
    const bool isPointerToValue)
  : HgiMetalShaderSection(
      identifier,
      attributes,
      "")
  , _type(type)
  , _isPointerToValue(isPointerToValue)
{
}

void
HgiMetalKeywordInputShaderSection::WriteType(std::ostream& ss) const
{
    ss << _type;
}

bool
HgiMetalKeywordInputShaderSection::VisitScopeMemberDeclarations(
    std::ostream &ss)
{   if (_isPointerToValue) {
        ss << "thread ";
    }
    WriteType(ss);
    ss << " ";
    if (_isPointerToValue) {
        ss << "* ";
    }
    WriteIdentifier(ss);
    ss << ";\n";
    return true;
}

bool
HgiMetalKeywordInputShaderSection::VisitEntryPointParameterDeclarations(
    std::ostream &ss)
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);

    WriteAttributesWithIndex(ss);
    return true;
}

bool
HgiMetalKeywordInputShaderSection::VisitEntryPointFunctionExecutions(
    std::ostream& ss,
    const std::string &scopeInstanceName)
{
    ss << scopeInstanceName << ".";
    WriteIdentifier(ss);
    ss << " = ";
    if (_isPointerToValue) {
        ss << "&";
    }
    WriteIdentifier(ss);
    ss << ";";
    return true;
}

HgiMetalStageOutputShaderSection::HgiMetalStageOutputShaderSection(
    const std::string &identifier,
    HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration)
  : HgiMetalStructInstanceShaderSection(
      identifier,
      {},
      structTypeDeclaration)
{
}

HgiMetalStageOutputShaderSection::HgiMetalStageOutputShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector &attributes,
    //for the time being, addressspace is just an adapter
    const std::string &addressSpace,
    const bool isPointer,
    HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration)
  : HgiMetalStructInstanceShaderSection(
      identifier,
      {},
      structTypeDeclaration)
{
}

bool
HgiMetalStageOutputShaderSection::VisitEntryPointFunctionExecutions(
    std::ostream& ss,
    const std::string &scopeInstanceName)
{
    ss << scopeInstanceName << ".main();\n";
    WriteDeclaration(ss);
    ss << "\n";
    const auto &structTypeDeclMembers =
        GetStructTypeDeclaration()->GetMembers();
    for (size_t i = 0; i < structTypeDeclMembers.size(); ++i) {
        if (i > 0) {
            ss << "\n";
        }
        HgiShaderSection *member = structTypeDeclMembers[i];
        WriteIdentifier(ss);
        ss << ".";
        member->WriteIdentifier(ss);
        ss << " = " << scopeInstanceName << ".";
        if (member->HasBlockInstanceIdentifier()) {
            member->WriteBlockInstanceIdentifier(ss);
            ss << ".";
        }
        member->WriteIdentifier(ss);
        ss << ";";
    }
    return true;
}

bool
HgiMetalStageOutputShaderSection::VisitGlobalMemberDeclarations(
    std::ostream &ss)
{
    GetStructTypeDeclaration()->WriteDeclaration(ss);
    ss << "\n";
    return true;
}

HgiMetalStageOutputMeshShaderSection::HgiMetalStageOutputMeshShaderSection(
    HgiMetalStructTypeDeclarationShaderSection * const structTypeDeclaration)
  : HgiMetalShaderSection("")
, _structTypeDeclaration(structTypeDeclaration)
{
}

bool
HgiMetalStageOutputMeshShaderSection::VisitGlobalMemberDeclarations(
    std::ostream &ss)
{
    ss << "struct PrimOut { };\n";
    _structTypeDeclaration->WriteDeclaration(ss);
    ss << "\n";
    return true;
}

bool
HgiMetalShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    return false;
}

bool
HgiMetalShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    return false;
}

HgiMetalMacroShaderSection::HgiMetalMacroShaderSection(
    const std::string &macroDeclaration,
    const std::string &macroComment)
  : HgiMetalShaderSection(macroDeclaration)
  , _macroComment(macroComment)
{
}

bool
HgiMetalMacroShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    WriteIdentifier(ss);
    return true;
}

HgiMetalInterstageBlockShaderSection::HgiMetalInterstageBlockShaderSection(
        const std::string &blockIdentifier,
        const std::string &blockInstanceIdentifier,
        const HgiMetalStructTypeDeclarationShaderSection *structTypeDeclaration)
  : HgiMetalShaderSection(
      blockIdentifier,
      /* attributes = */HgiShaderSectionAttributeVector(),
      /* defaultValue = */std::string(),
      /* arraySize = */std::string(),
      blockInstanceIdentifier)
  , _structTypeDeclaration(structTypeDeclaration)
{
}

const HgiMetalStructTypeDeclarationShaderSection*
HgiMetalInterstageBlockShaderSection::GetStructTypeDeclaration() const
{
    return _structTypeDeclaration;
}

bool
HgiMetalInterstageBlockShaderSection::VisitScopeStructs(
    std::ostream &ss)
{
    _structTypeDeclaration->WriteDeclaration(ss);
    return true;
}

bool
HgiMetalInterstageBlockShaderSection::VisitScopeMemberDeclarations(
    std::ostream &ss)
{
    _structTypeDeclaration->WriteIdentifier(ss);
    ss << " ";
    WriteBlockInstanceIdentifier(ss);
    ss << ";\n";
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
