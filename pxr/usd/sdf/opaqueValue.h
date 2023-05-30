//
// Copyright 2022 Pixar
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
#ifndef PXR_USD_SDF_OPAQUE_VALUE_H
#define PXR_USD_SDF_OPAQUE_VALUE_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"

#include <cstddef>
#include <iosfwd>


PXR_NAMESPACE_OPEN_SCOPE

/// In-memory representation of the value of an opaque attribute.
///
/// Opaque attributes cannot have authored values, but every typename in Sdf
/// must have a corresponding constructable C++ value type; SdfOpaqueValue is
/// the type associated with opaque attributes. Opaque values intentionally
/// cannot hold any information, cannot be parsed, and cannot be serialized to
/// a layer.
///
/// SdfOpaqueValue is also the type associated with group attributes. A group 
/// attribute is an opaque attribute that represents a group of other 
/// properties.
///
class SdfOpaqueValue final {};

inline bool
operator==(SdfOpaqueValue const &, SdfOpaqueValue const &)
{
    return true;
}

inline bool
operator!=(SdfOpaqueValue const &, SdfOpaqueValue const &)
{
    return false;
}

inline size_t hash_value(SdfOpaqueValue const &)
{
    // Use a nonzero constant here because some bad hash functions don't deal
    // with zero well. Chosen by fair dice roll.
    return 9;
}

SDF_API std::ostream& operator<<(std::ostream &, SdfOpaqueValue const &);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
