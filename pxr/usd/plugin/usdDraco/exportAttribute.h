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

#ifndef PXR_USD_PLUGIN_USD_DRACO_EXPORT_ATTRIBUTE_H
#define PXR_USD_PLUGIN_USD_DRACO_EXPORT_ATTRIBUTE_H

#include "attributeDescriptor.h"

#include "pxr/base/gf/matrix2f.h"
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include <draco/attributes/geometry_attribute.h>
#include <draco/attributes/point_attribute.h>
#include <draco/mesh/mesh.h>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdDracoExportAttributeInterface
///
/// Base class for UsdDracoExportAttribute<T> classes. This base class allows
/// to generalize generic attributes of different types T and, e.g., store them
/// in one STL container.
///
class UsdDracoExportAttributeInterface {
public:
    virtual const UsdDracoAttributeDescriptor &GetDescriptor() const = 0;
    virtual void GetFromMesh(const UsdGeomMesh &usdMesh,
                             size_t numPositions) = 0;
    virtual void SetToMesh(draco::Mesh *dracoMesh) = 0;
    virtual void SetPointMapEntry(draco::PointIndex pointIndex,
                                  size_t positionIndex, size_t cornerIndex) = 0;
    virtual size_t GetNumValues() const = 0;
    virtual size_t GetNumIndices() const = 0;
    virtual bool UsesPositionIndex() const = 0;
};


/// \class UsdDracoExportAttribute
///
/// Helps to read and write mesh attributes while exporting USD meshes to Draco.
///
template <class T>
class UsdDracoExportAttribute : public UsdDracoExportAttributeInterface {
public:
    UsdDracoExportAttribute(const UsdDracoAttributeDescriptor &descriptor);
    const UsdDracoAttributeDescriptor &GetDescriptor() const override;

    // Populates member arrays with data from USD mesh based on descriptor.
    void GetFromMesh(const UsdGeomMesh &usdMesh, size_t numPositions) override;

    // Populates member values array with an ascending sequence (0, 1, 2, ...)
    // of a given size.
    void GetFromRange(size_t size);

    // Creates Draco mesh attribute, sets the values, and metadata.
    void SetToMesh(draco::Mesh *dracoMesh) override;

    // Sets Draco mesh attribute point map entry.
    void SetPointMapEntry(draco::PointIndex pointIndex, size_t entryIndex);

    // Sets Draco mesh attribute point map entry using either position index or
    // corner index, depending on the USD attribute interpolation value.
    void SetPointMapEntry(draco::PointIndex pointIndex,
                          size_t positionIndex, size_t cornerIndex) override;
    void Clear();
    size_t GetNumValues() const override;
    size_t GetNumIndices() const override;
    bool UsesPositionIndex() const override;
    bool HasPointAttribute() const;

private:
    template <class S>
    static void _MakeRange(VtArray<S> *array, size_t size);
    void _SetAttributeValue(draco::AttributeValueIndex avi, size_t index);

    // Specialization for arithmetic types.
    template <class T_ = T,
        typename std::enable_if<std::is_arithmetic<T_>::value>::type* = nullptr>
    void _SetAttributeValueSpecialized(
        draco::AttributeValueIndex avi, const T &value) {
        _pointAttribute->SetAttributeValue(avi, &value);
    }

    // Specialization for GfHalf type.
    template <class T_ = T,
        typename std::enable_if<
            std::is_same<T_, GfHalf>::value>::type* = nullptr>
    void _SetAttributeValueSpecialized(
        draco::AttributeValueIndex avi, const T &value) {
        // USD halfs are stored as Draco 16-bit ints.
        _pointAttribute->SetAttributeValue(avi, &value);
    }

    // Specialization for vector types.
    template <class T_ = T,
        typename std::enable_if<GfIsGfVec<T_>::value>::type* = nullptr>
    void _SetAttributeValueSpecialized(
        draco::AttributeValueIndex avi, const T &value) {
        _pointAttribute->SetAttributeValue(avi, value.data());
    }

    // Specialization for matrix types.
    template <class T_ = T,
        typename std::enable_if<GfIsGfMatrix<T_>::value>::type* = nullptr>
    void _SetAttributeValueSpecialized(
        draco::AttributeValueIndex avi, const T &value) {
        _pointAttribute->SetAttributeValue(avi, value.data());
    }

