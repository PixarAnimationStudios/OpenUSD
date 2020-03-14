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
#ifndef PXR_BASE_TF_BIT_UTILS_H
#define PXR_BASE_TF_BIT_UTILS_H

/// \file tf/bitUtils.h
/// \ingroup group_tf_BasicMath

#include "pxr/pxr.h"

#include <cstddef>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// Compute the number of bits required to store the given number of values.
///
/// Note that the computed result for a number smaller or equal to zero is
/// undefined.  The input number can be any compile-time constant.
///
/// \ingroup group_tf_BasicMath
/// \hideinitializer
#define TF_BITS_FOR_VALUES(n) \
    Tf_NumBits<n-1>::type::value

template <size_t N, size_t SUM=0, size_t BIT=sizeof(N)*8/2>
struct Tf_NumBits
{
    // The result is computed by divide and conquer; for a given word N the
    // bit at position BIT divides the word in an upper and a lower half.
    // If the upper half contain any ones, then the result is SUM plus BIT
    // plus the result for the upper half.  If not, the result is SUM plus
    // the result for the lower half.
    typedef typename std::conditional<N >= (1ULL<<BIT),
        Tf_NumBits<(N>>BIT), SUM+BIT, BIT/2>,
        Tf_NumBits<N, SUM, BIT/2> >::type _func;
    typedef typename _func::type type;
};

template <size_t N, size_t SUM>
struct Tf_NumBits<N, SUM, 0>
{
    typedef std::integral_constant<size_t, SUM+1> type;
};

/// Compute the number of bits required to store the given number of (signed)
/// enum values.
///
/// \note This is intended to be used when storing enum values in a bitfield
/// without casting the enum type to an unsigned integer.  (At least GCC
/// considers enums to be signed and hence wastes one bit when all enumerants
/// are non-negative).
///
/// \ingroup group_tf_BasicMath
/// \hideinitializer
#define TF_BITS_FOR_ENUM_VALUES(n) \
    (TF_BITS_FOR_VALUES(n) + 1)

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_BASE_TF_BIT_UTILS_H */
