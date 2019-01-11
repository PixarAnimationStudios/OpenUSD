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
#ifndef VTKATANA_TRAITS_H
#define VTKATANA_TRAITS_H

#include "pxr/pxr.h"

#include <type_traits>

#include <FnAttribute/FnAttribute.h>

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/token.h"

#include "pxr/base/vt/array.h"

#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4h.h"

#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/matrix4f.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

extern TfEnvSetting<bool> VTKATANA_ENABLE_ZERO_COPY_ARRAYS;

/// We distinguish between two types of data that we want to
/// shuffle between Katana and Vt.  String and Numeric data.
/// String data consists of the 'string', 'token', and 'asset'
/// types, while numeric data consists of any floating point
/// or integral type supported by USD / Vt and their associated
/// vector and matrix forms.
///
/// Numeric types are broken down via two distinct vectors of
/// specialization.
///
/// Numeric types are either Scalar or Tuples, as determined by
/// the GetTupleSize traits. Tuple types may require an additional
/// transformation (usually a reinterpret_cast) to go to and
/// from Katana, but essentially as cheap to Copy and Map as
/// their Scalar siblings.
///
/// Numeric types are also either Castable or CopyRequired.
/// Castable Numeric types, can be translated into a native
/// Katana attribute value type (float, int, double),
/// requiring one less intermediate copy, and opening the door
/// to "zero copy" translations. Castability is determined
/// via the ScalarType of the numeric attribute, which means
/// the various Tuple variants (ie. GfVec3d, GfMatrix4f) can be
/// optimally translated.
///
/// String types are broken down into either HoldsString or
/// IsString, often requiring an intermediate conversion to
/// either std::string or c-string pointers for translation.

/// True if T is the same as std::string
template <typename T>
struct VtKatana_IsString : public std::is_same<T, std::string> {};

/// True if T holds onto and can be converted to a std::string
/// The mapping to string for SdfPath and TfToken are relatively clear, but
/// \sa VtKatana_GetText(const SdfAssetPath&) for more information
/// on how we extract a single string for that type.
template <typename T>
struct VtKatana_HoldsString
    : public std::integral_constant<bool,
                                    std::is_same<T, SdfPath>::value ||
                                        std::is_same<T, SdfAssetPath>::value ||
                                        std::is_same<T, TfToken>::value> {};

/// \sa VtKatana_IsString VtKatana_HoldsString
template <typename T>
struct VtKatana_IsOrHoldsString
    : public std::integral_constant<bool, VtKatana_IsString<T>::value ||
                                              VtKatana_HoldsString<T>::value> {
};

/// Return the underlying arithmetic type for the numeric value
template <typename T, typename = void>
struct VtKatana_GetNumericScalarType {
    typedef void type;
};

/// Arithmetic types are their own ScalarType
template <typename T>
struct VtKatana_GetNumericScalarType<
    T, typename std::enable_if<std::is_arithmetic<T>::value>::type> {
    typedef T type;
};

/// GfHalf types are their own ScalarType
template <>
struct VtKatana_GetNumericScalarType<GfHalf> {
    typedef GfHalf type;
};

/// Tuple types have a ScalarType typedeffed (ie. GfVec3f => float)
template <typename T>
struct VtKatana_GetNumericScalarType<
    T, typename std::enable_if<GfIsGfVec<T>::value || 
                                  GfIsGfMatrix<T>::value>::type> {
    typedef typename T::ScalarType type;
};

/// Numeric types are types represented by a single scalar or a memory-aligned
/// tuple of scalars
template <typename T, typename = void>
struct VtKatana_IsNumeric
    : public std::integral_constant<
          bool, std::is_same<typename VtKatana_GetNumericScalarType<T>::type,
                             GfHalf>::value ||
                    std::is_arithmetic<typename VtKatana_GetNumericScalarType<
                        T>::type>::value> {};

/// Every Numeric and String type can be mapped to a single Katana Attribute
/// Type
template <typename T, typename = void>
struct VtKatana_GetKatanaAttrType {
    typedef FnAttribute::NullAttribute type;
};

/// Strings and String Holders map to Katana StringAttributes
/// (ie. SdfAssetPath => StringAttribute)
template <typename T>
struct VtKatana_GetKatanaAttrType<
    T, typename std::enable_if<VtKatana_IsOrHoldsString<T>::value>::type> {
    typedef FnAttribute::StringAttribute type;
};

/// Numeric integral types map to Katana IntAttributes
/// (ie. GfVec3i => IntAttribute)
template <typename T>
struct VtKatana_GetKatanaAttrType<
    T, typename std::enable_if<
           VtKatana_IsNumeric<T>::value &&
           std::is_integral<
               typename VtKatana_GetNumericScalarType<T>::type>::value>::type> {
    typedef FnAttribute::IntAttribute type;
};

/// Float and Half numeric types map to Katana FloatAttributes
/// (ie. GfVec3h => FloatAttribute)
template <typename T>
struct VtKatana_GetKatanaAttrType<
    T, typename std::enable_if<
           VtKatana_IsNumeric<T>::value &&
           (std::is_same<typename VtKatana_GetNumericScalarType<T>::type,
                         float>::value ||
            std::is_same<typename VtKatana_GetNumericScalarType<T>::type,
                         GfHalf>::value)>::type> {
    typedef FnAttribute::FloatAttribute type;
};

/// Double numeric types map to Katana DoubleAttributes
/// (ie. GfVec3d => DoubleAttribute)
template <typename T>
struct VtKatana_GetKatanaAttrType<
    T, typename std::enable_if<
           VtKatana_IsNumeric<T>::value &&
           std::is_same<typename VtKatana_GetNumericScalarType<T>::type,
                        double>::value>::type> {
    typedef FnAttribute::DoubleAttribute type;
};

/// All Katana Data Attribute types (Float, String, Int, Double) have
/// an associated value_type member.
template <typename T,
          typename = typename std::enable_if<std::is_base_of<
              FnAttribute::DataAttribute,
              typename VtKatana_GetKatanaAttrType<T>::type>::value>::type>
struct VtKatana_GetKatanaAttrValueType {
    typedef typename VtKatana_GetKatanaAttrType<T>::type::value_type type;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
