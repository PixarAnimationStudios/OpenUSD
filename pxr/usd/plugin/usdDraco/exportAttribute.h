//
// Copyright 2019 Google LLC
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

#ifndef USDDRACO_EXPORT_ATTRIBUTE_H
#define USDDRACO_EXPORT_ATTRIBUTE_H

#include "attributeDescriptor.h"

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include <draco/attributes/geometry_attribute.h>
#include <draco/attributes/point_attribute.h>
#include <draco/mesh/mesh.h>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdDracoExportAttribute
///
/// Helps to read and write mesh attributes while exporting USD files to Draco.
///
template <class T>
class UsdDracoExportAttribute {
public:
    UsdDracoExportAttribute(const UsdDracoAttributeDescriptor &descriptor);

    // Populates member arrays with data from USD mesh based on descriptor.
    void GetFromMesh(const UsdGeomMesh &usdMesh, size_t numPositions);

    // Populates member values array with an ascending sequence (0, 1, 2, ...)
    // of a given size.
    void GetFromRange(size_t size);

    // Creates Draco mesh attribute, sets the values, and metadata.
    void SetToMesh(draco::Mesh *dracoMesh);

    // Sets Draco mesh attribute point map entry.
    void SetPointMapEntry(draco::PointIndex pointIndex, size_t entryIndex);

    // Sets Draco mesh attribute point map entry using either position index or
    // corner index, depending on the USD attribute interpolation value.
    void SetPointMapEntry(draco::PointIndex pointIndex,
                          size_t positionIndex, size_t cornerIndex);
    void Clear();
    size_t GetNumValues() const;
    size_t GetNumIndices() const;
    bool UsesPositionIndex() const;
    bool HasPointAttribute() const;

private:
    template <class S>
    static void _MakeRange(VtArray<S> *array, size_t size);
    void _SetAttributeValue(draco::AttributeValueIndex avi, size_t index);

    // Specialization for values of integral types like int and bool.
    void _SetAttributeValueSpecialized(
        draco::AttributeValueIndex avi, const T &value, std::true_type);

    // Specialization for values of non-integral types like GfVec2f and GfVec3f.
    void _SetAttributeValueSpecialized(
        draco::AttributeValueIndex avi, const T &value, std::false_type);

private:
    UsdDracoAttributeDescriptor _descriptor;
    draco::PointAttribute *_pointAttribute;
    bool _usePositionIndex;
    VtArray<T> _values;
    VtArray<int> _indices;
};


template <class T>
UsdDracoExportAttribute<T>::UsdDracoExportAttribute(
    const UsdDracoAttributeDescriptor &descriptor) :
        _descriptor(descriptor),
        _pointAttribute(nullptr),
        _usePositionIndex(false) {
}

template <class T>
void UsdDracoExportAttribute<T>::GetFromMesh(
        const UsdGeomMesh &usdMesh, size_t numPositions) {
    if (_descriptor.isPrimvar) {
        // Get data from a primvar.
        const UsdGeomPrimvarsAPI api = UsdGeomPrimvarsAPI(usdMesh.GetPrim());
        UsdGeomPrimvar primvar = api.GetPrimvar(_descriptor.name);
        if (!primvar)
            return;
        primvar.GetAttr().Get(&_values);
        primvar.GetIndices(&_indices);

        // Primvars with constant interpolation are not translated to Draco and
        // remain in USD mesh.
        TfToken interpolation = primvar.GetInterpolation();
        if (interpolation == UsdGeomTokens->constant)
            return;

        // Primvars with vertex interpolation may have implicit indices.
        _usePositionIndex = interpolation == UsdGeomTokens->vertex;
        if (_indices.empty() && _usePositionIndex &&
            _values.size() == numPositions)
            _MakeRange(&_indices, numPositions);
    } else {
        // Get data from an attribute.
        UsdAttribute attribute =
            usdMesh.GetPrim().GetAttribute(_descriptor.name);
        if (attribute)
            attribute.Get(&_values);
    }
}

