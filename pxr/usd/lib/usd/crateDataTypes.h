//
// Copyright 2016 Pixar
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
// Note that changing any of the enum value numbers will break backward
// compatibility.  Adding additional types will not.
//
// Also note that the enumerant value 0 is reserved, and corresponds to the
// enumerant 'Invalid'.
#include "pxr/pxr.h"

// xx(<enumerant>, <enumerant-value>, <c++ type>, <supportsArray>)

// Array types.
xx(Bool,          1, bool,         true)
xx(UChar,         2, uint8_t,      true)
xx(Int,           3, int,          true)
xx(UInt,          4, unsigned int, true)
xx(Int64,         5, int64_t,      true)
xx(UInt64,        6, uint64_t,     true)

xx(Half,          7, GfHalf,       true)
xx(Float,         8, float,        true)
xx(Double,        9, double,       true)

xx(String,       10, std::string,  true)

xx(Token,        11, TfToken,      true)

xx(AssetPath,    12, SdfAssetPath, true)

xx(Quatd,        16, GfQuatd,      true)
xx(Quatf,        17, GfQuatf,      true)
xx(Quath,        18, GfQuath,      true)

xx(Vec2d,        19, GfVec2d,      true)
xx(Vec2f,        20, GfVec2f,      true)
xx(Vec2h,        21, GfVec2h,      true)
xx(Vec2i,        22, GfVec2i,      true)

xx(Vec3d,        23, GfVec3d,      true)
xx(Vec3f,        24, GfVec3f,      true)
xx(Vec3h,        25, GfVec3h,      true)
xx(Vec3i,        26, GfVec3i,      true)

xx(Vec4d,        27, GfVec4d,      true)
xx(Vec4f,        28, GfVec4f,      true)
xx(Vec4h,        29, GfVec4h,      true)
xx(Vec4i,        30, GfVec4i,      true)

xx(Matrix2d,     13, GfMatrix2d,   true)
xx(Matrix3d,     14, GfMatrix3d,   true)
xx(Matrix4d,     15, GfMatrix4d,   true)


// Non-array types.
xx(Dictionary,       31, VtDictionary,          false)

xx(TokenListOp,      32, SdfTokenListOp,        false)
xx(StringListOp,     33, SdfStringListOp,       false)
xx(PathListOp,       34, SdfPathListOp,         false)
xx(ReferenceListOp,  35, SdfReferenceListOp,    false)
xx(IntListOp,        36, SdfIntListOp,          false)
xx(Int64ListOp,      37, SdfInt64ListOp,        false)
xx(UIntListOp,       38, SdfUIntListOp,         false)
xx(UInt64ListOp,     39, SdfUInt64ListOp,       false)

xx(PathVector,       40, SdfPathVector,         false)
xx(TokenVector,      41, std::vector<TfToken>,  false)

xx(Specifier,        42, SdfSpecifier,          false)
xx(Permission,       43, SdfPermission,         false)
xx(Variability,      44, SdfVariability,        false)


xx(VariantSelectionMap, 45, SdfVariantSelectionMap,      false)
xx(TimeSamples,         46, TimeSamples,                 false)
xx(Payload,             47, SdfPayload,                  false)

xx(DoubleVector,        48, std::vector<double>,         false)
xx(LayerOffsetVector,   49, std::vector<SdfLayerOffset>, false)
xx(StringVector,        50, std::vector<std::string>,    false)

xx(ValueBlock,          51, SdfValueBlock,               false)
xx(Value,               52, VtValue,                     false)

xx(UnregisteredValue,   53, SdfUnregisteredValue,        false)
xx(UnregisteredValueListOp, 54, SdfUnregisteredValueListOp, false)
xx(PayloadListOp,       55, SdfPayloadListOp,            false)

