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
#include "pxr/pxr.h"
#include "vtKatana/array.h"

#include "vtKatana/internalFromVt.h"
#include "vtKatana/internalToVt.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace VtKatana_Internal {

/// Creates an empty attribute with the specified tuple size.
template <typename AttrType>
AttrType EmptyAttr(size_t tupleSize) {
    // Need an explicit ptr to avoid ambiguous overload
    const typename AttrType::value_type* ptr = NULL;
    return AttrType(ptr, 0ul, tupleSize);
}

/// Creates an attribute suitable for return in the event of a failure.
/// This an empty attribute with tupleSize 0.
template <typename AttrType>
AttrType FailureAttr() {
    return AttrType();
}
}

template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaMapOrCopy(
    const VtArray<T>& value) {
    typedef typename VtKatana_GetKatanaAttrType<T>::type AttrType;
    if (value.empty()) {
        return VtKatana_Internal::EmptyAttr<AttrType>(
            VtKatana_GetNumericTupleSize<T>::value);
    }
    return VtKatana_Internal::VtKatana_FromVtConversion<T>::MapInternal(value);
}

template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaMapOrCopy(
    const std::vector<float>& times,
    const typename std::vector<VtArray<T>>& values) {
    typedef typename VtKatana_GetKatanaAttrType<T>::type AttrType;
    if (times.size() != values.size()) {
        TF_CODING_ERROR(
            "'times' array size doesn't match 'values' array shape");
        return VtKatana_Internal::FailureAttr<AttrType>();
    }
    if (!std::is_sorted(times.begin(), times.end())) {
        TF_CODING_ERROR("'times' must be sorted.");
        return VtKatana_Internal::FailureAttr<AttrType>();
    }

    if (values.empty()) {
        return VtKatana_Internal::EmptyAttr<AttrType>(
            VtKatana_GetNumericTupleSize<T>::value);
    }
    if (values.front().empty()) {
        return VtKatana_Internal::EmptyAttr<AttrType>(
            VtKatana_GetNumericTupleSize<T>::value);
    }
    auto checkSizes = [&values](const VtArray<T>& array) {
        return values.front().size() == array.size();
    };
    if (not std::all_of(values.begin() + 1, values.end(), checkSizes)) {
        TF_CODING_ERROR("'values' topology is varying.");
        return VtKatana_Internal::FailureAttr<AttrType>();
    }
    return VtKatana_Internal::VtKatana_FromVtConversion<T>::MapInternalMultiple(
        times, values);
}

template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaCopy(
    const VtArray<T>& value) {
    typedef typename VtKatana_GetKatanaAttrType<T>::type AttrType;
    if (value.empty()) {
        return VtKatana_Internal::EmptyAttr<AttrType>(
            VtKatana_GetNumericTupleSize<T>::value);
    }
    return VtKatana_Internal::VtKatana_FromVtConversion<T>::Copy(value);
}

template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaCopy(
    const std::vector<float>& times,
    const typename std::vector<VtArray<T>>& values) {
    typedef typename VtKatana_GetKatanaAttrType<T>::type AttrType;
    if (times.size() != values.size()) {
        TF_CODING_ERROR(
            "'times' array size doesn't match 'values' array shape");
        return VtKatana_Internal::FailureAttr<AttrType>();
    }
    if (!std::is_sorted(times.begin(), times.end())) {
        TF_CODING_ERROR("'times' must be sorted.");
        return VtKatana_Internal::FailureAttr<AttrType>();
    }
    if (values.empty()) {
        return VtKatana_Internal::EmptyAttr<AttrType>(
            VtKatana_GetNumericTupleSize<T>::value);
    }
    if (values.front().empty()) {
        return VtKatana_Internal::EmptyAttr<AttrType>(
            VtKatana_GetNumericTupleSize<T>::value);
    }
    auto checkSizes = [&values](const VtArray<T>& array) {
        return values.front().size() == array.size();
    };
    if (not std::all_of(values.begin() + 1, values.end(), checkSizes)) {
        TF_CODING_ERROR("'values' topology is varying.");
        return VtKatana_Internal::FailureAttr<AttrType>();
    }
    return VtKatana_Internal::VtKatana_FromVtConversion<T>::Copy(times, values);
}

template <typename T>
const VtArray<T> VtKatanaMapOrCopy(
    const typename VtKatana_GetKatanaAttrType<T>::type& attr,
    float sample) {
    if (attr.getSamples().size() < 1) {
        TF_CODING_ERROR("Cannot map attribute.  Attribute has no samples.");
        return VtArray<T>();
    }
    return VtKatana_Internal::VtKatana_ToVtConversion<T>::MapInternal(attr,
                                                                      sample);
}

