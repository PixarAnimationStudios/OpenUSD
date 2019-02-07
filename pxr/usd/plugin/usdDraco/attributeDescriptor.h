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

#ifndef USDDRACO_ATTRIBUTE_DESCRIPTOR_H
#define USDDRACO_ATTRIBUTE_DESCRIPTOR_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/valueTypeName.h"

#include <draco/attributes/geometry_attribute.h>


PXR_NAMESPACE_OPEN_SCOPE


/// \struct AttributeDescriptor
///
/// Stores meta information about USD and Draco attributes, such as data type,
/// data dimensionality, name, etc.
///
struct UsdDracoAttributeDescriptor {
    static const std::string METADATA_NAME_KEY;
    const draco::GeometryAttribute::Type attributeType;
    const TfToken name;
    const std::string metadataName;
    const draco::DataType dataType;
    const SdfValueTypeName valueType;
    const bool isPrimvar;
    const size_t numComponents;

    UsdDracoAttributeDescriptor() = delete;
    UsdDracoAttributeDescriptor(draco::GeometryAttribute::Type attribteType,
                                TfToken name,
                                const std::string &metadataName,
                                draco::DataType dataType,
                                SdfValueTypeName valueType,
                                bool isPrimvar,
                                size_t numComponents);
};

namespace attributedescriptor {
    UsdDracoAttributeDescriptor ForPositions();
    UsdDracoAttributeDescriptor ForTexCoords();
    UsdDracoAttributeDescriptor ForNormals();
    UsdDracoAttributeDescriptor ForHoleFaces();
    UsdDracoAttributeDescriptor ForAddedEdges();
    UsdDracoAttributeDescriptor ForPosOrder();
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // USDDRACO_ATTRIBUTE_DESCRIPTOR_H
