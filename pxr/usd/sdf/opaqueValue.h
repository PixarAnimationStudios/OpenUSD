//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
