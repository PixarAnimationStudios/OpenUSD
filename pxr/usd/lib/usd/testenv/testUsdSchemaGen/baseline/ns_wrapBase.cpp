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
#include "pxr/usd/usdContrived/base.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

using namespace foo;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateMyVaryingTokenAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyVaryingTokenAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateMyUniformBoolAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyUniformBoolAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateMyDoubleAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyDoubleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateBoolAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBoolAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateUcharAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUcharAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UChar), writeSparsely);
}
        
static UsdAttribute
_CreateIntAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateIntAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateUintAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UInt), writeSparsely);
}
        
static UsdAttribute
_CreateInt64Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInt64Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int64), writeSparsely);
}
        
static UsdAttribute
_CreateUint64Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUint64Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UInt64), writeSparsely);
}
        
static UsdAttribute
_CreateHalfAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHalfAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half), writeSparsely);
}
        
static UsdAttribute
_CreateFloatAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFloatAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateDoubleAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDoubleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateStringAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateStringAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateTokenAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTokenAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateAssetAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAssetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateInt2Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInt2Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int2), writeSparsely);
}
        
static UsdAttribute
_CreateInt3Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInt3Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int3), writeSparsely);
}
        
static UsdAttribute
_CreateInt4Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInt4Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int4), writeSparsely);
}
        
static UsdAttribute
_CreateHalf2Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHalf2Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half2), writeSparsely);
}
        
static UsdAttribute
_CreateHalf3Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHalf3Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3), writeSparsely);
}
        
static UsdAttribute
_CreateHalf4Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHalf4Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half4), writeSparsely);
}
        
static UsdAttribute
_CreateFloat2Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFloat2Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateFloat3Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFloat3Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3), writeSparsely);
}
        
static UsdAttribute
_CreateFloat4Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFloat4Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float4), writeSparsely);
}
        
static UsdAttribute
_CreateDouble2Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDouble2Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2), writeSparsely);
}
        
static UsdAttribute
_CreateDouble3Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDouble3Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double3), writeSparsely);
}
        
static UsdAttribute
_CreateDouble4Attr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDouble4Attr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double4), writeSparsely);
}
        
static UsdAttribute
_CreatePoint3hAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePoint3hAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3h), writeSparsely);
}
        
static UsdAttribute
_CreatePoint3fAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePoint3fAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3f), writeSparsely);
}
        
static UsdAttribute
_CreatePoint3dAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePoint3dAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3d), writeSparsely);
}
        
static UsdAttribute
_CreateVector3dAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVector3dAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3d), writeSparsely);
}
        
static UsdAttribute
_CreateVector3fAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVector3fAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3f), writeSparsely);
}
        
static UsdAttribute
_CreateVector3hAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVector3hAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3h), writeSparsely);
}
        
static UsdAttribute
_CreateNormal3dAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateNormal3dAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3d), writeSparsely);
}
        
static UsdAttribute
_CreateNormal3fAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateNormal3fAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3f), writeSparsely);
}
        
static UsdAttribute
_CreateNormal3hAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateNormal3hAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3h), writeSparsely);
}
        
static UsdAttribute
_CreateColor3dAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor3dAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3d), writeSparsely);
}
        
static UsdAttribute
_CreateColor3fAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor3fAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateColor3hAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor3hAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3h), writeSparsely);
}
        
static UsdAttribute
_CreateColor4dAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor4dAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color4d), writeSparsely);
}
        
static UsdAttribute
_CreateColor4fAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor4fAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color4f), writeSparsely);
}
        
static UsdAttribute
_CreateColor4hAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor4hAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color4h), writeSparsely);
}
        
static UsdAttribute
_CreateQuatdAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateQuatdAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Quatd), writeSparsely);
}
        
static UsdAttribute
_CreateQuatfAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateQuatfAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Quatf), writeSparsely);
}
        
static UsdAttribute
_CreateQuathAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateQuathAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Quath), writeSparsely);
}
        
static UsdAttribute
_CreateMatrix2dAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMatrix2dAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix2d), writeSparsely);
}
        
static UsdAttribute
_CreateMatrix3dAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMatrix3dAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix3d), writeSparsely);
}
        
static UsdAttribute
_CreateMatrix4dAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMatrix4dAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4d), writeSparsely);
}
        
static UsdAttribute
_CreateFrame4dAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFrame4dAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Frame4d), writeSparsely);
}
        
static UsdAttribute
_CreateBoolArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBoolArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->BoolArray), writeSparsely);
}
        
static UsdAttribute
_CreateUcharArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUcharArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UCharArray), writeSparsely);
}
        
