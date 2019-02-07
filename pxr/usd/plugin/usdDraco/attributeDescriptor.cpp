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

#include "pxr/pxr.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdGeom/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


const std::string UsdDracoAttributeDescriptor::METADATA_NAME_KEY = "name";

UsdDracoAttributeDescriptor::UsdDracoAttributeDescriptor(
        draco::GeometryAttribute::Type attributeType,
        TfToken name,
        const std::string &metadataName,
        draco::DataType dataType,
        SdfValueTypeName valueType,
        bool isPrimvar,
        size_t numComponents) :
            attributeType(attributeType),
            name(name),
            metadataName(metadataName),
            dataType(dataType),
            valueType(valueType),
            isPrimvar(isPrimvar),
            numComponents(numComponents) {
}


namespace attributedescriptor {

UsdDracoAttributeDescriptor ForPositions() {
    return UsdDracoAttributeDescriptor(
        draco::GeometryAttribute::POSITION,
        UsdGeomTokens->points,
        "",  /* metadataName */
        draco::DT_FLOAT32,
        SdfValueTypeNames->Float3Array,
        false,  /* isPrimvar */
        3  /* numComponents */);
}

UsdDracoAttributeDescriptor ForTexCoords() {
    return UsdDracoAttributeDescriptor(
        draco::GeometryAttribute::TEX_COORD,
        TfToken("Texture_uv"),
        "",  /* metadataName */
        draco::DT_FLOAT32,
        SdfValueTypeNames->Float2Array,
        true,  /* isPrimvar */
        2  /* numComponents */);
}

UsdDracoAttributeDescriptor ForNormals() {
    return UsdDracoAttributeDescriptor(
        draco::GeometryAttribute::NORMAL,
        UsdGeomTokens->normals,
        "",  /* metadataName */
        draco::DT_FLOAT32,
        SdfValueTypeNames->Float3Array,
        true,  /* isPrimvar */
        3  /* numComponents */);
}

UsdDracoAttributeDescriptor ForHoleFaces() {
    return UsdDracoAttributeDescriptor(
        draco::GeometryAttribute::GENERIC,
        TfToken(),
        "hole_faces",
        draco::DT_UINT8,
        SdfValueTypeName(),
        false,  /* isPrimvar */
        1  /* numComponents */);
}

UsdDracoAttributeDescriptor ForAddedEdges() {
    return UsdDracoAttributeDescriptor(
        draco::GeometryAttribute::GENERIC,
        TfToken(),
        "added_edges",
        draco::DT_UINT8,
        SdfValueTypeName(),
        false,  /* isPrimvar */
        1  /* numComponents */);
}

UsdDracoAttributeDescriptor ForPosOrder() {
    return UsdDracoAttributeDescriptor(
        draco::GeometryAttribute::GENERIC,
        TfToken(),
        "point_order",
        draco::DT_UINT32,
        SdfValueTypeName(),
        false,  /* isPrimvar */
        1  /* numComponents */);
}

}  // namespace attributedescriptor

PXR_NAMESPACE_CLOSE_SCOPE
