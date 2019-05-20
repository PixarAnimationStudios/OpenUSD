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

#ifndef NDR_PROPERTY_H
#define NDR_PROPERTY_H

/// \file ndr/property.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/ndr/declare.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class NdrProperty
///
/// Represents a property (input or output) that is part of a `NdrNode`
/// instance.
///
/// A property must have a name and type, but may also specify a host of
/// additional metadata. Instances can also be queried to determine if another
/// `NdrProperty` instance can be connected to it.
///
/// In almost all cases, this class will not be used directly. More specialized
/// properties can be created that derive from `NdrProperty`; those specialized
/// properties can add their own domain-specific data and methods.
class NdrProperty
{
public:
    /// Constructor.
    NDR_API
    NdrProperty(
        const TfToken& name,
        const TfToken& type,
        const VtValue& defaultValue,
        bool isOutput,
        size_t arraySize,
        bool isDynamicArray,
        const NdrTokenMap& metadata
    );

    /// Destructor.
    NDR_API
    virtual ~NdrProperty();

    /// \name The Basics
    /// @{

    /// Gets the name of the property.
    NDR_API
    const TfToken& GetName() const { return _name; }

    /// Gets the type of the property.
    NDR_API
    const TfToken& GetType() const { return _type; }

    /// Gets this property's default value.
    NDR_API
    const VtValue& GetDefaultValue() const { return _defaultValue; }

    /// Whether this property is an output.
    NDR_API
    bool IsOutput() const { return _isOutput; }

    /// Whether this property's type is an array type.
    NDR_API
    bool IsArray() const { return (_arraySize > 0) || _isDynamicArray; }

    /// Whether this property's array type is dynamically-sized.
    NDR_API
    bool IsDynamicArray() const { return _isDynamicArray; };

    /// Gets this property's array size.
    ///
    /// If this property is a fixed-size array type, the array size is returned.
    /// In the case of a dynamically-sized array, this method returns the array
    /// size that the parser reports, and should not be relied upon to be
    /// accurate. A parser may report -1 for the array size, for example, to
    /// indicate a dynamically-sized array. For types that are not a fixed-size
    /// array or dynamic array, this returns 0.
    NDR_API
    int GetArraySize() const { return _arraySize; }

    /// Gets a string with basic information about this property. Helpful for
    /// things like adding this property to a log.
    NDR_API
    virtual std::string GetInfoString() const;

    /// @}


    /// \name Metadata
    /// The metadata returned here is a direct result of what the parser plugin
    /// is able to determine about the node. See the documentation for a
    /// specific parser plugin to get help on what the parser is looking for to
    /// populate these values.
    /// @{

    /// All of the metadata that came from the parse process.
    NDR_API
    virtual const NdrTokenMap& GetMetadata() const { return _metadata; }

    /// @}


    /// \name Connection Information
    /// @{

    /// Whether this property can be connected to other properties.
    NDR_API
    virtual bool IsConnectable() const;

    /// Determines if this property can be connected to the specified property.
    NDR_API
    virtual bool CanConnectTo(const NdrProperty& other) const;

    /// @}


    /// \name Utilities
    /// @{

    /// Converts the property's type from `GetType()` into a `SdfValueTypeName`.
    ///
    /// Two scenarios can result: an exact mapping from property type to Sdf
    /// type, and an inexact mapping. In the first scenario, the first element
    /// in the pair will be the cleanly-mapped Sdf type, and the second element,
    /// a TfToken, will be empty. In the second scenario, the Sdf type will be
    /// set to `Token` to indicate an unclean mapping, and the second element
    /// will be set to the original type returned by `GetType()`.
    ///
    /// This base property class is generic and cannot know ahead of time how to
    /// perform this mapping reliably, thus it will always fall into the second
    /// scenario. It is up to specialized properties to perform the mapping.
    NDR_API
    virtual const SdfTypeIndicator GetTypeAsSdfType() const;

    /// @}

protected:
    NdrProperty& operator=(const NdrProperty&) = delete;

    TfToken _name;
    TfToken _type;
    VtValue _defaultValue;
    bool _isOutput;
    size_t _arraySize;
    bool _isDynamicArray;
    bool _isConnectable;
    NdrTokenMap _metadata;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // NDR_PROPERTY_H
