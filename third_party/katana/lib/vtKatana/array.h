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
#ifndef VTKATANA_ARRAY_H
#define VTKATANA_ARRAY_H

#include "pxr/pxr.h"
#include <FnAttribute/FnAttribute.h>

#include "pxr/base/vt/array.h"
#include "vtKatana/traits.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Maps a VtArray to a Katana attribute, minimizing intermediate copies.
///
/// The number of intermediate copies required to construct an attribute
/// is determined by the type traits internal to this library. As a general,
/// rule of thumb, if the precision of the source array type matches
/// the destination type, you can assume that no intermediate copies are
/// required. For example, a Vec3fArray shouldn't require intermediate
/// copies to construct a FloatAttribute, but a BoolArray requires
/// constructing an intermediate Int copy to construct an IntAttribute.
///
/// If VTKATANA_ENABLE_ZERO_COPY_ARRAYS is enabled, MapOrCopy is allowed to
/// utilize Katana's ZeroCopy feature to allow the data to be owned by a
/// VtArray
///
/// \note Because Katana hashes every attribute, zero copy data from
/// crate files will need to be read as soon as the attribute is created.
/// There's no way to cleverly stack crate and katana's zero copy features
/// to avoid or defer an attribute being copied into memory.
template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaMapOrCopy(
    const VtArray<T>& value);

/// Maps a series of \p times and \p values to a Katana attribute, minimizing
/// intermediate copies.
///
/// The number of intermediate copies required to construct an attribute
/// is determined by the type traits internal to this library. As a general,
/// rule of thumb, if the precision of the source array type matches
/// the destination type, you can assume that no intermediate copies are
/// required.  For example, a Vec3fArray shouldn't require intermediate
/// copies to construct a FloatAttribute, but a BoolArray requires
/// constructing an intermediate Int copy to construct an IntAttribute.
///
/// If VTKATANA_ENABLE_ZERO_COPY_ARRAYS is enabled, MapOrCopy is allowed to
/// utilize Katana's ZeroCopy feature to allow the data to be owned by the
/// VtArray.
///
/// \warn \p times MUST be sorted.
///
/// \note Because Katana hashes every attribute, zero copy data from
/// crate files will need to be read as soon as the attribute is created.
/// There's no way to cleverly stack crate and katana's zero copy features
/// to avoid or defer an attribute being copied into memory.
template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaMapOrCopy(
    const std::vector<float>& times,
    const typename std::vector<VtArray<T>>& values);

/// Maps \p timeToValueMap to a Katana attribute, minimizing
/// intermediate copies.
///
/// Internally, the map will be flattened into two vectors, so
/// \ref VtKatanaMapOrCopy(const std::vector<float>&,const std::vector<VtArray<T>>&)
/// is preferable if you already have sorted vectors.
template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaMapOrCopy(
    const typename std::map<float, VtArray<T>>& timeToValueMap);

/// Copy a VtArray to a Katana attribute, minimizing intermediate copies, but
/// disallowing any Zero Copy features the type might support.
template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaCopy(
    const VtArray<T>& value);

/// Copy a series of VtArray time samples to a Katana attribute, minimizing
/// intermediate copies, but disallowing any Zero Copy features the type
/// might support.
///
/// \warn \p times MUST be sorted.
template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaCopy(
    const std::vector<float>& times,
    const typename std::vector<VtArray<T>>& values);

/// Copy \p timeToValueMap to a Katana attribute, minimizing
/// intermediate copies, but disallowing any Zero Copy features the type
/// might support.
///
/// Internally, the map will be flattened into two vectors, so
/// \ref VtKatanaCopy(const std::vector<float>&,const std::vector<VtArray<T>>&)
/// is preferable if you already have sorted vectors.
template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaCopy(
    const typename std::map<float, VtArray<T>>& timeToValueMap);

/// Create a VtArray from the \p attribute array nearest to \sample
///
/// The number of intermediate copies required to construct an attribute
/// is determined by the type traits internal to this library. As a general,
/// rule of thumb, if the precision of the source array type matches
/// the destination type, you can assume that no intermediate copies are
/// required.  For example, a FloatAttribute shouldn't require intermediate
/// copies to construct a Vec3fArray, but an IntAttribute requires
/// constructing an intermediate copy to construct an BoolArray.
///
/// If the VTKATANA_ENABLE_ZERO_COPY_ARRAYS env settng is enabled,
/// this returns a VtArray with an attribute holder pointing to the
/// originating attribute.  For vec and matrix types, the attribute
/// must match the dimensionality of the of the Element to be succesfully
/// constructed.
///
/// \note A reference to the attribute is retained until the array is
/// uniquified by calling any non-const method on the array. Since an
/// attribute stores multiple time samples, it is technically possible
/// for you to hold onto more data than you intended. However, the
/// number of time samples in general is small, so this shouldn't be
/// an issue, but if this is of concern, use VtKatanaCopy instead.
template <typename T>
const VtArray<T> VtKatanaMapOrCopy(
        const typename VtKatana_GetKatanaAttrType<T>::type& attr,
        float sample = 0.0f);

/// Copy a single sample from a Katana attribute to a VtArray, minizing
/// intermediate copies, but disallowing any Zero Copy features the type
/// might support.
template <typename T>
const VtArray<T> VtKatanaCopy(
    const typename VtKatana_GetKatanaAttrType<T>::type& attr,
    float sample = 0.0f);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
