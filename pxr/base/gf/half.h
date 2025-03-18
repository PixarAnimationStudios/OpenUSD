//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_HALF_H
#define PXR_BASE_GF_HALF_H

/// \file gf/half.h
///
/// This header serves to simply bring in the half float datatype and
/// provide a hash_value function.  For documentation, of the half type,
/// please see the half header in ilmbase_half.h.

#include "pxr/pxr.h"
#include "pxr/base/gf/ilmbase_half.h"
#include "pxr/base/gf/ilmbase_halfLimits.h"
#include "pxr/base/gf/traits.h"

#include <cstddef>

PXR_NAMESPACE_OPEN_SCOPE

/// A 16-bit floating point data type.
using GfHalf = pxr_half::half;

namespace pxr_half {
    /// Overload hash_value for half.
    inline std::size_t hash_value(const half h) { return h.bits(); }
    // Explicitly delete hashing via implicit conversion of half to float
    std::size_t hash_value(float) = delete;
}

template <>
struct GfIsFloatingPoint<GfHalf> : 
    public std::integral_constant<bool, true>{};

PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXR_BASE_GF_HALF_H