template <class T>
void UsdDracoExportAttribute<T>::SetToMesh(draco::Mesh *dracoMesh) {
    // Optional attributes like normals may not be present.
    if (_values.empty())
        return;

    // Create Draco attribtue.
    draco::GeometryAttribute geometryAttr;
    const size_t byteStride = _descriptor.numComponents *
        draco::DataTypeLength(_descriptor.dataType);
    geometryAttr.Init(_descriptor.attributeType,
                      nullptr /* buffer */,
                      _descriptor.numComponents,
                      _descriptor.dataType,
                      false /* normalized */,
                      byteStride,
                      0 /* byteOffset */);
    const int attrId =
        dracoMesh->AddAttribute(geometryAttr, false, _values.size());
    _pointAttribute = dracoMesh->attribute(attrId);

    // Populate Draco attribute values.
    for (size_t i = 0; i < _values.size(); i++)
        _SetAttributeValue(draco::AttributeValueIndex(i), i);

    // Name Draco attribute via metadata.
    if (!_descriptor.metadataName.empty()) {
        std::unique_ptr<draco::AttributeMetadata> metadata =
            std::unique_ptr<draco::AttributeMetadata>(
                new draco::AttributeMetadata());
        metadata->AddEntryString(
            UsdDracoAttributeDescriptor::METADATA_NAME_KEY.c_str(),
            _descriptor.metadataName.c_str());
        dracoMesh->AddAttributeMetadata(attrId, std::move(metadata));
    }
}

template <class T>
void UsdDracoExportAttribute<T>::GetFromRange(size_t size) {
    _MakeRange(&_values, size);
}

template <class T>
template <class S>
void UsdDracoExportAttribute<T>::_MakeRange(
        VtArray<S> *array, size_t size) {
    (*array).resize(size);
    for (size_t i = 0; i < size; i++) {
        (*array)[i] = static_cast<S>(i);
    }
}

template <class T>
inline void UsdDracoExportAttribute<T>::_SetAttributeValue(
        draco::AttributeValueIndex avi, size_t index) {
    _SetAttributeValueSpecialized(avi, _values[index], std::is_integral<T>());
}

template<class T>
void UsdDracoExportAttribute<T>::_SetAttributeValueSpecialized(
        draco::AttributeValueIndex avi, const T &value, std::true_type) {
    _pointAttribute->SetAttributeValue(avi, &value);
}

template<class T>
void UsdDracoExportAttribute<T>::_SetAttributeValueSpecialized(
        draco::AttributeValueIndex avi, const T &value, std::false_type) {
    _pointAttribute->SetAttributeValue(avi, value.data());
}

template <class T>
inline void UsdDracoExportAttribute<T>::SetPointMapEntry(
        draco::PointIndex pointIndex, size_t entryIndex) {
    if (_pointAttribute != nullptr) {
        _pointAttribute->SetPointMapEntry(
            pointIndex, draco::AttributeValueIndex(entryIndex));
    }
}

template <class T>
inline void UsdDracoExportAttribute<T>::SetPointMapEntry(
        draco::PointIndex pointIndex, size_t positionIndex,
        size_t cornerIndex) {
    if (_pointAttribute != nullptr) {
        const size_t index = _usePositionIndex ? positionIndex : cornerIndex;
        const size_t entryIndex = _indices[index];
        SetPointMapEntry(pointIndex, entryIndex);
    }
}

template <class T>
void UsdDracoExportAttribute<T>::Clear() {
    _values.clear();
    _indices.clear();
    _usePositionIndex = false;
    _pointAttribute = nullptr;
}

template <class T>
size_t UsdDracoExportAttribute<T>::GetNumValues() const {
    return _values.size();
}

template <class T>
size_t UsdDracoExportAttribute<T>::GetNumIndices() const {
    return _indices.size();
}

template <class T>
bool UsdDracoExportAttribute<T>::UsesPositionIndex() const {
    return _usePositionIndex;
}

template <class T>
inline bool UsdDracoExportAttribute<T>::HasPointAttribute() const {
    return _pointAttribute != nullptr;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // USDDRACO_EXPORT_ATTRIBUTE_H
