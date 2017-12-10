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
#include "pxr/pxr.h"
#include "usdMaya/writeUtil.h"
#include "usdMaya/UserTaggedAttribute.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/inherits.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdRi/statements.h"

#include <maya/MDagPath.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnFloatArrayData.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MVectorArray.h>

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE



static
bool
_GetMayaAttributeNumericTypedAndUnitDataTypes(
        const MPlug& attrPlug,
        MFnNumericData::Type& numericDataType,
        MFnData::Type& typedDataType,
        MFnUnitAttribute::Type& unitDataType)
{
    numericDataType = MFnNumericData::kInvalid;
    typedDataType = MFnData::kInvalid;
    unitDataType = MFnUnitAttribute::kInvalid;

    MObject attrObj(attrPlug.attribute());
    if (attrObj.isNull()) {
        return false;
    }

    if (attrObj.hasFn(MFn::kNumericAttribute)) {
        MFnNumericAttribute numericAttrFn(attrObj);
        numericDataType = numericAttrFn.unitType();
    } else if (attrObj.hasFn(MFn::kTypedAttribute)) {
        MFnTypedAttribute typedAttrFn(attrObj);
        typedDataType = typedAttrFn.attrType();

        if (typedDataType == MFnData::kNumeric) {
            // Inspect the type of the data itself to find the actual type.
            MObject plugObj = attrPlug.asMObject();
            if (plugObj.hasFn(MFn::kNumericData)) {
                MFnNumericData numericDataFn(plugObj);
                numericDataType = numericDataFn.numericType();
            }
        }
    } else if (attrObj.hasFn(MFn::kUnitAttribute)) {
        MFnUnitAttribute unitAttrFn(attrObj);
        unitDataType = unitAttrFn.unitType();
    }

    return true;
}

