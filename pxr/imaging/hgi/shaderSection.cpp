//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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


PXR_NAMESPACE_CLOSE_SCOPE
