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

#ifndef PXR_USD_PLUGIN_USD_DRACO_ATTRIBUTE_DESCRIPTOR_H
#define PXR_USD_PLUGIN_USD_DRACO_ATTRIBUTE_DESCRIPTOR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include <draco/attributes/geometry_attribute.h>
#include <draco/attributes/point_attribute.h>
#include <draco/mesh/mesh.h>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdDracoAttributeDescriptor
///
/// Stores description of mesh attributes, such as name, data type, shape, time
/// sample, interpolation, etc. Provides methods for reading/writing such
/// attribute descriptions from/to USD mesh attributes, primvars, and Draco
/// metadata.
///
class UsdDracoAttributeDescriptor {
public:
    // The status indicates whether the descriptor is valid or invalid, as well
    // as whether the corresponding attribute is absent from the mesh.
    enum Status { VALID, INVALID, ABSENT };

    // Describes attribute data shape, such as vector, matrix, or quaternion.
    // Scalar data types are assumed to be a special case of a vector.
    enum Shape { VECTOR, MATRIX, QUATERNION};

    // Keys for storing of attribute description items in Draco metadata, for
    // description items that are not directly supported by Draco file format.
    static const std::string METADATA_NAME_KEY;
    static const std::string METADATA_SHAPE_KEY;
    static const std::string METADATA_HALF_KEY;
    static const std::string METADATA_VALUES_TIME_KEY;
    static const std::string METADATA_INDICES_TIME_KEY;
    static const std::string METADATA_INTERPOLATION_KEY;

    // Methods for creating descriptors for specific named attributes from USD
    // or Draco mesh. While some of the description items for named attributes
    // are known, e.g., a texture coordinate is always a length-two vector,
    // other description items need to be obtained from mesh, susch as
    // interpolation, time sample, shape, etc.
    static UsdDracoAttributeDescriptor ForPositions(const UsdGeomMesh &mesh);
    static UsdDracoAttributeDescriptor ForTexCoords(const UsdGeomMesh &mesh);
    static UsdDracoAttributeDescriptor ForNormals(const UsdGeomMesh &mesh);
    static UsdDracoAttributeDescriptor ForPositions(const draco::Mesh &mesh);
    static UsdDracoAttributeDescriptor ForTexCoords(const draco::Mesh &mesh);
    static UsdDracoAttributeDescriptor ForNormals(const draco::Mesh &mesh);

    // Descriptors for helper attributes that enable support of USD mesh
    // features that are not supported directly by Draco, such as quads, holes,
    // and subdivision surfaces.
    static UsdDracoAttributeDescriptor ForHoleFaces();
    static UsdDracoAttributeDescriptor ForAddedEdges();
    static UsdDracoAttributeDescriptor ForPosOrder();

    // Creates attribute descriptor from Draco attribute and its metadata.
    static UsdDracoAttributeDescriptor FromDracoAttribute(
        const draco::PointAttribute &attribute,
        const draco::AttributeMetadata &metadata, bool isPrimvar);

    // Creates attribute descriptor from a given USD mesh primvar.
    static UsdDracoAttributeDescriptor FromUsdPrimvar(
        const UsdGeomPrimvar &primvar,
        draco::GeometryAttribute::Type attributeType);

    // Craetes Draco metadata representation of attribute descriptor.
    std::unique_ptr<draco::AttributeMetadata> ToMetadata() const;

    // Getters for individual attribute description items.
    Status GetStatus() const { return _status; }
    draco::GeometryAttribute::Type GetAttributeType() const { return _attributeType; }
    const TfToken &GetName() const { return _name; }
    draco::DataType GetDataType() const { return _dataType; }
    bool GetIsPrimvar() const { return _isPrimvar; }
    size_t GetNumComponents() const { return _numComponents; }
    Shape GetShape() const { return _shape; }
    bool GetIsHalf() const { return _isHalf; }
    UsdTimeCode GetValuesTime() const { return _valuesTime; }
    UsdTimeCode GetIndicesTime() const { return _indicesTime; }
    const TfToken &GetInterpolation() const { return _interpolation; }

    // Default values for Draco metadata entries.
    static Shape GetDefaultShape() { return VECTOR; }
    static bool GetDefaultHalf() { return false; }
    static UsdTimeCode GetDefaultTime() { return UsdTimeCode::Default(); }
    static TfToken GetDefaultInterpolation() {
        return UsdGeomTokens->faceVarying;
    }