SdfValueTypeName
PxrUsdMayaWriteUtil::GetUsdTypeName(
        const MPlug& attrPlug,
        const bool translateMayaDoubleToUsdSinglePrecision)
{
    // The various types of Maya attributes that can be created are spread
    // across a handful of MFn function sets. Some are a straightforward
    // translation such as MFnEnumAttributes or MFnMatrixAttributes, but others
    // are interesting mixes of function sets. For example, an attribute created
    // with addAttr and 'double' as the type results in an MFnNumericAttribute
    // while 'double2' as the type results in an MFnTypedAttribute that has
    // MFnData::Type kNumeric.

    MObject attrObj(attrPlug.attribute());
    if (attrObj.isNull()) {
        return SdfValueTypeName();
    }

    if (attrObj.hasFn(MFn::kEnumAttribute)) {
        return SdfValueTypeNames->Int;
    }

    MFnNumericData::Type numericDataType;
    MFnData::Type typedDataType;
    MFnUnitAttribute::Type unitDataType;

    _GetMayaAttributeNumericTypedAndUnitDataTypes(attrPlug,
                                                  numericDataType,
                                                  typedDataType,
                                                  unitDataType);

    if (attrObj.hasFn(MFn::kMatrixAttribute)) {
        // Using type "fltMatrix" with addAttr results in an MFnMatrixAttribute
        // while using type "matrix" results in an MFnTypedAttribute with type
        // kMatrix, but the data is extracted the same way for both.
        typedDataType = MFnData::kMatrix;
    }

    // Deal with the MFnTypedAttribute attributes first. If it is numeric, it
    // will fall through to the numericDataType switch below.
    switch (typedDataType) {
        case MFnData::kString:
            return SdfValueTypeNames->String;
            break;
        case MFnData::kMatrix:
            // This must be a Matrix4d even if
            // translateMayaDoubleToUsdSinglePrecision is true, since Matrix4f
            // is not supported in Sdf.
            return SdfValueTypeNames->Matrix4d;
            break;
        case MFnData::kStringArray:
            return SdfValueTypeNames->StringArray;
            break;
        case MFnData::kDoubleArray:
            if (translateMayaDoubleToUsdSinglePrecision) {
                return SdfValueTypeNames->FloatArray;
            } else {
                return SdfValueTypeNames->DoubleArray;
            }
            break;
        case MFnData::kFloatArray:
            return SdfValueTypeNames->FloatArray;
            break;
        case MFnData::kIntArray:
            return SdfValueTypeNames->IntArray;
            break;
        case MFnData::kPointArray:
            // Sdf does not have a 4-float point type, so we'll divide out W
            // and export the points as 3 floats.
            if (translateMayaDoubleToUsdSinglePrecision) {
                return SdfValueTypeNames->Point3fArray;
            } else {
                return SdfValueTypeNames->Point3dArray;
            }
            break;
        case MFnData::kVectorArray:
            if (translateMayaDoubleToUsdSinglePrecision) {
                return SdfValueTypeNames->Vector3fArray;
            } else {
                return SdfValueTypeNames->Vector3dArray;
            }
            break;
        default:
            break;
    }

    switch (numericDataType) {
        case MFnNumericData::kBoolean:
            return SdfValueTypeNames->Bool;
            break;
        case MFnNumericData::kByte:
        case MFnNumericData::kChar:
        case MFnNumericData::kShort:
        // Maya treats longs the same as ints, since long is not
        // platform-consistent. The Maya constants MFnNumericData::kInt and
        // MFnNumericData::kLong have the same value. The same is true of
        // k2Int/k2Long and k3Int/k3Long.
        case MFnNumericData::kInt:
            return SdfValueTypeNames->Int;
            break;
        case MFnNumericData::k2Short:
        case MFnNumericData::k2Int:
            return SdfValueTypeNames->Int2;
            break;
        case MFnNumericData::k3Short:
        case MFnNumericData::k3Int:
            return SdfValueTypeNames->Int3;
            break;
        case MFnNumericData::kFloat:
            return SdfValueTypeNames->Float;
            break;
        case MFnNumericData::k2Float:
            return SdfValueTypeNames->Float2;
            break;
        case MFnNumericData::k3Float:
            if (MFnAttribute(attrObj).isUsedAsColor()) {
                return SdfValueTypeNames->Color3f;
            } else {
                return SdfValueTypeNames->Float3;
            }
            break;
        case MFnNumericData::kDouble:
            if (translateMayaDoubleToUsdSinglePrecision) {
                return SdfValueTypeNames->Float;
            } else {
                return SdfValueTypeNames->Double;
            }
            break;
        case MFnNumericData::k2Double:
            if (translateMayaDoubleToUsdSinglePrecision) {
                return SdfValueTypeNames->Float2;
            } else {
                return SdfValueTypeNames->Double2;
            }
            break;
        case MFnNumericData::k3Double:
            if (MFnAttribute(attrObj).isUsedAsColor()) {
                if (translateMayaDoubleToUsdSinglePrecision) {
                    return SdfValueTypeNames->Color3f;
                } else {
                    return SdfValueTypeNames->Color3d;
                }
            } else {
                if (translateMayaDoubleToUsdSinglePrecision) {
                    return SdfValueTypeNames->Float3;
                } else {
                    return SdfValueTypeNames->Double3;
                }
            }
            break;
        case MFnNumericData::k4Double:
            if (translateMayaDoubleToUsdSinglePrecision) {
                return SdfValueTypeNames->Float4;
            } else {
                return SdfValueTypeNames->Double4;
            }
            break;
        default:
            break;
    }

    switch (unitDataType) {
        case MFnUnitAttribute::kAngle:
        case MFnUnitAttribute::kDistance:
            if (translateMayaDoubleToUsdSinglePrecision) {
                return SdfValueTypeNames->Float;
            } else {
                return SdfValueTypeNames->Double;
            }
            break;
        default:
            break;
    }

    return SdfValueTypeName();
}

