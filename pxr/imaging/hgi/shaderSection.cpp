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
    const std::string &defaultValue)
  : _identifierVar(identifier)
  , _attributes(attributes)
  , _defaultValue(defaultValue)
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


PXR_NAMESPACE_CLOSE_SCOPE