template <typename T>
const VtArray<T> VtKatanaCopy(
    const typename VtKatana_GetKatanaAttrType<T>::type& attr,
    float sample) {
    if (attr.getSamples().size() < 1) {
        TF_CODING_ERROR("Cannot copy attribute.  Attribute has no samples.");
        return VtArray<T>();
    }
    auto nearestSample = attr.getNearestSample(sample);
    return VtKatana_Internal::VtKatana_ToVtConversion<T>::Copy(nearestSample);
}

template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaMapOrCopy(
    const typename std::map<float, VtArray<T>>& timeToValueMap) {
    std::vector<float> times;
    std::vector<VtArray<T>> values;
    for (const auto& entry : timeToValueMap) {
        times.push_back(entry.first);
        values.push_back(entry.second);
    }
    return VtKatanaMapOrCopy(times, values);
}

template <typename T>
typename VtKatana_GetKatanaAttrType<T>::type VtKatanaCopy(
    const typename std::map<float, VtArray<T>>& timeToValueMap) {
    std::vector<float> times;
    std::vector<VtArray<T>> values;
    for (const auto& entry : timeToValueMap) {
        times.push_back(entry.first);
        values.push_back(entry.second);
    }
    return VtKatanaCopy(times, values);
}

#define VTKATANA_DEFINE_MAP_AND_COPY(T)                                    \
    template typename VtKatana_GetKatanaAttrType<T>::type                  \
        VtKatanaMapOrCopy<T>(                                              \
            const VtArray<T>& value);                                      \
    template typename VtKatana_GetKatanaAttrType<T>::type                  \
        VtKatanaMapOrCopy<T>(                                              \
            const std::vector<float>& times,                               \
            const typename std::vector<VtArray<T>>& values);               \
    template typename VtKatana_GetKatanaAttrType<T>::type                  \
        VtKatanaMapOrCopy<T>(                                              \
            const typename std::map<float, VtArray<T>>&);                  \
    template const VtArray<T> VtKatanaMapOrCopy<T>(                        \
        const typename VtKatana_GetKatanaAttrType<T>::type&, float);       \
    template typename VtKatana_GetKatanaAttrType<T>::type                  \
        VtKatanaCopy<T>(                                                   \
            const VtArray<T>& value);                                      \
    template typename VtKatana_GetKatanaAttrType<T>::type                  \
        VtKatanaCopy<T>(                                                   \
            const std::vector<float>& times,                               \
            const typename std::vector<VtArray<T>>& values);               \
    template typename VtKatana_GetKatanaAttrType<T>::type                  \
        VtKatanaCopy<T>(                                                   \
            const typename std::map<float, VtArray<T>>&);                  \
    template const VtArray<T> VtKatanaCopy<T>(                             \
        const typename VtKatana_GetKatanaAttrType<T>::type&, float);       \

// Defines for copy
// Integral Types
VTKATANA_DEFINE_MAP_AND_COPY(bool)
VTKATANA_DEFINE_MAP_AND_COPY(char)
VTKATANA_DEFINE_MAP_AND_COPY(unsigned char)
VTKATANA_DEFINE_MAP_AND_COPY(short)
VTKATANA_DEFINE_MAP_AND_COPY(unsigned short)
VTKATANA_DEFINE_MAP_AND_COPY(int)
VTKATANA_DEFINE_MAP_AND_COPY(unsigned int)
VTKATANA_DEFINE_MAP_AND_COPY(uint64_t)
VTKATANA_DEFINE_MAP_AND_COPY(int64_t)

// Floating point types
VTKATANA_DEFINE_MAP_AND_COPY(float)
VTKATANA_DEFINE_MAP_AND_COPY(double)
VTKATANA_DEFINE_MAP_AND_COPY(GfHalf)

// Vec Types
VTKATANA_DEFINE_MAP_AND_COPY(GfVec2i)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec2f)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec2h)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec2d)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec3i)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec3f)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec3h)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec3d)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec4i)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec4f)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec4h)
VTKATANA_DEFINE_MAP_AND_COPY(GfVec4d)

// Matrix Types
VTKATANA_DEFINE_MAP_AND_COPY(GfMatrix3f)
VTKATANA_DEFINE_MAP_AND_COPY(GfMatrix3d)
VTKATANA_DEFINE_MAP_AND_COPY(GfMatrix4f)
VTKATANA_DEFINE_MAP_AND_COPY(GfMatrix4d)

// String types
VTKATANA_DEFINE_MAP_AND_COPY(std::string)
VTKATANA_DEFINE_MAP_AND_COPY(SdfAssetPath)
VTKATANA_DEFINE_MAP_AND_COPY(SdfPath)
VTKATANA_DEFINE_MAP_AND_COPY(TfToken)

#undef VTKATANA_DEFINE_MAP_AND_COPY

PXR_NAMESPACE_CLOSE_SCOPE