/* static */
UsdAttribute
PxrUsdMayaWriteUtil::GetOrCreateUsdAttr(
        const MPlug& attrPlug,
        const UsdPrim& usdPrim,
        const std::string& attrName,
        const bool custom,
        const bool translateMayaDoubleToUsdSinglePrecision)
{
    UsdAttribute usdAttr;

    if (!usdPrim) {
        return usdAttr;
    }

    MObject attrObj(attrPlug.attribute());

    TfToken usdAttrNameToken(attrName);
    if (usdAttrNameToken.IsEmpty()) {
        MGlobal::displayError(
            TfStringPrintf("Invalid USD attribute name '%s' for Maya plug '%s'",
                           attrName.c_str(),
                           attrPlug.name().asChar()).c_str());
        return usdAttr;
    }

    // See if the USD attribute already exists. If so, return it.
    usdAttr = usdPrim.GetAttribute(usdAttrNameToken);
    if (usdAttr) {
        return usdAttr;
    }

    const SdfValueTypeName& typeName =
        PxrUsdMayaWriteUtil::GetUsdTypeName(attrPlug,
                                            translateMayaDoubleToUsdSinglePrecision);
    if (typeName) {
        usdAttr = usdPrim.CreateAttribute(usdAttrNameToken, typeName, custom);
    }

    return usdAttr;
}

/* static */
UsdGeomPrimvar PxrUsdMayaWriteUtil::GetOrCreatePrimvar(
        const MPlug& attrPlug,
        UsdGeomImageable& imageable,
        const std::string& primvarName,
        const TfToken& interpolation,
        const int elementSize,
        const bool translateMayaDoubleToUsdSinglePrecision)
{
    UsdGeomPrimvar primvar;

    if (!imageable) {
        return primvar;
    }

    MObject attrObj(attrPlug.attribute());

    TfToken primvarNameToken(primvarName);
    if (primvarNameToken.IsEmpty()) {
        MGlobal::displayError(
            TfStringPrintf("Invalid primvar name '%s' for Maya plug '%s'",
                           primvarName.c_str(),
                           attrPlug.name().asChar()).c_str());
        return primvar;
    }

    // See if the primvar already exists. If so, return it.
    primvar = imageable.GetPrimvar(primvarNameToken);
    if (primvar) {
        return primvar;
    }

    const SdfValueTypeName& typeName =
        PxrUsdMayaWriteUtil::GetUsdTypeName(attrPlug,
                                            translateMayaDoubleToUsdSinglePrecision);
    if (typeName) {
        primvar = imageable.CreatePrimvar(primvarNameToken,
                                          typeName,
                                          interpolation,
                                          elementSize);
    }

    return primvar;
}

/* static */
UsdAttribute PxrUsdMayaWriteUtil::GetOrCreateUsdRiAttribute(
        const MPlug& attrPlug,
        const UsdPrim& usdPrim,
        const std::string& attrName,
        const std::string& nameSpace,
        const bool translateMayaDoubleToUsdSinglePrecision)
{
    UsdAttribute usdAttr;

    if (!usdPrim) {
        return usdAttr;
    }

    MObject attrObj(attrPlug.attribute());

    TfToken riAttrNameToken(attrName);
    if (riAttrNameToken.IsEmpty()) {
        MGlobal::displayError(
            TfStringPrintf("Invalid UsdRi attribute name '%s' for Maya plug '%s'",
                           attrName.c_str(),
                           attrPlug.name().asChar()).c_str());
        return usdAttr;
    }

    UsdRiStatements riStatements(usdPrim);
    if (!riStatements) {
        return usdAttr;
    }

    // See if a UsdRi attribute with this name already exists. If so, return it.
    // XXX: There isn't currently API for looking for a specific UsdRi attribute
    // by name, so we have to get them all and then see if one matches.
    const std::vector<UsdProperty>& riAttrs = riStatements.GetRiAttributes(nameSpace);
    TF_FOR_ALL(iter, riAttrs) {
        if (iter->GetBaseName() == riAttrNameToken) {
            // Re-get the attribute from the prim so we can return it as a
            // UsdAttribute rather than a UsdProperty.
            return usdPrim.GetAttribute(iter->GetName());
        }
    }

    const SdfValueTypeName& typeName =
        PxrUsdMayaWriteUtil::GetUsdTypeName(attrPlug,
                                            translateMayaDoubleToUsdSinglePrecision);
    if (typeName) {
        usdAttr = riStatements.CreateRiAttribute(riAttrNameToken,
                                                 typeName.GetType(),
                                                 nameSpace);
    }

    return usdAttr;
}

