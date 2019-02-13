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
#ifndef VTKATANA_INTERNALTRAITS_H
#define VTKATANA_INTERNALTRAITS_H

#include "pxr/pxr.h"
#include "vtKatana/traits.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Gets the 'tuple size' of T, or number of elements a numeric type contains.
/// See the specializations below for more info
template <typename T, typename = void>
struct VtKatana_GetNumericTupleSize {
    /// TODO: Can we remove this so only numeric types have tuple sizes defined?
    static const size_t value = 1;
};

/// Arithmetic types trivially have a tuple size of 1.
template <typename T>
struct VtKatana_GetNumericTupleSize<
    T, typename std::enable_if<std::is_arithmetic<T>::value>::type> {
    static const size_t value = 1;
};

/// GfHalf has a tuple sizes of 1
template <>
struct VtKatana_GetNumericTupleSize<GfHalf> {
    static const size_t value = 1;
};

/// Vector types derive tuple sizes from their dimension (ie. GfVec3f => 3)
template <typename T>
struct VtKatana_GetNumericTupleSize<
    T, typename std::enable_if<GfIsGfVec<T>::value>::type> {
    static const size_t value = T::dimension;
};

/// Matrix types derive tuple sizes from their row and column count (ie. GfVec4f
/// =>
/// 16)
template <typename T>
struct VtKatana_GetNumericTupleSize<
    T, typename std::enable_if<GfIsGfMatrix<T>::value>::type> {
    static const size_t value = T::numRows * T::numColumns;
};

/// This trait is highly specialized for GfHalf types.
/// GfHalf based types are the only types defined as
/// being both Tuples and Requiring Copy. This means we
/// need to cast the Katana float* to a GfVec{2,3,4}f*
/// before copying to the destination GfVec{2,3,4}h
/// If other types begin to need this, it might be
/// worth rethinking how specific this is.
template <typename T>
struct VtKatana_GetNumericCopyTuplePeer {
    typedef void type;
};

template <>
struct VtKatana_GetNumericCopyTuplePeer<GfVec2h> {
    typedef GfVec2f type;
};

template <>
struct VtKatana_GetNumericCopyTuplePeer<GfVec3h> {
    typedef GfVec3f type;
};

template <>
struct VtKatana_GetNumericCopyTuplePeer<GfVec4h> {
    typedef GfVec4f type;
};

/// Castable numeric types don't require intermediate copies, and allows
/// for ZeroCopy behavior.
template <typename T>
struct VtKatana_IsNumericCastable
    : public std::integral_constant<
          bool,
          VtKatana_IsNumeric<T>::value &&
              std::is_same<
                  typename VtKatana_GetNumericScalarType<T>::type,
                  typename VtKatana_GetKatanaAttrValueType<T>::type>::value> {};

/// If true, the element represents a single value (ie. float or double)
/// and not a tuple (ie. GfVec3h).
template <typename T>
struct VtKatana_IsNumericScalar
    : public std::integral_constant<
          bool, VtKatana_IsNumeric<T>::value &&
                    VtKatana_GetNumericTupleSize<T>::value == 1> {};

/// If true, the element represents a matrix or vector value.
template <typename T>
struct VtKatana_IsNumericTuple
    : public std::integral_constant<bool,
                                    VtKatana_IsNumeric<T>::value &&
                                        !VtKatana_IsNumericScalar<T>::value> {};

/// \sa VtKatana_IsNumericCastable VtKatana_IsNumericScalar
template <typename T>
struct VtKatana_IsNumericCastableScalar
    : public std::integral_constant<bool,
                                    VtKatana_IsNumericCastable<T>::value &&
                                        VtKatana_IsNumericScalar<T>::value> {};

/// \sa VtKatana_IsNumericCastable VtKatana_IsNumericTuple
template <typename T>
struct VtKatana_IsNumericCastableTuple
    : public std::integral_constant<bool,
                                    VtKatana_IsNumericCastable<T>::value &&
                                        VtKatana_IsNumericTuple<T>::value> {};

/// If true, this type always requires an intermediate copy, and cannot
/// support the zero copy feature set.
template <typename T>
struct VtKatana_IsNumericCopyRequired
    : public std::integral_constant<bool,
                                    VtKatana_IsNumeric<T>::value &&
                                        !VtKatana_IsNumericCastable<T>::value> {
};

/// \sa VtKatana_IsNumericCopyRequired VtKatana_IsNumericScalar
template <typename T>
struct VtKatana_IsNumericCopyRequiredScalar
    : public std::integral_constant<bool,
                                    VtKatana_IsNumericCopyRequired<T>::value &&
                                        VtKatana_IsNumericScalar<T>::value> {};

/// \sa VtKatana_IsNumericCopyRequired VtKatana_IsNumericTuple
template <typename T>
struct VtKatana_IsNumericCopyRequiredTuple
    : public std::integral_constant<bool,
                                    VtKatana_IsNumericCopyRequired<T>::value &&
                                        VtKatana_IsNumericTuple<T>::value> {};

/// String types require a template specialization to get access to the
/// internal c-string The lifetime of the resulting c-string is tied to
/// the lifetime of the input parameter.
///
/// We use GetText to refer to c-strings for symmetry with the rest
/// of the pxr APIs.
template <typename T, typename = typename std::enable_if<
                          VtKatana_IsOrHoldsString<T>::value>::type>
const char* VtKatana_GetText(const T&);

/// Retrieves the c-string held by \p string
template <>
const char* VtKatana_GetText<std::string>(const std::string& string);

/// Retrieves the c-string held by \p token
template <>
const char* VtKatana_GetText<TfToken>(const TfToken& token);

/// Retrieves the c-string held by the resolved \p assetPath if possible,
/// otherwise the unresolved asset path.
template <>
const char* VtKatana_GetText<SdfAssetPath>(const SdfAssetPath& assetPath);

/// Retrieves the c-string held by \p path,
template <>
const char* VtKatana_GetText<SdfPath>(const SdfPath& path);

/// Returns the underlying data pointed to by array.
template <typename T>
typename std::enable_if<VtKatana_IsNumericScalar<T>::value, const T*>::type
VtKatana_GetScalarPtr(const VtArray<T>& array) {
    return array.cdata();
};

/// Returns the underlying data pointed to by array, cast to its scalar type.
template <typename T>
typename std::enable_if<
    VtKatana_IsNumericTuple<T>::value,
    const typename VtKatana_GetNumericScalarType<T>::type*>::type
VtKatana_GetScalarPtr(const VtArray<T>& array) {
    typedef typename VtKatana_GetNumericScalarType<T>::type ScalarType;
    TF_VERIFY(array.size() > 0);
    return array.cdata()->data();
};

/// Returns the underlying data pointed to by array.
template <typename T>
typename std::enable_if<VtKatana_IsNumericCastableScalar<T>::value,
                        const T*>::type
VtKatana_GetVtPtr(
    const typename VtKatana_GetKatanaAttrType<T>::type::array_type& sample) {
    return sample.data();
};

/// Check to make sure the katana scalar pt is aligned.
template <typename T>
typename std::enable_if<VtKatana_IsNumericCastable<T>::value,
                        bool>::type
VtKatana_IsSampleAligned( 
    const typename VtKatana_GetKatanaAttrType<T>::type::array_type& sample){
    auto data = sample.data();
    return ((size_t)data) % alignof(T) == 0;
}

/// Returns the underlying data pointed to by array, cast to its native vt
/// type. You must use \sa VtKatana_IsSampleAligned to verify alignment
/// before calling this. We've seen cases where double attributes are
/// not aligned.
template <typename T>
typename std::enable_if<VtKatana_IsNumericCastableTuple<T>::value,
                        const T*>::type
VtKatana_GetVtPtr(
    const typename VtKatana_GetKatanaAttrType<T>::type::array_type& sample) {
    TF_VERIFY(VtKatana_IsSampleAligned<T>(sample));
    return reinterpret_cast<const T*>(sample.data());
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