static UsdAttribute
_CreateIntArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateIntArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateUintArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUintArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UIntArray), writeSparsely);
}
        
static UsdAttribute
_CreateInt64ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInt64ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int64Array), writeSparsely);
}
        
static UsdAttribute
_CreateUint64ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUint64ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UInt64Array), writeSparsely);
}
        
static UsdAttribute
_CreateHalfArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHalfArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->HalfArray), writeSparsely);
}
        
static UsdAttribute
_CreateFloatArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFloatArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateDoubleArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDoubleArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateStringArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateStringArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateTokenArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTokenArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateAssetArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAssetArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->AssetArray), writeSparsely);
}
        
static UsdAttribute
_CreateInt2ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInt2ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int2Array), writeSparsely);
}
        
static UsdAttribute
_CreateInt3ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInt3ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int3Array), writeSparsely);
}
        
static UsdAttribute
_CreateInt4ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInt4ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int4Array), writeSparsely);
}
        
static UsdAttribute
_CreateHalf2ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHalf2ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half2Array), writeSparsely);
}
        
static UsdAttribute
_CreateHalf3ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHalf3ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}
        
static UsdAttribute
_CreateHalf4ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHalf4ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half4Array), writeSparsely);
}
        
static UsdAttribute
_CreateFloat2ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFloat2ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2Array), writeSparsely);
}
        
static UsdAttribute
_CreateFloat3ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFloat3ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateFloat4ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFloat4ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float4Array), writeSparsely);
}
        
static UsdAttribute
_CreateDouble2ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDouble2ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2Array), writeSparsely);
}
        
static UsdAttribute
_CreateDouble3ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDouble3ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double3Array), writeSparsely);
}
        
static UsdAttribute
_CreateDouble4ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDouble4ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double4Array), writeSparsely);
}
        
static UsdAttribute
_CreatePoint3hArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePoint3hArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3hArray), writeSparsely);
}
        
static UsdAttribute
_CreatePoint3fArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePoint3fArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreatePoint3dArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePoint3dArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3dArray), writeSparsely);
}
        
static UsdAttribute
_CreateVector3hArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVector3hArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3hArray), writeSparsely);
}
        
static UsdAttribute
_CreateVector3fArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVector3fArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateVector3dArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVector3dArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3dArray), writeSparsely);
}
        
static UsdAttribute
_CreateNormal3hArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateNormal3hArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3hArray), writeSparsely);
}
        
static UsdAttribute
_CreateNormal3fArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateNormal3fArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateNormal3dArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateNormal3dArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3dArray), writeSparsely);
}
        
static UsdAttribute
_CreateColor3hArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor3hArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3hArray), writeSparsely);
}
        
static UsdAttribute
_CreateColor3fArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor3fArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateColor3dArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor3dArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3dArray), writeSparsely);
}
        
static UsdAttribute
_CreateColor4hArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor4hArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color4hArray), writeSparsely);
}
        
static UsdAttribute
_CreateColor4fArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor4fArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color4fArray), writeSparsely);
}
        
static UsdAttribute
_CreateColor4dArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColor4dArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color4dArray), writeSparsely);
}
        
static UsdAttribute
_CreateQuathArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateQuathArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuathArray), writeSparsely);
}
        
static UsdAttribute
_CreateQuatfArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateQuatfArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuatfArray), writeSparsely);
}
        
static UsdAttribute
_CreateQuatdArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateQuatdArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuatdArray), writeSparsely);
}
        
static UsdAttribute
_CreateMatrix2dArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMatrix2dArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix2dArray), writeSparsely);
}
        
static UsdAttribute
_CreateMatrix3dArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMatrix3dArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix3dArray), writeSparsely);
}
        
static UsdAttribute
_CreateMatrix4dArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMatrix4dArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Matrix4dArray), writeSparsely);
}
        
static UsdAttribute
_CreateFrame4dArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFrame4dArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Frame4dArray), writeSparsely);
}

} // anonymous namespace