template <typename T>
bool
_SetVec(
        const UsdAttribute& attr,
        const T& val,
        const UsdTimeCode& time) {
    return attr.Set((attr.GetRoleName() == SdfValueRoleNames->Color)
                        ? GfConvertDisplayToLinear(val)
                        : val,
                    time);
}

bool
PxrUsdMayaWriteUtil::SetUsdAttr(
        const MPlug& attrPlug,
        const UsdAttribute& usdAttr,
        const UsdTimeCode& usdTime,
        const bool translateMayaDoubleToUsdSinglePrecision)
{
    if (!usdAttr || attrPlug.isNull()) {
        return false;
    }

    bool isAnimated = attrPlug.isDestination();
    if (usdTime.IsDefault() == isAnimated) {
        return true;
    }

    // We perform a similar set of type-infererence acrobatics here as we do up
    // above in GetUsdTypeName(). See the comments there for more detail on a
    // few type-related oddities.

    MObject attrObj(attrPlug.attribute());

    if (attrObj.hasFn(MFn::kEnumAttribute)) {
        return usdAttr.Set(attrPlug.asInt(), usdTime);
    }

    MFnNumericData::Type numericDataType;
    MFnData::Type typedDataType;
    MFnUnitAttribute::Type unitDataType;

    _GetMayaAttributeNumericTypedAndUnitDataTypes(attrPlug,
                                                  numericDataType,
                                                  typedDataType,
                                                  unitDataType);

    if (attrObj.hasFn(MFn::kMatrixAttribute)) {
        typedDataType = MFnData::kMatrix;
    }

    switch (typedDataType) {
        case MFnData::kString: {
            MFnStringData stringDataFn(attrPlug.asMObject());
            const std::string usdVal(stringDataFn.string().asChar());
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kMatrix: {
            MFnMatrixData matrixDataFn(attrPlug.asMObject());
            const GfMatrix4d usdVal(matrixDataFn.matrix().matrix);
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kStringArray: {
            MFnStringArrayData stringArrayDataFn(attrPlug.asMObject());
            VtStringArray usdVal(stringArrayDataFn.length());
            for (unsigned int i = 0; i < stringArrayDataFn.length(); ++i) {
                usdVal[i] = std::string(stringArrayDataFn[i].asChar());
            }
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kDoubleArray: {
            MFnDoubleArrayData doubleArrayDataFn(attrPlug.asMObject());
            if (translateMayaDoubleToUsdSinglePrecision) {
                VtFloatArray usdVal(doubleArrayDataFn.length());
                for (unsigned int i = 0; i < doubleArrayDataFn.length(); ++i) {
                    usdVal[i] = (float)doubleArrayDataFn[i];
                }
                return usdAttr.Set(usdVal, usdTime);
            } else {
                VtDoubleArray usdVal(doubleArrayDataFn.length());
                for (unsigned int i = 0; i < doubleArrayDataFn.length(); ++i) {
                    usdVal[i] = doubleArrayDataFn[i];
                }
                return usdAttr.Set(usdVal, usdTime);
            }
            break;
        }
        case MFnData::kFloatArray: {
            MFnFloatArrayData floatArrayDataFn(attrPlug.asMObject());
            VtFloatArray usdVal(floatArrayDataFn.length());
            for (unsigned int i = 0; i < floatArrayDataFn.length(); ++i) {
                usdVal[i] = floatArrayDataFn[i];
            }
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kIntArray: {
            MFnIntArrayData intArrayDataFn(attrPlug.asMObject());
            VtIntArray usdVal(intArrayDataFn.length());
            for (unsigned int i = 0; i < intArrayDataFn.length(); ++i) {
                usdVal[i] = intArrayDataFn[i];
            }
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kPointArray: {
            MFnPointArrayData pointArrayDataFn(attrPlug.asMObject());
            if (translateMayaDoubleToUsdSinglePrecision) {
                VtVec3fArray usdVal(pointArrayDataFn.length());
                for (unsigned int i = 0; i < pointArrayDataFn.length(); ++i) {
                    MPoint tmpMayaVal = pointArrayDataFn[i];
                    if (tmpMayaVal.w != 0) {
                        tmpMayaVal.cartesianize();
                    }
                    usdVal[i] = GfVec3f((float)tmpMayaVal[0],
                                        (float)tmpMayaVal[1],
                                        (float)tmpMayaVal[2]);
                }
                return usdAttr.Set(usdVal, usdTime);
            } else {
                VtVec3dArray usdVal(pointArrayDataFn.length());
                for (unsigned int i = 0; i < pointArrayDataFn.length(); ++i) {
                    MPoint tmpMayaVal = pointArrayDataFn[i];
                    if (tmpMayaVal.w != 0) {
                        tmpMayaVal.cartesianize();
                    }
                    usdVal[i] = GfVec3d(tmpMayaVal[0],
                                        tmpMayaVal[1],
                                        tmpMayaVal[2]);
                }
                return usdAttr.Set(usdVal, usdTime);
            }
            break;
        }
        case MFnData::kVectorArray: {
            MFnVectorArrayData vectorArrayDataFn(attrPlug.asMObject());
            if (translateMayaDoubleToUsdSinglePrecision) {
                VtVec3fArray usdVal(vectorArrayDataFn.length());
                for (unsigned int i = 0; i < vectorArrayDataFn.length(); ++i) {
                    MVector tmpMayaVal = vectorArrayDataFn[i];
                    usdVal[i] = GfVec3f((float)tmpMayaVal[0],
                                        (float)tmpMayaVal[1],
                                        (float)tmpMayaVal[2]);
                }
                return usdAttr.Set(usdVal, usdTime);
            } else {
                VtVec3dArray usdVal(vectorArrayDataFn.length());
                for (unsigned int i = 0; i < vectorArrayDataFn.length(); ++i) {
                    MVector tmpMayaVal = vectorArrayDataFn[i];
                    usdVal[i] = GfVec3d(tmpMayaVal[0],
                                        tmpMayaVal[1],
                                        tmpMayaVal[2]);
                }
                return usdAttr.Set(usdVal, usdTime);
            }
            break;
        }
        default:
            break;
    }

    switch (numericDataType) {
        case MFnNumericData::kBoolean: {
            const bool usdVal(attrPlug.asBool());
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnNumericData::kByte:
        case MFnNumericData::kChar: {
            const int usdVal(attrPlug.asChar());
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnNumericData::kShort: {
            const int usdVal(attrPlug.asShort());
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnNumericData::kInt: {
            const int usdVal(attrPlug.asInt());
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnNumericData::k2Short: {
            short tmp1, tmp2;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2);
            return usdAttr.Set(GfVec2i(tmp1, tmp2), usdTime);
            break;
        }
        case MFnNumericData::k2Int: {
            int tmp1, tmp2;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2);
            return usdAttr.Set(GfVec2i(tmp1, tmp2), usdTime);
            break;
        }
        case MFnNumericData::k3Short: {
            short tmp1, tmp2, tmp3;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2, tmp3);
            return usdAttr.Set(GfVec3i(tmp1, tmp2, tmp3), usdTime);
            break;
        }
        case MFnNumericData::k3Int: {
            int tmp1, tmp2, tmp3;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2, tmp3);
            return usdAttr.Set(GfVec3i(tmp1, tmp2, tmp3), usdTime);
            break;
        }
        case MFnNumericData::kFloat: {
            const float usdVal(attrPlug.asFloat());
            return usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnNumericData::k2Float: {
            float tmp1, tmp2;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2);
            return usdAttr.Set(GfVec2f(tmp1, tmp2), usdTime);
            break;
        }
        case MFnNumericData::k3Float: {
            float tmp1, tmp2, tmp3;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2, tmp3);
            return _SetVec(usdAttr, GfVec3f(tmp1, tmp2, tmp3), usdTime);
            break;
        }
        case MFnNumericData::kDouble: {
            const double usdVal(attrPlug.asDouble());
            if (translateMayaDoubleToUsdSinglePrecision) {
                return usdAttr.Set((float)usdVal, usdTime);
            } else {
                return usdAttr.Set(usdVal, usdTime);
            }
            break;
        }
        case MFnNumericData::k2Double: {
            double tmp1, tmp2;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2);
            if (translateMayaDoubleToUsdSinglePrecision) {
                return usdAttr.Set(GfVec2f((float)tmp1, (float)tmp2), usdTime);
            } else {
                return usdAttr.Set(GfVec2d(tmp1, tmp2), usdTime);
            }
            break;
        }
        case MFnNumericData::k3Double: {
            double tmp1, tmp2, tmp3;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2, tmp3);
            if (translateMayaDoubleToUsdSinglePrecision) {
                return _SetVec(usdAttr,
                               GfVec3f((float)tmp1,
                                       (float)tmp2,
                                       (float)tmp3),
                               usdTime);
            } else {
                return _SetVec(usdAttr, GfVec3d(tmp1, tmp2, tmp3), usdTime);
            }
            break;
        }
        case MFnNumericData::k4Double: {
            double tmp1, tmp2, tmp3, tmp4;
            MFnNumericData numericDataFn(attrPlug.asMObject());
            numericDataFn.getData(tmp1, tmp2, tmp3, tmp4);
            if (translateMayaDoubleToUsdSinglePrecision) {
                return _SetVec(usdAttr,
                               GfVec4f((float)tmp1,
                                       (float)tmp2,
                                       (float)tmp3,
                                       (float)tmp4),
                               usdTime);
            } else {
                return _SetVec(usdAttr,
                               GfVec4d(tmp1, tmp2, tmp3, tmp4),
                               usdTime);
            }
            break;
        }
        default:
            break;
    }

    switch (unitDataType) {
        case MFnUnitAttribute::kAngle:
        case MFnUnitAttribute::kDistance:
            if (translateMayaDoubleToUsdSinglePrecision) {
                const float usdVal(attrPlug.asFloat());
                return usdAttr.Set(usdVal, usdTime);
            } else {
                const double usdVal(attrPlug.asDouble());
                return usdAttr.Set(usdVal, usdTime);
            }
            break;
        default:
            break;
    }

    return false;
}

// This method inspects the JSON blob stored in the 'USD_UserExportedAttributesJson'
// attribute on the Maya node at dagPath and exports any attributes specified
// there onto usdPrim at time usdTime. The JSON should contain an object that
// maps Maya attribute names to other JSON objects that contain metadata about
// how to export the attribute into USD. For example:
//
//    {
//        "myMayaAttributeOne": {
//        },
//        "myMayaAttributeTwo": {
//            "usdAttrName": "my:namespace:attributeTwo"
//        },
//        "attributeAsPrimvar": {
//            "usdAttrType": "primvar"
//        },
//        "attributeAsVertexInterpPrimvar": {
//            "usdAttrType": "primvar",
//            "interpolation": "vertex"
//        },
//        "attributeAsRibAttribute": {
//            "usdAttrType": "usdRi"
//        },
//        "doubleAttributeAsFloatAttribute": {
//            "translateMayaDoubleToUsdSinglePrecision": true
//        }
//    }
//
// If the attribute metadata contains a value for "usdAttrName", the attribute
// will be given that name in USD. Otherwise, the Maya attribute name will be
// used for primvars and UsdRi attributes, or the Maya attribute name prepended
// with the "userProperties" namespace will be used for regular USD attributes.
// Maya attributes in the JSON will be processed in sorted order, and any
// USD attribute name collisions will be resolved by using the first attribute
// visited and warning about subsequent attribute tags.
//
bool
PxrUsdMayaWriteUtil::WriteUserExportedAttributes(
        const MDagPath& dagPath,
        const UsdPrim& usdPrim,
        const UsdTimeCode& usdTime)
{
    std::vector<PxrUsdMayaUserTaggedAttribute> exportedAttributes =
        PxrUsdMayaUserTaggedAttribute::GetUserTaggedAttributesForNode(dagPath);
    for (const PxrUsdMayaUserTaggedAttribute& attr : exportedAttributes) {
        const std::string& usdAttrName = attr.GetUsdName();
        const TfToken& usdAttrType = attr.GetUsdType();
        const TfToken& interpolation = attr.GetUsdInterpolation();
        const bool translateMayaDoubleToUsdSinglePrecision =
            attr.GetTranslateMayaDoubleToUsdSinglePrecision();
        const MPlug& attrPlug = attr.GetMayaPlug();
        UsdAttribute usdAttr;

        if (usdAttrType ==
                    PxrUsdMayaUserTaggedAttributeTokens->USDAttrTypePrimvar) {
            UsdGeomImageable imageable(usdPrim);
            if (!imageable) {
                MGlobal::displayError(
                    TfStringPrintf(
                        "Cannot create primvar for non-UsdGeomImageable USD prim: '%s'",
                        usdPrim.GetPath().GetText()).c_str());
                continue;
            }
            UsdGeomPrimvar primvar =
                PxrUsdMayaWriteUtil::GetOrCreatePrimvar(attrPlug,
                                                        imageable,
                                                        usdAttrName,
                                                        interpolation,
                                                        -1,
                                                        translateMayaDoubleToUsdSinglePrecision);
            if (primvar) {
                usdAttr = primvar.GetAttr();
            }
        } else if (usdAttrType ==
                    PxrUsdMayaUserTaggedAttributeTokens->USDAttrTypeUsdRi) {
            usdAttr =
                PxrUsdMayaWriteUtil::GetOrCreateUsdRiAttribute(attrPlug,
                                                               usdPrim,
                                                               usdAttrName,
                                                               "user",
                                                               translateMayaDoubleToUsdSinglePrecision);
        } else {
            usdAttr = PxrUsdMayaWriteUtil::GetOrCreateUsdAttr(attrPlug,
                                                              usdPrim,
                                                              usdAttrName,
                                                              true,
                                                              translateMayaDoubleToUsdSinglePrecision);
        }

        if (usdAttr) {
            if (!PxrUsdMayaWriteUtil::SetUsdAttr(attrPlug,
                                                    usdAttr,
                                                    usdTime,
                                                    translateMayaDoubleToUsdSinglePrecision)) {
                MGlobal::displayError(
                    TfStringPrintf("Could not set value for attribute: '%s'",
                                   usdAttr.GetPath().GetText()).c_str());
                continue;
            }
        } else {
            MGlobal::displayError(
                TfStringPrintf("Could not create attribute '%s' for USD prim: '%s'",
                               usdAttrName.c_str(),
                               usdPrim.GetPath().GetText()).c_str());
                continue;
        }
    }

    return true;
}

// static
bool
PxrUsdMayaWriteUtil::WriteClassInherits(
        const UsdPrim& prim,
        const std::vector<std::string>& classNamesToInherit)
{
    if (classNamesToInherit.empty()) {
        return true;
    }

    for (const auto& className: classNamesToInherit) {
        if (!TfIsValidIdentifier(className)) {
            return false;
        }
    }

    UsdStagePtr stage = prim.GetStage();

    auto inherits = prim.GetInherits();
    for (const auto& className: classNamesToInherit) {
        const SdfPath inheritPath = SdfPath::AbsoluteRootPath().AppendChild(
                TfToken(className));
        UsdPrim classPrim = stage->CreateClassPrim(inheritPath);
        inherits.AddInherit(classPrim.GetPath());
    }
    return true;
}

bool
PxrUsdMayaWriteUtil::ReadMayaAttribute(
        const MFnDependencyNode& depNode,
        const MString& name,
        std::string* val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug plug = depNode.findPlug(name);
        MObject dataObj;

        if ( (plug.getValue(dataObj) == MS::kSuccess) &&
             (dataObj.hasFn(MFn::kStringData)) ) {

            (*val) = std::string(plug.asString().asChar());
            return true;
        }
    }

    return false;
}

bool
PxrUsdMayaWriteUtil::ReadMayaAttribute(
        const MFnDependencyNode& depNode,
        const MString& name,
        std::vector<std::string>* val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug plug = depNode.findPlug(name);
        MObject dataObj;

        if ( (plug.getValue(dataObj) == MS::kSuccess) &&
             (dataObj.hasFn(MFn::kStringArrayData)) ) {

            MFnStringArrayData dData(dataObj, &status);
            if (status == MS::kSuccess) {
                MStringArray arrayValues = dData.array();
                size_t numValues = arrayValues.length();
                val->resize(numValues);
                for (size_t i = 0; i < numValues; ++i) {
                    (*val)[i] = std::string(arrayValues[i].asChar());
                }
                return true;
            }
        }
    }

    return false;
}

