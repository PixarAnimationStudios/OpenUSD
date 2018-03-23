//
// Copyright 2016 Pixar
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
#ifndef SDF_VALUETYPENAME_H
#define SDF_VALUETYPENAME_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/token.h"
#include <boost/operators.hpp>
#include <iosfwd>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class TfEnum;
class TfType;
class VtValue;
class Sdf_ValueTypeImpl;

/// \struct SdfTupleDimensions
///
/// Represents the shape of a value type (or that of an element in an array).
///
struct SdfTupleDimensions 
    : boost::equality_comparable<SdfTupleDimensions> {
public:
    SdfTupleDimensions() : size(0) {}
    SdfTupleDimensions(size_t m) : size(1) { d[0] = m; }
    SdfTupleDimensions(size_t m, size_t n) : size(2) { d[0] = m; d[1] = n; }
    SdfTupleDimensions(size_t s[2]) : size(2) { d[0] = s[0]; d[1] = s[1]; }

    bool operator==(const SdfTupleDimensions& rhs) const;

public:
    size_t d[2];
    size_t size;
};

/// \class SdfValueTypeName
///
/// Represents a value type name, i.e. an attribute's type name.  Usually,
/// a value type name associates a string with a \c TfType and an optional
/// role, along with additional metadata.  A schema registers all known
/// value type names and may register multiple names for the same TfType
/// and role pair.  All name strings for a given pair are collectively
/// called its aliases.
///
/// A value type name may also represent just a name string, without a
/// \c TfType, role or other metadata.  This is currently used exclusively
/// to unserialize and re-serialize an attribute's type name where that
/// name is not known to the schema.
///
/// Because value type names can have aliases and those aliases may change
/// in the future, clients should avoid using the value type name's string
/// representation except to report human readable messages and when
/// serializing.  Clients can look up a value type name by string using
/// \c SdfSchemaBase::FindType() and shouldn't otherwise need the string.
/// Aliases compare equal, even if registered by different schemas.
/// 
class SdfValueTypeName
    : boost::equality_comparable<SdfValueTypeName, std::string
    , boost::equality_comparable<SdfValueTypeName, TfToken
    , boost::equality_comparable<SdfValueTypeName
    > > > {
public:
    /// Constructs an invalid type name.
    SDF_API
    SdfValueTypeName();

    /// Returns the type name as a token.  This should not be used for
    /// comparison purposes.
    SDF_API
    TfToken GetAsToken() const;

    /// Returns the \c TfType of the type.
    SDF_API
    const TfType& GetType() const;

    /// Returns the type's role.
    SDF_API
    const TfToken& GetRole() const;

    /// Returns the default value for the type.
    SDF_API
    const VtValue& GetDefaultValue() const;

    /// Returns the default unit enum for the type.
    SDF_API
    const TfEnum& GetDefaultUnit() const;

    /// Returns the scalar version of this type name if it's an array type
    /// name, otherwise returns this type name.  If there is no scalar type
    /// name then this returns the invalid type name.
    SDF_API
    SdfValueTypeName GetScalarType() const;

    /// Returns the array version of this type name if it's an scalar type
    /// name, otherwise returns this type name.  If there is no array type
    /// name then this returns the invalid type name.
    SDF_API
    SdfValueTypeName GetArrayType() const;

    /// Returns \c true iff this type is a scalar.  The invalid type is
    /// considered neither scalar nor array.
    SDF_API
    bool IsScalar() const;

    /// Returns \c true iff this type is an array.  The invalid type is
    /// considered neither scalar nor array.
    SDF_API
    bool IsArray() const;

    /// Returns the dimensions of the scalar value, e.g. 3 for a 3D point.
    SDF_API
    SdfTupleDimensions GetDimensions() const;

    /// Returns \c true if this type name is equal to \p rhs.  Aliases
    /// compare equal.
    SDF_API
    bool operator==(const SdfValueTypeName& rhs) const;

    /// Returns \c true if this type name is equal to \p rhs.  Aliases
    /// compare equal.  Avoid relying on this overload.
    SDF_API
    bool operator==(const std::string& rhs) const;

    /// Returns \c true if this type name is equal to \p rhs.  Aliases
    /// compare equal.  Avoid relying on this overload.
    SDF_API
    bool operator==(const TfToken& rhs) const;

    /// Returns a hash value for this type name.
    SDF_API
    size_t GetHash() const;

#if !defined(doxygen)
    class _Untyped;
    typedef Sdf_ValueTypeImpl const* const (SdfValueTypeName::*_UnspecifiedBoolType);
#endif

    /// Returns \c false iff this is a valid type.
    SDF_API
    bool operator!() const;

    /// Returns \c true iff this is a valid type.
    operator _UnspecifiedBoolType() const
    {
        return (!*this) ? 0 : &SdfValueTypeName::_impl;
    }

    /// Returns all aliases of the type name as tokens.  These should not
    /// be used for comparison purposes.
    SDF_API
    std::vector<TfToken> GetAliasesAsTokens() const;

private:
    friend class Sdf_ValueTypeRegistry;
    friend struct Sdf_ValueTypePrivate;

    SDF_API
    explicit SdfValueTypeName(const Sdf_ValueTypeImpl*);

private:
    const Sdf_ValueTypeImpl* _impl;
};

/// Functor for hashing a \c SdfValueTypeName.
struct SdfValueTypeNameHash {
    size_t operator()(const SdfValueTypeName& x) const
    {
        return x.GetHash();
    }
};

inline size_t
hash_value(const SdfValueTypeName& typeName)
{
    return typeName.GetHash();
}

SDF_API std::ostream& operator<<(std::ostream&, const SdfValueTypeName& typeName);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_VALUETYPENAME_H
