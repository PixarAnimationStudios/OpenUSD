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
#ifndef PXR_BASE_VT_VISIT_VALUE_H
#define PXR_BASE_VT_VISIT_VALUE_H

#include "pxr/pxr.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_ValueVisitDetail {

// These two overloads do SFINAE to detect whether the visitor can be invoked
// with the given held type T.  If the visitor cannot be invoked with T, it is
// instead invoked with the VtValue itself.
template <class T, class Visitor,
    class = decltype(std::declval<Visitor>()(std::declval<T>()))>
auto
Visit(VtValue const &val, Visitor &&visitor, int) {
    return std::forward<Visitor>(visitor)(val.UncheckedGet<T>());
}

template <class T, class Visitor>
auto
Visit(VtValue const &val, Visitor &&visitor, ...) {
    return std::forward<Visitor>(visitor)(val);
}

} // Vt_ValueVisitDetail

/// Invoke \p visitor with \p value's held object if \p value holds an object of
/// one of the "known" value types (those in VT_VALUE_TYPES, see vt/types.h).
/// If \p value does not hold a known type, or if it is empty, or if \p visitor
/// cannot be called with an object of the held type, then call \p visitor with
/// \p value itself.  Note this means that \p visitor must be callable with a
/// VtValue argument.
///
/// VtVisitValue() can be lower overhead compared to a chained-if of
/// VtValue::IsHolding() calls, or a hash-table-lookup dispatch.  Additionally,
/// visitors can handle related types with a single case, rather than calling
/// out all types individually.  For example:
///
/// \code
/// // If the value holds an array return its size, otherwise size_t(-1).
/// struct GetArraySize {
///     template <class T>
///     size_t operator()(VtArray<T> const &array) const {
///         return array.size();
///     }
///     size_t operator()(VtValue const &val) const {
///         return size_t(-1);
///     }
/// };
///
/// VtVisitValue(VtValue(VtIntArray(123)), GetArraySize()) -> 123
/// VtVisitValue(VtValue(VtDoubleArray(234)), GetArraySize()) -> 234
/// VtVisitValue(VtValue(VtVec3fArray(345)), GetArraySize()) -> 345
/// VtVisitValue(VtValue("not-a-vt-array"), GetArraySize()) -> size_t(-1)
/// \endcode
///
/// Note that the visitor is invoked as a normal C++ call expression, so
/// implicit conversions and standard overload resolution (including controlling
/// overload resolution via techniques like enable_if) can take place.  For
/// example, consider the following, where the double-specific overload is
/// invoked for VtValues holding double, float, and GfHalf.
///
/// \code
/// struct AsDouble {
///     double operator()(double val) const {
///         return val;
///     }
///     double operator()(VtValue const &) const {
///         return std::numeric_limits<double>::quiet_NaN();
///     }
/// };
///
/// VtVisitValue(VtValue(1.23), AsDouble()) -> 1.23
/// VtVisitValue(VtValue(float(0.5f)), AsDouble()) -> 0.5
/// VtVisitValue(VtValue(GfHalf(1.5f)), AsDouble()) -> 1.5
/// VtVisitValue(VtValue("not-convertible-to-double"), AsDouble()) -> NaN.
/// \endcode
template <class Visitor>
auto VtVisitValue(VtValue const &value, Visitor &&visitor)
{
    // This generally gets the compiler to emit a jump table to dispatch
    // directly to the code for each known value type.
    switch (value.GetKnownValueTypeIndex()) {

// Cases for known types.
#define VT_CASE_FOR_TYPE_INDEX(r, unused, i, elem)                             \
        case i:                                                                \
            return Vt_ValueVisitDetail::Visit<VT_TYPE(elem)>(                  \
                value, std::forward<Visitor>(visitor), 0);                     \
            break;
BOOST_PP_SEQ_FOR_EACH_I(VT_CASE_FOR_TYPE_INDEX, ~, VT_VALUE_TYPES)
#undef VT_CASE_FOR_TYPE_INDEX
    
        default:
            // Invoke visitor with value itself.
            return Vt_ValueVisitDetail::Visit<VtValue>(
                value, std::forward<Visitor>(visitor), 0);
            break;
    };
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_VISIT_VALUE_H
