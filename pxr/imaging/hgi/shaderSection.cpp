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

#include "shaderSection.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiShaderSection::HgiShaderSection(
    const std::string &identifier,
    const HgiShaderSectionAttributeVector& attributes,
    const std::string &defaultValue,
    const std::string &arraySize,
    const std::string &blockInstanceIdentifier)
  : _identifierVar(identifier)
  , _attributes(attributes)
  , _defaultValue(defaultValue)
  , _arraySize(arraySize)
  , _blockInstanceIdentifier(blockInstanceIdentifier)
{
}

HgiShaderSection::~HgiShaderSection() = default;

void
HgiShaderSection::WriteType(std::ostream& ss) const
{
}

void
HgiShaderSection::WriteIdentifier(std::ostream& ss) const
{
    ss << _identifierVar;
}

void
HgiShaderSection::WriteBlockInstanceIdentifier(std::ostream& ss) const
{
    ss << _blockInstanceIdentifier;
}

void
HgiShaderSection::WriteDeclaration(std::ostream& ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    WriteArraySize(ss);
    ss << ";";
}

void
HgiShaderSection::WriteParameter(std::ostream& ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
}

void
HgiShaderSection::WriteArraySize(std::ostream& ss) const
{
    if (!_arraySize.empty()) {
        ss << "[" << _arraySize << "]";
    }
}

const HgiShaderSectionAttributeVector&
HgiShaderSection::GetAttributes() const
{
    return _attributes;
}

const std::string&
HgiShaderSection::_GetDefaultValue() const
{
    return _defaultValue;
}

HgiBaseGLShaderSection::HgiBaseGLShaderSection(
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

HgiBaseGLShaderSection::~HgiBaseGLShaderSection() = default;

void
HgiBaseGLShaderSection::WriteDeclaration(std::ostream &ss) const
{
    //If it has attributes, write them with corresponding layout
    //identifiers and indicies
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
HgiBaseGLShaderSection::WriteParameter(std::ostream &ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
    ss << ";";
}

bool
HgiBaseGLShaderSection::VisitGlobalIncludes(std::ostream &ss)
{
    return false;
}

bool
HgiBaseGLShaderSection::VisitGlobalMacros(std::ostream &ss)
{
    return false;
}

bool
HgiBaseGLShaderSection::VisitGlobalStructs(std::ostream &ss)
{
    return false;
}

bool HgiBaseGLShaderSection::VisitGlobalMemberDeclarations(std::ostream &ss)
{
    return false;
}

bool
HgiBaseGLShaderSection::VisitGlobalFunctionDefinitions(std::ostream &ss)
{
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
