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

#include "attributeDescriptor.h"
#include "attributeFactory.h"

#include <typeinfo>

#include "pxr/pxr.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


const std::string UsdDracoAttributeDescriptor::METADATA_NAME_KEY = "name";
const std::string UsdDracoAttributeDescriptor::METADATA_SHAPE_KEY = "shape";
const std::string UsdDracoAttributeDescriptor::METADATA_HALF_KEY = "half";
const std::string UsdDracoAttributeDescriptor::METADATA_VALUES_TIME_KEY =
    "values_time";
const std::string UsdDracoAttributeDescriptor::METADATA_INDICES_TIME_KEY =
    "indices_time";
const std::string UsdDracoAttributeDescriptor::METADATA_INTERPOLATION_KEY =
    "interpolation";

UsdDracoAttributeDescriptor::UsdDracoAttributeDescriptor() :
    UsdDracoAttributeDescriptor(INVALID) {}

UsdDracoAttributeDescriptor::UsdDracoAttributeDescriptor(Status status) :
    UsdDracoAttributeDescriptor(
        status, draco::GeometryAttribute::INVALID, TfToken(), draco::DT_INVALID,
        false, 0, GetDefaultShape(), GetDefaultHalf(), GetDefaultTime(),
        GetDefaultTime(), TfToken()) {}