    // Specialization for quaternion types.
    template <class T_ = T,
        typename std::enable_if<GfIsGfQuat<T_>::value>::type* = nullptr>
    void _SetAttributeValueSpecialized(
            draco::AttributeValueIndex avi, const T &value) {
        // Combine quaternion components into a length-four vector.
        // TODO: Write directly to data buffer of the point attribute.
        std::array<typename T::ScalarType, 4> quaternion;
        quaternion[0] = value.GetReal();
        quaternion[1] = value.GetImaginary()[0];
        quaternion[2] = value.GetImaginary()[1];
        quaternion[3] = value.GetImaginary()[2];
        _pointAttribute->SetAttributeValue(avi, quaternion.data());
    }

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
    _usePositionIndex(false) {}

template <class T>
const UsdDracoAttributeDescriptor &
UsdDracoExportAttribute<T>::GetDescriptor() const {
    return _descriptor;
}

template <class T>
void UsdDracoExportAttribute<T>::GetFromMesh(
        const UsdGeomMesh &usdMesh, size_t numPositions) {
    if (_descriptor.GetStatus() != UsdDracoAttributeDescriptor::VALID)
        return;
    if (_descriptor.GetIsPrimvar()) {
        // Get data from a primvar.
        const UsdGeomPrimvarsAPI api = UsdGeomPrimvarsAPI(usdMesh.GetPrim());
        UsdGeomPrimvar primvar = api.GetPrimvar(_descriptor.GetName());
        if (!primvar)
            return;
        primvar.GetAttr().Get(&_values, _descriptor.GetValuesTime());
        primvar.GetIndices(&_indices, _descriptor.GetIndicesTime());

        // Primvars with constant interpolation are not exported and remain in
        // the USD mesh. Primvars with vertex interpolation are exported as
        // attributes associated with mesh vertices and may have implicit
        // indices.
        _usePositionIndex = primvar.GetInterpolation() == UsdGeomTokens->vertex;
        if (_indices.empty() && _usePositionIndex &&
            _values.size() == numPositions)
            _MakeRange(&_indices, numPositions);
    } else {
        // Get data from an attribute.
        UsdAttribute attribute =
            usdMesh.GetPrim().GetAttribute(_descriptor.GetName());
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
    const size_t byteStride = _descriptor.GetNumComponents() *
        draco::DataTypeLength(_descriptor.GetDataType());
    geometryAttr.Init(_descriptor.GetAttributeType(),
                      nullptr /* buffer */,
                      _descriptor.GetNumComponents(),
                      _descriptor.GetDataType(),
                      false /* normalized */,
                      byteStride,
                      0 /* byteOffset */);
    const int attributeId =
        dracoMesh->AddAttribute(geometryAttr, false, _values.size());
    _pointAttribute = dracoMesh->attribute(attributeId);

    // Populate Draco attribute values.
    for (size_t i = 0; i < _values.size(); i++)
        _SetAttributeValue(draco::AttributeValueIndex(i), i);

    // Set metadata for Draco attribute.
    dracoMesh->AddAttributeMetadata(attributeId, _descriptor.ToMetadata());
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
    _SetAttributeValueSpecialized(avi, _values[index]);
}

template <class T>
inline void UsdDracoExportAttribute<T>::SetPointMapEntry(
        draco::PointIndex pointIndex, size_t entryIndex) {
    if (_pointAttribute == nullptr)
        return;
    _pointAttribute->SetPointMapEntry(
        pointIndex, draco::AttributeValueIndex(entryIndex));
}

template <class T>
inline void UsdDracoExportAttribute<T>::SetPointMapEntry(
        draco::PointIndex pointIndex, size_t positionIndex,
        size_t cornerIndex) {
    if (_pointAttribute == nullptr)
        return;
    const size_t index = _usePositionIndex ? positionIndex : cornerIndex;
    const size_t entryIndex = _indices[index];
    SetPointMapEntry(pointIndex, entryIndex);
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

#endif  // PXR_USD_PLUGIN_USD_DRACO_EXPORT_ATTRIBUTE_H