bool
PxrUsdMayaWriteUtil::ReadMayaAttribute(
        const MFnDependencyNode& depNode,
        const MString& name,
        VtIntArray* val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug plug = depNode.findPlug(name);
        MObject dataObj;

        if ( (plug.getValue(dataObj) == MS::kSuccess) &&
             (dataObj.hasFn(MFn::kIntArrayData)) ) {

            MFnIntArrayData dData(dataObj, &status);
            if (status == MS::kSuccess) {
                MIntArray arrayValues = dData.array();
                size_t numValues = arrayValues.length();
                val->resize(numValues);
                for (size_t i = 0; i < numValues; ++i) {
                    (*val)[i] = arrayValues[i];
                }
                return true;
            }
        }
    }

    return false;
}

bool
PxrUsdMayaWriteUtil::ReadMayaAttribute(
        const MFnDependencyNode& depNode,
        const MString& name,
        VtFloatArray* val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug plug = depNode.findPlug(name);
        MObject dataObj;

        if ( (plug.getValue(dataObj) == MS::kSuccess) &&
             (dataObj.hasFn(MFn::kDoubleArrayData)) ) {

            MFnDoubleArrayData dData(dataObj, &status);
            if (status == MS::kSuccess) {
                MDoubleArray arrayValues = dData.array();
                size_t numValues = arrayValues.length();
                val->resize(numValues);
                for (size_t i = 0; i < numValues; ++i) {
                    (*val)[i] = arrayValues[i];
                }
                return true;
            }
        }
    }

    return false;
}

bool
PxrUsdMayaWriteUtil::ReadMayaAttribute(
        const MFnDependencyNode& depNode,
        const MString& name,
        VtVec3fArray* val)
{
    MStatus status;
    depNode.attribute(name, &status);

    if (status == MS::kSuccess) {
        MPlug plug = depNode.findPlug(name);
        MObject dataObj;

        if ( (plug.getValue(dataObj) == MS::kSuccess) &&
             (dataObj.hasFn(MFn::kVectorArrayData)) ) {

            MFnVectorArrayData dData(dataObj, &status);
            if (status == MS::kSuccess) {
                MVectorArray arrayValues = dData.array();
                size_t numValues = arrayValues.length();
                val->resize(numValues);
                for (size_t i = 0; i < numValues; ++i) {
                    (*val)[i].Set(
                            arrayValues[i][0],
                            arrayValues[i][1],
                            arrayValues[i][2]);
                }
                return true;
            }
        }
    }

    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE

