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

#include "attributeFactory.h"

#include <typeinfo>

#include "pxr/pxr.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdGeom/tokens.h"


PXR_NAMESPACE_OPEN_SCOPE


draco::DataType UsdDracoAttributeFactory::GetDracoDataType(
    const std::type_info &typeInfo) {
    // Note that the fundamental data types supported by USD do not contain
    // int8_t, uint16_t, and int16_t. See the types.h header and the Basic data
    // types documentation.
    if (typeInfo == typeid(bool))
        return draco::DT_BOOL;
    if (typeInfo == typeid(uint8_t))
        return draco::DT_UINT8;
    if (typeInfo == typeid(int32_t) ||
        typeInfo == typeid(GfVec2i) ||
        typeInfo == typeid(GfVec3i) ||
        typeInfo == typeid(GfVec4i))
        return draco::DT_INT32;
    if (typeInfo == typeid(uint32_t))
        return draco::DT_UINT32;
    if (typeInfo == typeid(int64_t))
        return draco::DT_INT64;
    if (typeInfo == typeid(uint64_t))
        return draco::DT_UINT64;
    // USD halfs are stored as Draco 16-bit ints.
    if (typeInfo == typeid(GfHalf) ||
        typeInfo == typeid(GfVec2h) ||
        typeInfo == typeid(GfVec3h) ||
        typeInfo == typeid(GfVec4h) ||
        typeInfo == typeid(GfQuath))
        return draco::DT_INT16;
    if (typeInfo == typeid(float) ||
        typeInfo == typeid(GfVec2f) ||
        typeInfo == typeid(GfVec3f) ||
        typeInfo == typeid(GfVec4f) ||
        typeInfo == typeid(GfQuatf))
        return draco::DT_FLOAT32;
    if (typeInfo == typeid(double) ||
        typeInfo == typeid(GfVec2d) ||
        typeInfo == typeid(GfVec3d) ||
        typeInfo == typeid(GfVec4d) ||
        typeInfo == typeid(GfQuatd) ||
        typeInfo == typeid(GfMatrix2d) ||
        typeInfo == typeid(GfMatrix3d) ||
        typeInfo == typeid(GfMatrix4d))
        return draco::DT_FLOAT64;
    return draco::DT_INVALID;
}

UsdDracoAttributeDescriptor::Shape UsdDracoAttributeFactory::GetShape(
    const std::type_info &typeInfo) {
    if (typeInfo == typeid(bool) ||
        typeInfo == typeid(uint8_t) ||
        typeInfo == typeid(int32_t) ||
        typeInfo == typeid(uint32_t) ||
        typeInfo == typeid(int64_t) ||
        typeInfo == typeid(uint64_t) ||
        typeInfo == typeid(GfHalf) ||
        typeInfo == typeid(float) ||
        typeInfo == typeid(double) ||
        typeInfo == typeid(GfVec2i) ||
        typeInfo == typeid(GfVec3i) ||
        typeInfo == typeid(GfVec4i) ||
        typeInfo == typeid(GfVec2h) ||
        typeInfo == typeid(GfVec3h) ||
        typeInfo == typeid(GfVec4h) ||
        typeInfo == typeid(GfVec2f) ||
        typeInfo == typeid(GfVec3f) ||
        typeInfo == typeid(GfVec4f) ||
        typeInfo == typeid(GfVec2d) ||
        typeInfo == typeid(GfVec3d) ||
        typeInfo == typeid(GfVec4d))
        return UsdDracoAttributeDescriptor::VECTOR;
    if (typeInfo == typeid(GfQuath) ||
        typeInfo == typeid(GfQuatf) ||
        typeInfo == typeid(GfQuatd))
        return UsdDracoAttributeDescriptor::QUATERNION;
    if (typeInfo == typeid(GfMatrix2d) ||
        typeInfo == typeid(GfMatrix3d) ||
        typeInfo == typeid(GfMatrix4d))
        return UsdDracoAttributeDescriptor::MATRIX;
    return UsdDracoAttributeDescriptor::GetDefaultShape();
}

bool UsdDracoAttributeFactory::IsHalf(
    const std::type_info &typeInfo) {
    return typeInfo == typeid(GfHalf) ||
           typeInfo == typeid(GfVec2h) ||
           typeInfo == typeid(GfVec3h) ||
           typeInfo == typeid(GfVec4h) ||
           typeInfo == typeid(GfQuath);
}

SdfValueTypeName UsdDracoAttributeFactory::GetSdfValueTypeName(
    const UsdDracoAttributeDescriptor &descriptor) {
    switch (descriptor.GetShape()) {
        case UsdDracoAttributeDescriptor::MATRIX:
            // All matrices in USD have elements of type double.
            if (descriptor.GetDataType() != draco::DT_FLOAT64)
                break;
            switch (descriptor.GetNumComponents()) {
                case 4:  // 2-by-2 matrix.
                    return SdfValueTypeNames->Matrix2dArray;
                case 9:  // 3-by-3 matrix.
                    return SdfValueTypeNames->Matrix3dArray;
                case 16:  // 4-by-4 matrix.
                    return SdfValueTypeNames->Matrix4dArray;
                default:
                    break;
            }
            break;
        case UsdDracoAttributeDescriptor::QUATERNION:
            // Quaternion has four entries.
            if (descriptor.GetNumComponents() != 4)
                break;
            switch (descriptor.GetDataType()) {
                // USD halfs are stored as Draco 16-bit ints.
                case draco::DT_INT16:
                    if (descriptor.GetIsHalf())
                        return SdfValueTypeNames->QuathArray;
                    break;
                case draco::DT_FLOAT32:
                    return SdfValueTypeNames->QuatfArray;
                case draco::DT_FLOAT64:
                    return SdfValueTypeNames->QuatdArray;
                default:
                    break;
            }
            break;
        case UsdDracoAttributeDescriptor::VECTOR:
            switch (descriptor.GetNumComponents()) {
                case 1:  // Scalar.
                    switch (descriptor.GetDataType()) {
                        case draco::DT_UINT8:
                            return SdfValueTypeNames->UCharArray;
                        case draco::DT_INT32:
                            return SdfValueTypeNames->IntArray;
                        case draco::DT_UINT32:
                            return SdfValueTypeNames->UIntArray;
                        case draco::DT_INT64:
                            return SdfValueTypeNames->Int64Array;
                        case draco::DT_UINT64:
                            return SdfValueTypeNames->UInt64Array;
                        // USD halfs are stored as Draco 16-bit ints.
                        case draco::DT_INT16:
                            if (descriptor.GetIsHalf())
                                return SdfValueTypeNames->HalfArray;
                            break;
                        case draco::DT_FLOAT32:
                            return SdfValueTypeNames->FloatArray;
                        case draco::DT_FLOAT64:
                            return SdfValueTypeNames->DoubleArray;
                        case draco::DT_BOOL:
                            return SdfValueTypeNames->BoolArray;
                        default:
                            break;
                    }
                    break;
                case 2:  // Length-two vector.
                    switch (descriptor.GetDataType()) {
                        case draco::DT_INT32:
                            return SdfValueTypeNames->Int2Array;
                        // USD halfs are stored as Draco 16-bit ints.
                        case draco::DT_INT16:
                            if (descriptor.GetIsHalf())
                                return SdfValueTypeNames->Half2Array;
                            break;
                        case draco::DT_FLOAT32:
                            return SdfValueTypeNames->Float2Array;
                        case draco::DT_FLOAT64:
                            return SdfValueTypeNames->Double2Array;
                        default:
                            break;
                    }
                    break;
                case 3:  // Length-three vector.
                    switch (descriptor.GetDataType()) {
                        case draco::DT_INT32:
                            return SdfValueTypeNames->Int3Array;
                        // USD halfs are stored as Draco 16-bit ints.
                        case draco::DT_INT16:
                            if (descriptor.GetIsHalf())
                                return SdfValueTypeNames->Half3Array;
                            break;
                        case draco::DT_FLOAT32:
                            return SdfValueTypeNames->Float3Array;
                        case draco::DT_FLOAT64:
                            return SdfValueTypeNames->Double3Array;
                        default:
                            break;
                    }
                    break;
                case 4:  // Length-four vector.
                    switch (descriptor.GetDataType()) {
                        case draco::DT_INT32:
                            return SdfValueTypeNames->Int4Array;
                        // USD halfs are stored as Draco 16-bit ints.
                        case draco::DT_INT16:
                            if (descriptor.GetIsHalf())
                                return SdfValueTypeNames->Half4Array;
                            break;
                        case draco::DT_FLOAT32:
                            return SdfValueTypeNames->Float4Array;
                        case draco::DT_FLOAT64:
                            return SdfValueTypeNames->Double4Array;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
    }
    TF_RUNTIME_ERROR("Unsupported value type.");
    return SdfValueTypeName();
}


PXR_NAMESPACE_CLOSE_SCOPE
