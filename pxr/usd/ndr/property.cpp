//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ndr/node.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/ndr/property.h"
#include "pxr/usd/sdf/types.h"

PXR_NAMESPACE_OPEN_SCOPE

NdrProperty::NdrProperty(
    const TfToken& name,
    const TfToken& type,
    const VtValue& defaultValue,
    bool isOutput,
    size_t arraySize,
    bool isDynamicArray,
    const NdrTokenMap& metadata)
    : _name(name), _type(type), _defaultValue(defaultValue),
      _isOutput(isOutput), _arraySize(arraySize),
      _isDynamicArray(isDynamicArray), _isConnectable(true), _metadata(metadata)
{
}

NdrProperty::~NdrProperty()
{
    // nothing yet
}

std::string
NdrProperty::GetInfoString() const
{
    return TfStringPrintf(
        "%s (type: '%s'); %s",
        _name.GetText(), _type.GetText(), _isOutput ? "output" : "input"
    );
}

bool
NdrProperty::IsConnectable() const
{
    // Specialized nodes can define more complex rules here. Assume that all
    // inputs can accept connections.
    return _isConnectable && !_isOutput;
}

bool
NdrProperty::CanConnectTo(const NdrProperty& other) const
{
    // Outputs cannot connect to outputs and vice versa
    if (_isOutput == other.IsOutput()) {
        return false;
    }

    // The simplest implementation of this is to compare the types and see if
    // they are the same. Specialized nodes can define more complicated rules.
    return _type == other.GetType();
}

const NdrSdfTypeIndicator
NdrProperty::GetTypeAsSdfType() const {
    return std::make_pair(
        SdfValueTypeNames->Token,
        _type
    );
}

const VtValue&
NdrProperty::GetDefaultValueAsSdfType() const {
    static const VtValue& emptyValue = VtValue();
    return emptyValue;
}

PXR_NAMESPACE_CLOSE_SCOPE