void wrapUsdContrivedBase()
{
    typedef UsdContrivedBase This;

    class_<This, bases<UsdTyped> >
        cls("Base");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("IsConcrete",
            static_cast<bool (*)(void)>( [](){ return This::IsConcrete; }))
        .staticmethod("IsConcrete")

        .def("IsTyped",
            static_cast<bool (*)(void)>( [](){ return This::IsTyped; } ))
        .staticmethod("IsTyped")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetMyVaryingTokenAttr",
             &This::GetMyVaryingTokenAttr)
        .def("CreateMyVaryingTokenAttr",
             &_CreateMyVaryingTokenAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyUniformBoolAttr",
             &This::GetMyUniformBoolAttr)
        .def("CreateMyUniformBoolAttr",
             &_CreateMyUniformBoolAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyDoubleAttr",
             &This::GetMyDoubleAttr)
        .def("CreateMyDoubleAttr",
             &_CreateMyDoubleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBoolAttr",
             &This::GetBoolAttr)
        .def("CreateBoolAttr",
             &_CreateBoolAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUcharAttr",
             &This::GetUcharAttr)
        .def("CreateUcharAttr",
             &_CreateUcharAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetIntAttr",
             &This::GetIntAttr)
        .def("CreateIntAttr",
             &_CreateIntAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUintAttr",
             &This::GetUintAttr)
        .def("CreateUintAttr",
             &_CreateUintAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInt64Attr",
             &This::GetInt64Attr)
        .def("CreateInt64Attr",
             &_CreateInt64Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUint64Attr",
             &This::GetUint64Attr)
        .def("CreateUint64Attr",
             &_CreateUint64Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHalfAttr",
             &This::GetHalfAttr)
        .def("CreateHalfAttr",
             &_CreateHalfAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFloatAttr",
             &This::GetFloatAttr)
        .def("CreateFloatAttr",
             &_CreateFloatAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDoubleAttr",
             &This::GetDoubleAttr)
        .def("CreateDoubleAttr",
             &_CreateDoubleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetStringAttr",
             &This::GetStringAttr)
        .def("CreateStringAttr",
             &_CreateStringAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTokenAttr",
             &This::GetTokenAttr)
        .def("CreateTokenAttr",
             &_CreateTokenAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAssetAttr",
             &This::GetAssetAttr)
        .def("CreateAssetAttr",
             &_CreateAssetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInt2Attr",
             &This::GetInt2Attr)
        .def("CreateInt2Attr",
             &_CreateInt2Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInt3Attr",
             &This::GetInt3Attr)
        .def("CreateInt3Attr",
             &_CreateInt3Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInt4Attr",
             &This::GetInt4Attr)
        .def("CreateInt4Attr",
             &_CreateInt4Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHalf2Attr",
             &This::GetHalf2Attr)
        .def("CreateHalf2Attr",
             &_CreateHalf2Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHalf3Attr",
             &This::GetHalf3Attr)
        .def("CreateHalf3Attr",
             &_CreateHalf3Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHalf4Attr",
             &This::GetHalf4Attr)
        .def("CreateHalf4Attr",
             &_CreateHalf4Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFloat2Attr",
             &This::GetFloat2Attr)
        .def("CreateFloat2Attr",
             &_CreateFloat2Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFloat3Attr",
             &This::GetFloat3Attr)
        .def("CreateFloat3Attr",
             &_CreateFloat3Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFloat4Attr",
             &This::GetFloat4Attr)
        .def("CreateFloat4Attr",
             &_CreateFloat4Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDouble2Attr",
             &This::GetDouble2Attr)
        .def("CreateDouble2Attr",
             &_CreateDouble2Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDouble3Attr",
             &This::GetDouble3Attr)
        .def("CreateDouble3Attr",
             &_CreateDouble3Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDouble4Attr",
             &This::GetDouble4Attr)
        .def("CreateDouble4Attr",
             &_CreateDouble4Attr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPoint3hAttr",
             &This::GetPoint3hAttr)
        .def("CreatePoint3hAttr",
             &_CreatePoint3hAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPoint3fAttr",
             &This::GetPoint3fAttr)
        .def("CreatePoint3fAttr",
             &_CreatePoint3fAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPoint3dAttr",
             &This::GetPoint3dAttr)
        .def("CreatePoint3dAttr",
             &_CreatePoint3dAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVector3dAttr",
             &This::GetVector3dAttr)
        .def("CreateVector3dAttr",
             &_CreateVector3dAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVector3fAttr",
             &This::GetVector3fAttr)
        .def("CreateVector3fAttr",
             &_CreateVector3fAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVector3hAttr",
             &This::GetVector3hAttr)
        .def("CreateVector3hAttr",
             &_CreateVector3hAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetNormal3dAttr",
             &This::GetNormal3dAttr)
        .def("CreateNormal3dAttr",
             &_CreateNormal3dAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetNormal3fAttr",
             &This::GetNormal3fAttr)
        .def("CreateNormal3fAttr",
             &_CreateNormal3fAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetNormal3hAttr",
             &This::GetNormal3hAttr)
        .def("CreateNormal3hAttr",
             &_CreateNormal3hAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor3dAttr",
             &This::GetColor3dAttr)
        .def("CreateColor3dAttr",
             &_CreateColor3dAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor3fAttr",
             &This::GetColor3fAttr)
        .def("CreateColor3fAttr",
             &_CreateColor3fAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor3hAttr",
             &This::GetColor3hAttr)
        .def("CreateColor3hAttr",
             &_CreateColor3hAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor4dAttr",
             &This::GetColor4dAttr)
        .def("CreateColor4dAttr",
             &_CreateColor4dAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor4fAttr",
             &This::GetColor4fAttr)
        .def("CreateColor4fAttr",
             &_CreateColor4fAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor4hAttr",
             &This::GetColor4hAttr)
        .def("CreateColor4hAttr",
             &_CreateColor4hAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetQuatdAttr",
             &This::GetQuatdAttr)
        .def("CreateQuatdAttr",
             &_CreateQuatdAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetQuatfAttr",
             &This::GetQuatfAttr)
        .def("CreateQuatfAttr",
             &_CreateQuatfAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetQuathAttr",
             &This::GetQuathAttr)
        .def("CreateQuathAttr",
             &_CreateQuathAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMatrix2dAttr",
             &This::GetMatrix2dAttr)
        .def("CreateMatrix2dAttr",
             &_CreateMatrix2dAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMatrix3dAttr",
             &This::GetMatrix3dAttr)
        .def("CreateMatrix3dAttr",
             &_CreateMatrix3dAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMatrix4dAttr",
             &This::GetMatrix4dAttr)
        .def("CreateMatrix4dAttr",
             &_CreateMatrix4dAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFrame4dAttr",
             &This::GetFrame4dAttr)
        .def("CreateFrame4dAttr",
             &_CreateFrame4dAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBoolArrayAttr",
             &This::GetBoolArrayAttr)
        .def("CreateBoolArrayAttr",
             &_CreateBoolArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUcharArrayAttr",
             &This::GetUcharArrayAttr)
        .def("CreateUcharArrayAttr",
             &_CreateUcharArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetIntArrayAttr",
             &This::GetIntArrayAttr)
        .def("CreateIntArrayAttr",
             &_CreateIntArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUintArrayAttr",
             &This::GetUintArrayAttr)
        .def("CreateUintArrayAttr",
             &_CreateUintArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInt64ArrayAttr",
             &This::GetInt64ArrayAttr)
        .def("CreateInt64ArrayAttr",
             &_CreateInt64ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUint64ArrayAttr",
             &This::GetUint64ArrayAttr)
        .def("CreateUint64ArrayAttr",
             &_CreateUint64ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHalfArrayAttr",
             &This::GetHalfArrayAttr)
        .def("CreateHalfArrayAttr",
             &_CreateHalfArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFloatArrayAttr",
             &This::GetFloatArrayAttr)
        .def("CreateFloatArrayAttr",
             &_CreateFloatArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDoubleArrayAttr",
             &This::GetDoubleArrayAttr)
        .def("CreateDoubleArrayAttr",
             &_CreateDoubleArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetStringArrayAttr",
             &This::GetStringArrayAttr)
        .def("CreateStringArrayAttr",
             &_CreateStringArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTokenArrayAttr",
             &This::GetTokenArrayAttr)
        .def("CreateTokenArrayAttr",
             &_CreateTokenArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAssetArrayAttr",
             &This::GetAssetArrayAttr)
        .def("CreateAssetArrayAttr",
             &_CreateAssetArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInt2ArrayAttr",
             &This::GetInt2ArrayAttr)
        .def("CreateInt2ArrayAttr",
             &_CreateInt2ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInt3ArrayAttr",
             &This::GetInt3ArrayAttr)
        .def("CreateInt3ArrayAttr",
             &_CreateInt3ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInt4ArrayAttr",
             &This::GetInt4ArrayAttr)
        .def("CreateInt4ArrayAttr",
             &_CreateInt4ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHalf2ArrayAttr",
             &This::GetHalf2ArrayAttr)
        .def("CreateHalf2ArrayAttr",
             &_CreateHalf2ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHalf3ArrayAttr",
             &This::GetHalf3ArrayAttr)
        .def("CreateHalf3ArrayAttr",
             &_CreateHalf3ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHalf4ArrayAttr",
             &This::GetHalf4ArrayAttr)
        .def("CreateHalf4ArrayAttr",
             &_CreateHalf4ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFloat2ArrayAttr",
             &This::GetFloat2ArrayAttr)
        .def("CreateFloat2ArrayAttr",
             &_CreateFloat2ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFloat3ArrayAttr",
             &This::GetFloat3ArrayAttr)
        .def("CreateFloat3ArrayAttr",
             &_CreateFloat3ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFloat4ArrayAttr",
             &This::GetFloat4ArrayAttr)
        .def("CreateFloat4ArrayAttr",
             &_CreateFloat4ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDouble2ArrayAttr",
             &This::GetDouble2ArrayAttr)
        .def("CreateDouble2ArrayAttr",
             &_CreateDouble2ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDouble3ArrayAttr",
             &This::GetDouble3ArrayAttr)
        .def("CreateDouble3ArrayAttr",
             &_CreateDouble3ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDouble4ArrayAttr",
             &This::GetDouble4ArrayAttr)
        .def("CreateDouble4ArrayAttr",
             &_CreateDouble4ArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPoint3hArrayAttr",
             &This::GetPoint3hArrayAttr)
        .def("CreatePoint3hArrayAttr",
             &_CreatePoint3hArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPoint3fArrayAttr",
             &This::GetPoint3fArrayAttr)
        .def("CreatePoint3fArrayAttr",
             &_CreatePoint3fArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPoint3dArrayAttr",
             &This::GetPoint3dArrayAttr)
        .def("CreatePoint3dArrayAttr",
             &_CreatePoint3dArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVector3hArrayAttr",
             &This::GetVector3hArrayAttr)
        .def("CreateVector3hArrayAttr",
             &_CreateVector3hArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVector3fArrayAttr",
             &This::GetVector3fArrayAttr)
        .def("CreateVector3fArrayAttr",
             &_CreateVector3fArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVector3dArrayAttr",
             &This::GetVector3dArrayAttr)
        .def("CreateVector3dArrayAttr",
             &_CreateVector3dArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetNormal3hArrayAttr",
             &This::GetNormal3hArrayAttr)
        .def("CreateNormal3hArrayAttr",
             &_CreateNormal3hArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetNormal3fArrayAttr",
             &This::GetNormal3fArrayAttr)
        .def("CreateNormal3fArrayAttr",
             &_CreateNormal3fArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetNormal3dArrayAttr",
             &This::GetNormal3dArrayAttr)
        .def("CreateNormal3dArrayAttr",
             &_CreateNormal3dArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor3hArrayAttr",
             &This::GetColor3hArrayAttr)
        .def("CreateColor3hArrayAttr",
             &_CreateColor3hArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor3fArrayAttr",
             &This::GetColor3fArrayAttr)
        .def("CreateColor3fArrayAttr",
             &_CreateColor3fArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor3dArrayAttr",
             &This::GetColor3dArrayAttr)
        .def("CreateColor3dArrayAttr",
             &_CreateColor3dArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor4hArrayAttr",
             &This::GetColor4hArrayAttr)
        .def("CreateColor4hArrayAttr",
             &_CreateColor4hArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor4fArrayAttr",
             &This::GetColor4fArrayAttr)
        .def("CreateColor4fArrayAttr",
             &_CreateColor4fArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColor4dArrayAttr",
             &This::GetColor4dArrayAttr)
        .def("CreateColor4dArrayAttr",
             &_CreateColor4dArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetQuathArrayAttr",
             &This::GetQuathArrayAttr)
        .def("CreateQuathArrayAttr",
             &_CreateQuathArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetQuatfArrayAttr",
             &This::GetQuatfArrayAttr)
        .def("CreateQuatfArrayAttr",
             &_CreateQuatfArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetQuatdArrayAttr",
             &This::GetQuatdArrayAttr)
        .def("CreateQuatdArrayAttr",
             &_CreateQuatdArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMatrix2dArrayAttr",
             &This::GetMatrix2dArrayAttr)
        .def("CreateMatrix2dArrayAttr",
             &_CreateMatrix2dArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMatrix3dArrayAttr",
             &This::GetMatrix3dArrayAttr)
        .def("CreateMatrix3dArrayAttr",
             &_CreateMatrix3dArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMatrix4dArrayAttr",
             &This::GetMatrix4dArrayAttr)
        .def("CreateMatrix4dArrayAttr",
             &_CreateMatrix4dArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFrame4dArrayAttr",
             &This::GetFrame4dArrayAttr)
        .def("CreateFrame4dArrayAttr",
             &_CreateFrame4dArrayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace {

WRAP_CUSTOM {
}

}