UsdDracoAttributeDescriptor::UsdDracoAttributeDescriptor(
        Status status,
        draco::GeometryAttribute::Type attributeType,
        TfToken name,
        draco::DataType dataType,
        bool isPrimvar,
        size_t numComponents,
        Shape shape,
        bool isHalf,
        UsdTimeCode valuesTime,
        UsdTimeCode indicesTime,
        TfToken interpolation) :
            _status(status),
            _attributeType(attributeType),
            _name(name),
            _dataType(dataType),
            _isPrimvar(isPrimvar),
            _numComponents(numComponents),
            _shape(shape),
            _isHalf(isHalf),
            _valuesTime(valuesTime),
            _indicesTime(indicesTime),
            _interpolation(interpolation) {}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::Invalid() {
    // Create descriptor indicating that mesh contains an invalid attribute.
    return UsdDracoAttributeDescriptor(INVALID);
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::Absent() {
    // Create descriptor indicating that attribute is absent from the mesh.
    return UsdDracoAttributeDescriptor(ABSENT);
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::Create(
    draco::GeometryAttribute::Type attributeType, TfToken name,
    draco::DataType dataType, bool isPrimvar, size_t numComponents,
    Shape shape, bool isHalf, UsdTimeCode valuesTime, UsdTimeCode indicesTime,
    TfToken interpolation) {
    // Create a valid descriptor.
    return UsdDracoAttributeDescriptor(
        VALID, attributeType, name, dataType, isPrimvar, numComponents, shape,
        isHalf, valuesTime, indicesTime, interpolation);
}

bool UsdDracoAttributeDescriptor::IsGeneric() const {
    return _name != GetPositionsName() &&
           _name != GetTexCoordsName() &&
           _name != GetNormalsName() &&
           _name != GetHoleFacesName() &&
           _name != GetAddedEdgesName() &&
           _name != GetPointOrderName();
}

const std::set<TfToken> &
UsdDracoAttributeDescriptor::GetSupportedInterpolations() {
    static const std::set<TfToken> supportedInterpolations = {
        UsdGeomTokens->vertex, UsdGeomTokens->faceVarying};
    return supportedInterpolations;
}

std::string UsdDracoAttributeDescriptor::GetShapeText() const {
    return GetShapeText(_shape);
}

std::string UsdDracoAttributeDescriptor::GetShapeText(Shape shape) {
    switch (shape) {
        case VECTOR:
            return "vec";
        case QUATERNION:
            return "quat";
        case MATRIX:
            return "mat";
    }
    TF_RUNTIME_ERROR("Unsupported UsdDracoAttributeDescriptor::Shape type");
    return std::string(); 
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::ForPositions(
    const UsdGeomMesh &mesh) {
    // Get position attribute from USD mesh.
    const TfToken name = GetPositionsName();
    const UsdAttribute attribute = mesh.GetPrim().GetAttribute(name);
    if (!attribute)
        return Absent();

    // Create descriptor from position attribute. Unlike primvars, USD
    // attributes have no indices and no interpolation.
    const bool isPrimvar = false;
    const UsdTimeCode indicesTime = GetDefaultTime();
    const TfToken interpolation;
    return FromUsdAttribute(
        attribute, draco::GeometryAttribute::POSITION, name, isPrimvar,
        indicesTime, interpolation);
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::ForTexCoords(
    const UsdGeomMesh &mesh) {
    return FromUsdMesh(
        mesh, draco::GeometryAttribute::TEX_COORD, GetTexCoordsName());
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::ForNormals(
    const UsdGeomMesh &mesh) {
    return FromUsdMesh(
        mesh, draco::GeometryAttribute::NORMAL, GetNormalsName());
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::ForPositions(
    const draco::Mesh &mesh) {
    const bool isPrimvar = false;
    return FromDracoMesh(
        mesh, draco::GeometryAttribute::POSITION, GetPositionsName(),
        isPrimvar);
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::ForTexCoords(
    const draco::Mesh &mesh) {
    const bool isPrimvar = true;
    return FromDracoMesh(
        mesh, draco::GeometryAttribute::TEX_COORD, GetTexCoordsName(),
        isPrimvar);
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::ForNormals(
    const draco::Mesh &mesh) {
    const bool isPrimvar = true;
    return FromDracoMesh(
        mesh, draco::GeometryAttribute::NORMAL, GetNormalsName(), isPrimvar);
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::ForHoleFaces() {
    return Create(
        draco::GeometryAttribute::GENERIC, GetHoleFacesName(), draco::DT_UINT8,
        false, 1, VECTOR, false, GetDefaultTime(), GetDefaultTime(), TfToken());
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::ForAddedEdges() {
    return Create(
        draco::GeometryAttribute::GENERIC, GetAddedEdgesName(), draco::DT_UINT8,
        false, 1, VECTOR, false, GetDefaultTime(), GetDefaultTime(), TfToken());
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::ForPosOrder() {
    return Create(
        draco::GeometryAttribute::GENERIC, GetPointOrderName(),
        draco::DT_UINT32, false, 1, VECTOR, false, GetDefaultTime(),
        GetDefaultTime(), TfToken());
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::FromDracoMesh(
    const draco::Mesh &mesh, draco::GeometryAttribute::Type attributeType,
    TfToken name, bool isPrimvar) {
    // All attributes in meshes exported from USD have metadata with name.
    const int attributeId =
        mesh.GetAttributeIdByMetadataEntry(METADATA_NAME_KEY, name);
    if (attributeId == -1)
        return Absent();
    const draco::PointAttribute &attribute = *mesh.attribute(attributeId);

    // Get attribute metadata from Draco mesh.
    const draco::AttributeMetadata *metadata =
        mesh.GetAttributeMetadataByAttributeId(attributeId);

    // Create descriptor from Draco attribute and metadata.
    return FromDracoAttribute(attribute, *metadata, isPrimvar);
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::FromDracoAttribute(
    const draco::PointAttribute &attribute,
    const draco::AttributeMetadata &metadata, bool isPrimvar) {
    // Metadata must have a name.
    std::string name;
    if (!metadata.GetEntryString(METADATA_NAME_KEY, &name))
        return Invalid();

    // Metadata may have a shape.
    UsdDracoAttributeDescriptor::Shape shape = GetDefaultShape();
    std::string shapeText;
    if (metadata.GetEntryString(METADATA_SHAPE_KEY, &shapeText)) {
        if (shapeText == GetShapeText(VECTOR))
            shape = VECTOR;
        else if (shapeText == GetShapeText(MATRIX))
            shape = MATRIX;
        else if (shapeText == GetShapeText(QUATERNION))
            shape = QUATERNION;
        else
            return Invalid();
    }

    // Metadata may have a half.
    int isHalfInt = static_cast<int>(GetDefaultHalf());
    metadata.GetEntryInt(METADATA_HALF_KEY, &isHalfInt);
    const bool isHalf = static_cast<bool>(isHalfInt);

    // Metadata may have a time for values.
    UsdTimeCode valuesTime = GetDefaultTime();
    double valuesTimeDouble;
    if (metadata.GetEntryDouble(METADATA_VALUES_TIME_KEY, &valuesTimeDouble))
        valuesTime = UsdTimeCode(valuesTimeDouble);

    // Metadata may have a time for indices.
    UsdTimeCode indicesTime = GetDefaultTime();
    double indicesTimeDouble;
    if (metadata.GetEntryDouble(METADATA_INDICES_TIME_KEY, &indicesTimeDouble))
        indicesTime = UsdTimeCode(indicesTimeDouble);

    // Metadata may have interpolation for primvars.
    TfToken interpolation;
    if (isPrimvar) {
        interpolation = GetDefaultInterpolation();
        std::string interpolationText;
        if (metadata.GetEntryString(
                METADATA_INTERPOLATION_KEY, &interpolationText)) {
            for (const TfToken &supported : GetSupportedInterpolations()) {
                if (interpolationText == supported.GetString()) {
                    interpolation = supported;
                    break;
                }
            }
        }
    }

    // Create a descriptor.
    return UsdDracoAttributeDescriptor::Create(
        attribute.attribute_type(), TfToken(name.c_str()),
        attribute.data_type(), isPrimvar, attribute.num_components(), shape,
        isHalf, valuesTime, indicesTime, interpolation);
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::FromUsdMesh(
    const UsdGeomMesh &mesh, draco::GeometryAttribute::Type attributeType,
    TfToken name) {
    // Get primvar by name from USD mesh.
    const UsdGeomPrimvarsAPI api = UsdGeomPrimvarsAPI(mesh.GetPrim());
    const UsdGeomPrimvar primvar = api.GetPrimvar(name);
    if (!primvar)
        return Absent();

    // Create descriptor from primvar.
    return FromUsdPrimvar(primvar, attributeType);
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::FromUsdPrimvar(
    const UsdGeomPrimvar &primvar,
    draco::GeometryAttribute::Type attributeType) {
    // Skip primvars with unsupported interpolation.
    const std::set<TfToken> &supported = GetSupportedInterpolations();
    if (supported.find(primvar.GetInterpolation()) == supported.end())
        return Invalid();

    // Primvar indices at a single time sample can be exported.
    UsdTimeCode indicesTime = GetDefaultTime();
    double indicesTimeDouble;
    if (!GetTimeFrom(primvar, &indicesTimeDouble))
        return Invalid();
    indicesTime = UsdTimeCode(indicesTimeDouble);

    // Create descriptor from the underlying attribute and with a given name.
    const bool isPrimvar = true;
    return FromUsdAttribute(
        primvar.GetAttr(), attributeType, primvar.GetName(), isPrimvar,
        indicesTime, primvar.GetInterpolation());
}

UsdDracoAttributeDescriptor UsdDracoAttributeDescriptor::FromUsdAttribute(
    const UsdAttribute &attribute, draco::GeometryAttribute::Type attributeType,
    const TfToken &name, bool isPrimvar, const UsdTimeCode &indicesTime,
    const TfToken &interpolation) {
    // Skip attributes with unsupported dimensions.
    const size_t numDimensions = attribute.GetTypeName().GetDimensions().size;
    if (numDimensions > 2)
        return Invalid();

    // Skip attributes with unsupported number of components.
    size_t dimensionOneSize = 1;
    size_t dimensionTwoSize = 1;
    if (numDimensions > 0) {
        dimensionOneSize = attribute.GetTypeName().GetDimensions().d[0];
        if (numDimensions > 1)
            dimensionTwoSize = attribute.GetTypeName().GetDimensions().d[1];
    }
    if (dimensionOneSize < 1 || dimensionOneSize > 4 ||
        dimensionTwoSize < 1 || dimensionTwoSize > 4 )
        return Invalid();
    const size_t numComponents = dimensionOneSize * dimensionTwoSize;

    // Skip attributes with non-array data types.
    if (!attribute.GetTypeName().IsArray())
        return Invalid();

    // Get Draco data type from attribute type.
    const std::type_info &typeInfo =
        attribute.GetTypeName().GetScalarType().GetType().GetTypeid();
    const draco::DataType dataType =
        UsdDracoAttributeFactory::GetDracoDataType(typeInfo);

    // Skip attributes with unsupported data types.
    if (dataType == draco::DT_INVALID)
        return Invalid();

    // Get data shape, whcih can be vector, quaternion, or matrix.
    const UsdDracoAttributeDescriptor::Shape shape =
        UsdDracoAttributeFactory::GetShape(typeInfo);

    // Get flag indicating that the type is a 16-bit floating point.
    const bool isHalf = UsdDracoAttributeFactory::IsHalf(typeInfo);

    // Values at a single time sample can be exported.
    UsdTimeCode valuesTime = GetDefaultTime();
    double valuesTimeDouble;
    if (!GetTimeFrom(attribute, &valuesTimeDouble))
        return Invalid();
    valuesTime = UsdTimeCode(valuesTimeDouble);

    // Create a descriptor.
    return Create(
        attributeType, name, dataType, isPrimvar, numComponents, shape, isHalf,
        valuesTime, indicesTime, interpolation);
}

std::unique_ptr<draco::AttributeMetadata>
UsdDracoAttributeDescriptor::ToMetadata() const {
    // Create metadata.
    std::unique_ptr<draco::AttributeMetadata> metadata =
        std::unique_ptr<draco::AttributeMetadata>(
            new draco::AttributeMetadata());

    // Although non-generic attributes can be found by GeometryAttribute::Type,
    // we always add name to metadata to reduce the importer code complexity.
    metadata->AddEntryString(
        UsdDracoAttributeDescriptor::METADATA_NAME_KEY.c_str(),
        _name.GetText());

    // Default values are not added to metadata.
    if (_shape != GetDefaultShape())
        metadata->AddEntryString(
            METADATA_SHAPE_KEY.c_str(), GetShapeText());
    if (_isHalf != GetDefaultHalf())
        metadata->AddEntryInt(
            METADATA_HALF_KEY.c_str(), static_cast<int>(_isHalf));
    if (_valuesTime != GetDefaultTime())
        metadata->AddEntryDouble(
            METADATA_VALUES_TIME_KEY.c_str(), _valuesTime.GetValue());
    if (_indicesTime != GetDefaultTime())
        metadata->AddEntryDouble(
            METADATA_INDICES_TIME_KEY.c_str(), _indicesTime.GetValue());
    if (_interpolation != GetDefaultInterpolation())
        if (!_interpolation.IsEmpty())
            metadata->AddEntryString(
                METADATA_INTERPOLATION_KEY.c_str(), _interpolation.GetString());
    return metadata;
}


PXR_NAMESPACE_CLOSE_SCOPE
