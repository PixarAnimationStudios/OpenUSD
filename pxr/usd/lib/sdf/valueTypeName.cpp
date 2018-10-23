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

#include "pxr/pxr.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/sdf/valueTypePrivate.h"

#include <boost/functional/hash.hpp>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

template <typename C, typename V>
bool
IsValueIn(const C& container, V value)
{
    for (const auto& element : container) {
        if (element == value) {
            return true;
        }
    }
    return false;
}

} // anonymous namespace


//
// SdfTupleDimensions
//

bool
SdfTupleDimensions::operator==(const SdfTupleDimensions& rhs) const
{
    if (size != rhs.size) {
        return false;
    }
    if (size >= 1 && d[0] != rhs.d[0]) {
        return false;
    }
    if (size >= 2 && d[1] != rhs.d[1]) {
        return false;
    }
    return true;
}


//
// SdfValueTypeName
//

SdfValueTypeName::SdfValueTypeName() :
    _impl(Sdf_ValueTypePrivate::GetEmptyTypeName())
{
    // Do nothing
}

SdfValueTypeName::SdfValueTypeName(const Sdf_ValueTypeImpl* impl) : _impl(impl)
{
    // Do nothing
}

TfToken
SdfValueTypeName::GetAsToken() const
{
    return _impl->name;
}

const TfType&
SdfValueTypeName::GetType() const
{
    return _impl->type->type;
}

const std::string& 
SdfValueTypeName::GetCPPTypeName() const
{
    return _impl->type->cppTypeName;
}

const TfToken&
SdfValueTypeName::GetRole() const
{
    return _impl->type->role;
}

const VtValue&
SdfValueTypeName::GetDefaultValue() const
{
    return _impl->type->value;
}

const TfEnum&
SdfValueTypeName::GetDefaultUnit() const
{
    return _impl->type->unit;
}

SdfValueTypeName
SdfValueTypeName::GetScalarType() const
{
    return SdfValueTypeName(_impl->scalar);
}

SdfValueTypeName
SdfValueTypeName::GetArrayType() const
{
    return SdfValueTypeName(_impl->array);
}

bool
SdfValueTypeName::IsScalar() const
{
    return *this && _impl == _impl->scalar;
}

bool
SdfValueTypeName::IsArray() const
{
    return *this && _impl == _impl->array;
}

SdfTupleDimensions
SdfValueTypeName::GetDimensions() const
{
    return _impl->type->dim;
}

bool
SdfValueTypeName::operator==(const SdfValueTypeName& rhs) const
{
    // Do equality comparisons on core type to ensure that
    // equivalent type names from different registries compare
    // equal. The registry ensures that type and role are
    // the only things we need to look at here.
    return (_impl->type->type == rhs._impl->type->type && 
            _impl->type->role == rhs._impl->type->role);
}

bool
SdfValueTypeName::operator==(const std::string& rhs) const
{
    return IsValueIn(_impl->type->aliases, rhs);
}

bool
SdfValueTypeName::operator==(const TfToken& rhs) const
{
    return IsValueIn(_impl->type->aliases, rhs);
}

size_t
SdfValueTypeName::GetHash() const
{
    // See comment in operator==.
    size_t hash = 0;
    boost::hash_combine(hash, TfHash()(_impl->type->type));
    boost::hash_combine(hash, _impl->type->role.Hash());
    return hash;
}

bool
SdfValueTypeName::_IsEmpty() const
{
    return _impl == Sdf_ValueTypePrivate::GetEmptyTypeName();
}

std::vector<TfToken>
SdfValueTypeName::GetAliasesAsTokens() const
{
    return _impl->type->aliases;
}

std::ostream&
operator<<(std::ostream& s, const SdfValueTypeName& typeName)
{
    return s << typeName.GetAsToken().GetString();
}

PXR_NAMESPACE_CLOSE_SCOPE
