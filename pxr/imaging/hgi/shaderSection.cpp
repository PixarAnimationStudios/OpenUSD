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
    const std::string *attribute,
    const std::string *attributeIndex,
    const std::string *defaultValue)
    : _identifierVar(identifier)
    , _defaultValue(
        defaultValue != nullptr ?
            std::make_unique<std::string>(*defaultValue) : nullptr)
    , _attribute(
        attribute != nullptr ? 
            std::make_unique<std::string>(*attribute) : nullptr)
    , _attributeIndex(
        attributeIndex != nullptr ?
            std::make_unique<std::string>(*attributeIndex) : nullptr)
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
HgiShaderSection::WriteDeclaration(std::ostream& ss) const
{
    WriteType(ss);
    ss << " ";
    WriteIdentifier(ss);
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
HgiShaderSection::WriteAttributeWithIndex(std::ostream& ss) const
{
    if(_attribute != nullptr) {
        if(_attributeIndex != nullptr) {
            WriteParameter(ss);
            ss << "[[" << *_attribute
            << "(" << *_attributeIndex << ")" << "]]";
        } else {
            WriteParameter(ss);
            ss << "[[" << *_attribute << "]]";
        }
    }
}

const std::string*
HgiShaderSection::GetAttribute() const
{
    return _attribute.get();
}

const std::string*
HgiShaderSection::GetAttributeIndex() const
{
    return _attributeIndex.get();
}

const std::string*
HgiShaderSection::GetDefaultValue() const
{
    return _defaultValue.get();
}


PXR_NAMESPACE_CLOSE_SCOPE
