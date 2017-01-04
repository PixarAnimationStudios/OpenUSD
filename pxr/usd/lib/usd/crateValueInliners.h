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
#ifndef USD_CRATE_VALUE_INLINERS_H
#define USD_CRATE_VALUE_INLINERS_H

#include "pxr/base/gf/traits.h"

#include <type_traits>
#include <limits>
#include <cstdint>
#include <cstring>

namespace Usd_CrateValueInliners
{

// Return true and set \p *dst if \p src can be exactly represented as a Dst
// instance.  This only works for numeric types, and it checks range before
// doing the conversion.
template <class Src, class Dst>
inline bool _IsExactlyRepresented(Src const &src, Dst *dst) {
    Src min = static_cast<Src>(std::numeric_limits<Dst>::min());
    Src max = static_cast<Src>(std::numeric_limits<Dst>::max());
    if (min <= src && src <= max &&
        static_cast<Src>(static_cast<Dst>(src)) == src) {
        *dst = static_cast<Dst>(src);
        return true;
    }
    return false;
}

// Base case templates.
template <class T> bool _EncodeInline(T, ...) { return false; }
template <class T> void _DecodeInline(T *, ...) { }

////////////////////////////////////////////////////////////////////////
// Inline double as float if possible.
template <class FP>
typename std::enable_if<std::is_floating_point<FP>::value, bool>::type
_EncodeInline(FP fp, uint32_t *ival) {
    // If fp is representable exactly as float, encode as inline float.
    float f;
    if (_IsExactlyRepresented(fp, &f)) {
        memcpy(ival, &f, sizeof(f));
        return true;
    }
    return false;
}
template <class FP>
typename std::enable_if<std::is_floating_point<FP>::value>::type
_DecodeInline(FP *fp, uint32_t ival) {
    float f;
    memcpy(&f, &ival, sizeof(f));
    *fp = static_cast<FP>(f);
}

////////////////////////////////////////////////////////////////////////
// Inline integral as int if possible.
template <class INT>
typename std::enable_if<std::is_integral<INT>::value, bool>::type
_EncodeInline(INT i, uint32_t *ival) {
    // If i is in-range for (u)int32_t, encode as such.
    using int_t = typename std::conditional<
        std::is_signed<INT>::value, int32_t, uint32_t>::type;
    int_t rep;
    if (_IsExactlyRepresented(i, &rep)) {
        memcpy(ival, &rep, sizeof(rep));
        return true;
    }
    return false;
}
template <class INT>
typename std::enable_if<std::is_integral<INT>::value>::type
_DecodeInline(INT *i, uint32_t ival) {
    using int_t = typename std::conditional<
        std::is_signed<INT>::value, int32_t, uint32_t>::type;
    int_t tmp;
    memcpy(&tmp, &ival, sizeof(tmp));
    *i = static_cast<INT>(tmp);
}

////////////////////////////////////////////////////////////////////////
// Inline GfVecs when their components are exactly represented by int8_t.
template <class T>
typename std::enable_if<GfIsGfVec<T>::value, bool>::type
_EncodeInline(T vec, uint32_t *out) {
    // If each component of the vector can be represented by an int8_t, we can
    // inline it.
    static_assert(T::dimension <= 4, "Vec dimension cannot exceed 4.");
    int8_t ivec[T::dimension];
    for (int i = 0; i != T::dimension; ++i) {
        if (!_IsExactlyRepresented(vec[i], &ivec[i]))
            return false;
    }
    // All components exactly represented as int8_t, can inline.
    memcpy(out, ivec, sizeof(ivec));
    return true;
}
template <class T>
typename std::enable_if<GfIsGfVec<T>::value>::type
_DecodeInline(T *vec, uint32_t in) {
    int8_t ivec[T::dimension];
    memcpy(ivec, &in, sizeof(ivec));
    for (int i = 0; i != T::dimension; ++i) {
        (*vec)[i] = static_cast<typename T::ScalarType>(ivec[i]);
    }
}

////////////////////////////////////////////////////////////////////////
// Inline GfMatrices when they are all zeros off the diagonal and the diagonal
// entries are exactly represented by int8_t.
template <class Matrix>
typename std::enable_if<GfIsGfMatrix<Matrix>::value, bool>::type
_EncodeInline(Matrix m, uint32_t *out) {
    static_assert(Matrix::numRows == Matrix::numColumns,
                  "Requires square matrices");
    static_assert(Matrix::numRows <= 4,
                  "Matrix dimension cannot exceed 4");

    int8_t diag[Matrix::numRows];
    for (int i = 0; i != Matrix::numRows; ++i) {
        for (int j = 0; j != Matrix::numColumns; ++j) {
            if (((i != j) && m[i][j] != 0) ||
                ((i == j) && !_IsExactlyRepresented(m[i][j], &diag[i]))) {
                return false;
            }
        }
    }

    // All zeros off diagonal and diagonal is exactly represented by int8_t's --
    // store inline.
    memcpy(out, diag, sizeof(diag));
    return true;
}
template <class Matrix>
typename std::enable_if<GfIsGfMatrix<Matrix>::value>::type
_DecodeInline(Matrix *m, uint32_t in) {
    int8_t diag[Matrix::numRows];
    memcpy(diag, &in, sizeof(diag));
    *m = Matrix(1);
    for (int i = 0; i != Matrix::numRows; ++i) {
        (*m)[i][i] = static_cast<typename Matrix::ScalarType>(diag[i]);
    }
}

////////////////////////////////////////////////////////////////////////
// Encode VtDictionary inline if it's empty.
inline bool
_EncodeInline(VtDictionary const &dict, uint32_t *ival) {
    if (dict.empty()) {
        *ival = 0;
        return true;
    }
    return false;
}
inline void
_DecodeInline(VtDictionary *dict, uint32_t ival) {
    *dict = VtDictionary();
}

} // Usd_CrateValueInliners

#endif // USD_CRATE_VALUE_INLINERS_H