    // Names of non-generic attributes.
    static TfToken GetPositionsName() { return UsdGeomTokens->points; }
    static TfToken GetTexCoordsName() { return TfToken("primvars:Texture_uv"); }
    static TfToken GetNormalsName() { return TfToken("primvars:normals"); }
    static TfToken GetHoleFacesName() { return TfToken("hole_faces"); }
    static TfToken GetAddedEdgesName() { return TfToken("added_edges"); }
    static TfToken GetPointOrderName() { return TfToken("point_order"); }

    // Indicates whether the attribute is generic.
    bool IsGeneric() const;

private:
    // Creates an attribute description with an invalid status.
    UsdDracoAttributeDescriptor();

    // Creates an attribute descriptor with a given status.
    UsdDracoAttributeDescriptor(Status status);

    // Creates an sttribute descriptor with all the given items.
    UsdDracoAttributeDescriptor(Status status,
                                draco::GeometryAttribute::Type attributeType,
                                TfToken name,
                                draco::DataType dataType,
                                bool isPrimvar,
                                size_t numComponents,
                                Shape shape,
                                bool isHalf,
                                UsdTimeCode valuesTime,
                                UsdTimeCode indicesTime,
                                TfToken interpolation);

    // Static helper methods for creating attribute desccriptor.
    static UsdDracoAttributeDescriptor Invalid();
    static UsdDracoAttributeDescriptor Absent();
    static UsdDracoAttributeDescriptor Create(
        draco::GeometryAttribute::Type attributeType, TfToken name,
        draco::DataType dataType, bool isPrimvar, size_t numComponents,
        Shape shape, bool isHalf, UsdTimeCode valuesTime,
        UsdTimeCode indicesTime, TfToken interpolation);

    // Methods for creating attribute descriptor from Draco and USD meshes.
    static UsdDracoAttributeDescriptor FromDracoMesh(
        const draco::Mesh &mesh, draco::GeometryAttribute::Type attributeType,
        TfToken name, bool isPrimvar);
    static UsdDracoAttributeDescriptor FromUsdMesh(
        const UsdGeomMesh &mesh, draco::GeometryAttribute::Type attributeType,
        TfToken name);
    static UsdDracoAttributeDescriptor FromUsdAttribute(
        const UsdAttribute &attribute,
        draco::GeometryAttribute::Type attributeType, const TfToken &name,
        bool isPrimvar, const UsdTimeCode &indicesTime,
        const TfToken &interpolation);

    // Helper methods.
    static const std::set<TfToken> &GetSupportedInterpolations();
    std::string GetShapeText() const;
    static std::string GetShapeText(Shape shape);

    // Template method for extracting a single time sample for attribute values
    // or primvar indices. Supported types are UsdGeomPrimvar and UsdAttribute.
    // Returns true on success.
    template <class T>
    static bool GetTimeFrom(const T &item, double *time) {
        // Try to get time samples and return in case of error.
        std::vector<double> times;
        if (!item.GetTimeSamples(&times))
            return false;

        // Multiple time samples are not supported.
        if (times.size() > 1) {
            return false;
        }

        // Use default time if there are no time samples.
        if (times.empty()) {
            *time = GetDefaultTime().GetValue();
            return true;
        }

        // Use a single time samples.
        *time = times[0];
        return true;
    }

private:
    const Status _status;
    const draco::GeometryAttribute::Type _attributeType;
    const TfToken _name;
    const draco::DataType _dataType;
    const bool _isPrimvar;
    const size_t _numComponents;
    const Shape _shape;
    // Draco has no direct support for USD's 16-bit floating point primvars,
    // i.e., half, half2, half3, half4, and quath. Such primvars are stored in
    // place of 16-bit integer Draco attributes. To distinguish such attributes
    // from genuine 16-bit integers, Draco metadata entry is used.
    const bool _isHalf;
    // Draco has no animation support. USD attributes are exported to Draco if
    // their values and indices are only defined at a single time sample.
    // Otherwise, animated attributes remain in USD mesh.
    const UsdTimeCode _valuesTime;
    const UsdTimeCode _indicesTime;
    const TfToken _interpolation;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_PLUGIN_USD_DRACO_ATTRIBUTE_DESCRIPTOR_H
