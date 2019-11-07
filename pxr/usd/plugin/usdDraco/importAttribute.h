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

#ifndef PXR_USD_PLUGIN_USD_DRACO_IMPORT_ATTRIBUTE_H
#define PXR_USD_PLUGIN_USD_DRACO_IMPORT_ATTRIBUTE_H

#include "attributeDescriptor.h"
#include "attributeFactory.h"

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include <draco/attributes/point_attribute.h>
#include <draco/mesh/mesh.h>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdDracoImportAttributeInterface
///
/// Base class for UsdDracoImportAttribute<T> classes. This base class allows
/// to generalize generic attributes of different types T and, e.g., store them
/// in one STL container.
///
class UsdDracoImportAttributeInterface {
public:
    virtual const UsdDracoAttributeDescriptor &GetDescriptor() const = 0;
    virtual void SetToMesh(UsdGeomMesh *usdMesh) const = 0;
    virtual void PopulateValues() = 0;
    virtual int GetMappedIndex(draco::PointIndex pi) const = 0;
    virtual void ResizeIndices(size_t size) = 0;
    virtual void SetIndex(size_t at, int index) = 0;
    virtual size_t GetNumValues() const = 0;
    virtual size_t GetNumIndices() const = 0;
    virtual bool HasPointAttribute() const = 0;
};


/// \class UsdDracoImportAttribute
///
/// Helps to read and write mesh attributes while importing Draco meshes to USD.
///
template <class T>
class UsdDracoImportAttribute : public UsdDracoImportAttributeInterface {
public:
    UsdDracoImportAttribute(UsdDracoAttributeDescriptor descriptor,
                            const draco::Mesh &dracoMesh);
    const UsdDracoAttributeDescriptor &GetDescriptor() const override;

    // Adds an attribute or primvar to USD mesh according to attribuite
    // descriptor.
    void SetToMesh(UsdGeomMesh *usdMesh) const override;

    // Populates member values array with data from Draco attribute.
    void PopulateValues() override;
    void PopulateValuesWithOrder(
        const UsdDracoImportAttribute<int> &order, size_t numFaces,
        const draco::Mesh &dracoMesh);

    // Returns mapped value from Draco attribute.
    T GetMappedValue(draco::PointIndex pi) const;

    // Returns mapped index from Draco attribute.
    int GetMappedIndex(draco::PointIndex pi) const override;

    const VtArray<T> &GetValues() const;
    void ResizeIndices(size_t size) override;
    void SetIndex(size_t at, int index) override;
    size_t GetNumValues() const override;
    size_t GetNumIndices() const override;
    bool HasPointAttribute() const override;

private:
    const draco::PointAttribute *_GetFromMesh(
        const draco::Mesh &dracoMesh);

    // Specialization for arithmetic types.
    template <class T_ = T,
        typename std::enable_if<std::is_arithmetic<T_>::value>::type* = nullptr>
    void _GetAttributeValueSpecialized(draco::AttributeValueIndex avi) {
        _pointAttribute->GetValue(avi, &_values[avi.value()]);
    }

    // Specialization for GfHalf type.
    template <class T_ = T,
        typename std::enable_if<
            std::is_same<T_, GfHalf>::value>::type* = nullptr>
    void _GetAttributeValueSpecialized(draco::AttributeValueIndex avi) {
        // USD halfs are stored as Draco 16-bit ints.
        _pointAttribute->GetValue(avi, &_values[avi.value()]);
    }

    // Specialization for vector types.
    template <class T_ = T,
        typename std::enable_if<GfIsGfVec<T_>::value>::type* = nullptr>
    void _GetAttributeValueSpecialized(draco::AttributeValueIndex avi) {
        _pointAttribute->GetValue(avi, _values[avi.value()].data());
    }

    // Specialization for matrix types.
    template <class T_ = T,
        typename std::enable_if<GfIsGfMatrix<T_>::value>::type* = nullptr>
    void _GetAttributeValueSpecialized(draco::AttributeValueIndex avi) {
         _pointAttribute->GetValue(avi, _values[avi.value()].data());
    }

    // Specialization for quaternion types.
    template <class T_ = T,
        typename std::enable_if<GfIsGfQuat<T_>::value>::type* = nullptr>
    void _GetAttributeValueSpecialized(draco::AttributeValueIndex avi) {
        // Split a length-four vector into quaternion components.
        // TODO: Read directly from data buffer of the point attribute.
        std::array<typename T::ScalarType, 4> quaternion;
        _pointAttribute->GetValue(avi, quaternion.data());
        T &value = _values[avi.value()];
        value.SetReal(quaternion[0]);
        value.SetImaginary(quaternion[1], quaternion[2], quaternion[3]);
    }

private:
    const UsdDracoAttributeDescriptor _descriptor;
    const draco::PointAttribute *_pointAttribute;
    VtArray<T> _values;
    VtArray<int> _indices;
};


template <class T>
UsdDracoImportAttribute<T>::UsdDracoImportAttribute(
        UsdDracoAttributeDescriptor descriptor, const draco::Mesh &dracoMesh) :
            _descriptor(descriptor), _pointAttribute(_GetFromMesh(dracoMesh)) {
}

