//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_NDR_PROPERTY_H
#define PXR_USD_NDR_PROPERTY_H

/// \file ndr/property.h
///
/// \deprecated
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in sdr/property.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/api.h"
#include "pxr/usd/ndr/sdfTypeIndicator.h"
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
///
/// \deprecated
/// Deprecated in favor of SdrShaderProperty
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

    /// Gets this property's default value associated with the type of the
    /// property.
    /// 
    /// \sa GetType()
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
    ///
    /// \deprecated
    /// Deprecated in favor of
    /// SdrShaderProperty::CanConnectTo(SdrShaderProperty)
    NDR_API
    virtual bool CanConnectTo(const NdrProperty& other) const;

    /// @}


    /// \name Utilities
    /// @{

    /// Converts the property's type from `GetType()` into a
    /// `NdrSdfTypeIndicator`.
    ///
    /// Two scenarios can result: an exact mapping from property type to Sdf
    /// type, and an inexact mapping. In the first scenario,
    /// NdrSdfTypeIndicator will contain a cleanly-mapped Sdf type. In the
    /// second scenario, the NdrSdfTypeIndicator will contain an Sdf type
    /// set to `Token` to indicate an unclean mapping, and
    /// NdrSdfTypeIndicator::HasSdfType will return false.
    ///
    /// This base property class is generic and cannot know ahead of time how to
    /// perform this mapping reliably, thus it will always fall into the second
    /// scenario. It is up to specialized properties to perform the mapping.
    ///
    /// \sa GetDefaultValueAsSdfType()
    NDR_API
    virtual NdrSdfTypeIndicator GetTypeAsSdfType() const;

    /// Provides default value corresponding to the SdfValueTypeName returned 
    /// by GetTypeAsSdfType. 
    /// 
    /// Derived classes providing an implementation for GetTypeAsSdfType should
    /// also provide an implementation for this.
    ///
    /// \sa GetTypeAsSdfType()
    NDR_API
    virtual const VtValue& GetDefaultValueAsSdfType() const;

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

#endif // PXR_USD_NDR_PROPERTY_H
