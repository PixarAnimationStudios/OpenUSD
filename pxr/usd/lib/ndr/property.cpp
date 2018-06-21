//
// Copyright 2018 Pixar
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

const SdfTypeIndicator
NdrProperty::GetTypeAsSdfType() const {
    return std::make_pair(
        SdfValueTypeNames->Token,
        _type
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