template <class T>
const UsdDracoAttributeDescriptor &
UsdDracoImportAttribute<T>::GetDescriptor() const {
    return _descriptor;
}

template <class T>
const draco::PointAttribute *UsdDracoImportAttribute<T>::_GetFromMesh(
        const draco::Mesh &dracoMesh) {
    const bool hasMetadata =
        _descriptor.GetAttributeType() == draco::GeometryAttribute::GENERIC;
    const int attributeId = hasMetadata
        ? dracoMesh.GetAttributeIdByMetadataEntry(
            UsdDracoAttributeDescriptor::METADATA_NAME_KEY,
            _descriptor.GetName().GetText())
        : dracoMesh.GetNamedAttributeId(_descriptor.GetAttributeType());
    return (attributeId == -1) ? nullptr : dracoMesh.attribute(attributeId);
}

template <class T>
void UsdDracoImportAttribute<T>::SetToMesh(UsdGeomMesh *usdMesh) const {
    if (_pointAttribute == nullptr)
        return;
    if (_descriptor.GetIsPrimvar()) {
        // Set data as a primvar.
        const UsdGeomPrimvarsAPI api = UsdGeomPrimvarsAPI(usdMesh->GetPrim());
        UsdGeomPrimvar primvar = api.CreatePrimvar(
            _descriptor.GetName(),
            UsdDracoAttributeFactory::GetSdfValueTypeName(_descriptor));
        primvar.Set(_values, _descriptor.GetValuesTime());
        primvar.SetIndices(_indices, _descriptor.GetIndicesTime());
        if (_descriptor.GetInterpolation() == UsdGeomTokens->vertex) {
            // TODO: While exporting to Draco, indices of primvars with vertex
            // interpolation are converted to point to face corners. Such
            // indices should be restored to point to vertices in order to
            // reduce memory. Meeanwhile, the interpolation is changed to
            // faceVarying.
            primvar.SetInterpolation(UsdGeomTokens->faceVarying);
        } else {
            primvar.SetInterpolation(_descriptor.GetInterpolation());
        }
    } else {
        // Set data as an attribute.
        UsdAttribute attribute = usdMesh->GetPrim().CreateAttribute(
            _descriptor.GetName(),
            UsdDracoAttributeFactory::GetSdfValueTypeName(_descriptor));
        attribute.Set(_values, _descriptor.GetValuesTime());
    }
}

template <class T>
void UsdDracoImportAttribute<T>::PopulateValues() {
    if (_pointAttribute == nullptr)
        return;
    const size_t numValues = _pointAttribute->size();
    _values.resize(numValues);
    for (size_t i = 0; i < numValues; i++) {
        const draco::AttributeValueIndex avi(i);
        _GetAttributeValueSpecialized(avi);
    }
}

template <class T>
void UsdDracoImportAttribute<T>::PopulateValuesWithOrder(
        const UsdDracoImportAttribute<int> &order, size_t numFaces,
        const draco::Mesh &dracoMesh) {
    if (_pointAttribute == nullptr)
        return;
    const size_t numValues = _pointAttribute->size();
    _values.resize(numValues);
    std::vector<bool> populated(numValues, false);
    for (size_t i = 0; i < numFaces; i++) {
        const draco::Mesh::Face &face = dracoMesh.face(draco::FaceIndex(i));
        for (size_t c = 0; c < 3; c++) {
            const draco::PointIndex pi = face[c];
            const int origIndex = order.GetMappedValue(pi);
            if (!populated[origIndex]) {
                _pointAttribute->GetMappedValue(pi, _values[origIndex].data());
                populated[origIndex] = true;
            }
        }
    }
}

template <class T>
inline T UsdDracoImportAttribute<T>::GetMappedValue(
        draco::PointIndex pi) const {
    if (_pointAttribute == nullptr)
        return T(0);
    T value;
    _pointAttribute->GetMappedValue(pi, &value);
    return value;
}

template <class T>
inline int UsdDracoImportAttribute<T>::GetMappedIndex(
        draco::PointIndex pi) const {
    if (_pointAttribute == nullptr)
        return -1;
    return static_cast<int>(_pointAttribute->mapped_index(pi).value());
}

template <class T>
const VtArray<T> &UsdDracoImportAttribute<T>::GetValues() const {
    return _values;
}

template <class T>
void UsdDracoImportAttribute<T>::ResizeIndices(size_t size) {
    if (_pointAttribute == nullptr)
        return;
    _indices.resize(size);
}

template <class T>
inline void UsdDracoImportAttribute<T>::SetIndex(size_t at, int index) {
    if (_pointAttribute == nullptr)
        return;
    _indices[at] = index;
}

template <class T>
size_t UsdDracoImportAttribute<T>::GetNumValues() const {
    return _values.size();
}

template <class T>
size_t UsdDracoImportAttribute<T>::GetNumIndices() const {
    return _indices.size();
}

template <class T>
inline bool UsdDracoImportAttribute<T>::HasPointAttribute() const {
    return _pointAttribute != nullptr;
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_PLUGIN_USD_DRACO_IMPORT_ATTRIBUTE_H
