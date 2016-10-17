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
#include "usdMaya/writeUtil.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/js/json.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdRi/statements.h"

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
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnVectorArrayData.h>

#include <maya/MDagPath.h>
#include <maya/MDoubleArray.h>
#include <maya/MGlobal.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MVector.h>
#include <maya/MVectorArray.h>

#include <set>
#include <string>
#include <vector>


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (USD_UserExportedAttributesJson)
    (usdAttrName)
    (usdAttrType)
    ((USDAttrTypePrimvar, "primvar"))
    ((USDAttrTypeUsdRi, "usdRi"))
    ((UserPropertiesNamespace, "userProperties:"))
);


SdfValueTypeName
PxrUsdMayaWriteUtil::GetUsdTypeName( const MPlug& plg)
{
    MObject attrObj(plg.attribute());

    SdfValueTypeName attrType;
    if (attrObj.hasFn(MFn::kNumericAttribute)) {
        MFnNumericAttribute attrNumericFn(attrObj);
        switch (attrNumericFn.unitType()) 
        {
        case MFnNumericData::kBoolean:
            attrType = SdfValueTypeNames->Bool;
            break;
        case MFnNumericData::kByte:
        case MFnNumericData::kChar:
        case MFnNumericData::kShort:
        case MFnNumericData::kInt:
            attrType = SdfValueTypeNames->Int;
            break;
/* We could do these, but there doesn't seem to be a good way of extracting
   the data later on.  For a Maya expert to solve...
        case MFnNumericData::kLong:
            attrType = SdfValueTypeNames->Int64;
            break;
        case MFnNumericData::kAddr:
            attrType = SdfValueTypeNames->UInt64;
            break;
*/
        case MFnNumericData::kFloat:
            attrType = SdfValueTypeNames->Float;
            break;
        case MFnNumericData::kDouble:
            attrType = SdfValueTypeNames->Double;
            break;
        case MFnNumericData::k2Short:
        case MFnNumericData::k2Int:
        //case MFnNumericData::k2Long:
            attrType = SdfValueTypeNames->Int2;
            break;
        case MFnNumericData::k3Short:
        case MFnNumericData::k3Int:
        //case MFnNumericData::k3Long:
            attrType = SdfValueTypeNames->Int3;
            break;
        case MFnNumericData::k2Float:
            attrType = SdfValueTypeNames->Float2;
            break;
        case MFnNumericData::k3Float:
            if (MFnAttribute(attrObj).isUsedAsColor()) {
                attrType = SdfValueTypeNames->Color3f;
            }
            else {
                attrType = SdfValueTypeNames->Float3;
            }
            break;
        case MFnNumericData::k2Double:
            attrType = SdfValueTypeNames->Double2;
            break;
        case MFnNumericData::k3Double:
            if (MFnAttribute(attrObj).isUsedAsColor()) {
                attrType = SdfValueTypeNames->Color3d;
            }
            else {
                attrType = SdfValueTypeNames->Double3;
            }
            break;
        case MFnNumericData::k4Double:
            attrType = SdfValueTypeNames->Double4;
            break;
        default:
            break;
        }
    }
    else if (attrObj.hasFn(MFn::kTypedAttribute)) {
        MFnTypedAttribute attrTypedFn(attrObj);
        switch (attrTypedFn.attrType()) 
        {
        case MFnData::kString:
            attrType = SdfValueTypeNames->String;
            break;
        case MFnData::kMatrix:
            attrType = SdfValueTypeNames->Matrix4d;
            break;
        case MFnData::kStringArray:
            attrType = SdfValueTypeNames->StringArray;
            break;
        case MFnData::kIntArray:
            attrType = SdfValueTypeNames->IntArray;
            break;
        case MFnData::kFloatArray:
            attrType = SdfValueTypeNames->FloatArray;
            break;
        case MFnData::kDoubleArray:
            attrType = SdfValueTypeNames->DoubleArray;
            break;
        case MFnData::kVectorArray:
            attrType = SdfValueTypeNames->Vector3dArray;
            break;
        case MFnData::kPointArray:
            attrType = SdfValueTypeNames->Point3dArray;
            break;
        default:
            break;
        }
    }
    else if (attrObj.hasFn(MFn::kUnitAttribute)) {
        //MFnUnitAttribute attrUnitFn(attrObj);
    }
    else if (attrObj.hasFn(MFn::kEnumAttribute)) {
        attrType = SdfValueTypeNames->Token;
    }

    return attrType;
}

/* static */
UsdAttribute
PxrUsdMayaWriteUtil::GetOrCreateUsdAttr(
        const MPlug& attrPlug,
        const UsdPrim& usdPrim,
        const std::string& attrName,
        const bool custom)
{
    UsdAttribute usdAttr;

    if (not usdPrim) {
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

    const SdfValueTypeName& typeName = PxrUsdMayaWriteUtil::GetUsdTypeName(attrPlug);
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
        const bool custom)
{
    UsdGeomPrimvar primvar;

    if (not imageable) {
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

    const SdfValueTypeName& typeName = PxrUsdMayaWriteUtil::GetUsdTypeName(attrPlug);
    if (typeName) {
        primvar = imageable.CreatePrimvar(primvarNameToken,
                                          typeName,
                                          interpolation,
                                          elementSize,
                                          custom);
    }

    return primvar;
}

/* static */
UsdAttribute PxrUsdMayaWriteUtil::GetOrCreateUsdRiAttribute(
        const MPlug& attrPlug,
        const UsdPrim& usdPrim,
        const std::string& attrName,
        const std::string& nameSpace)
{
    UsdAttribute usdAttr;

    if (not usdPrim) {
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
    if (not riStatements) {
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

    const SdfValueTypeName& typeName = PxrUsdMayaWriteUtil::GetUsdTypeName(attrPlug);
    if (typeName) {
        usdAttr = riStatements.CreateRiAttribute(riAttrNameToken,
                                                 typeName.GetType(),
                                                 nameSpace);
    }

    return usdAttr;
}

template <typename T>
void
_SetVec(
        const UsdAttribute& attr,
        const T& val,
        const UsdTimeCode& time) {
    attr.Set((attr.GetRoleName() == SdfValueRoleNames->Color) 
            ? GfConvertDisplayToLinear(val) 
            : val,

            time);
}

bool
PxrUsdMayaWriteUtil::SetUsdAttr(
        const MPlug &plg, 
        const UsdAttribute& usdAttr, 
        const UsdTimeCode &usdTime)
{
    MStatus status;
    if (!usdAttr || plg.isNull() ) {
        return false;
    }

    bool isAnimated = plg.isDestination();
    if (usdTime.IsDefault() == isAnimated ) {
        return true;
    }

    // Set UsdAttr
    MObject attrObj = plg.attribute();
    if (attrObj.hasFn(MFn::kNumericAttribute)) {
        MFnNumericAttribute attrNumericFn(attrObj);
        switch (attrNumericFn.unitType()) 
        {
        case MFnNumericData::kBoolean:
            usdAttr.Set(plg.asBool(), usdTime);
            break;
        case MFnNumericData::kByte:
        case MFnNumericData::kChar:
            usdAttr.Set((int)plg.asChar(), usdTime);
            break;
        case MFnNumericData::kShort:
            usdAttr.Set(int(plg.asShort()), usdTime);
            break;
        case MFnNumericData::kInt:
            usdAttr.Set(int(plg.asInt()), usdTime);
            break;
        //case MFnNumericData::kLong:
        //case MFnNumericData::kAddr:
        //    usdAttr.Set(plg.asInt(), usdTime);
        //    break;
        case MFnNumericData::kFloat:
            usdAttr.Set(plg.asFloat(), usdTime);
            break;
        case MFnNumericData::kDouble:
            usdAttr.Set(plg.asDouble(), usdTime);
            break;
        case MFnNumericData::k2Short:
        {
            short tmp1, tmp2;
            MFnNumericData attrNumericDataFn(plg.asMObject());
            attrNumericDataFn.getData(tmp1, tmp2);
            usdAttr.Set(GfVec2i(tmp1, tmp2), usdTime);
            break;
        }
        case MFnNumericData::k2Int:
        {
            int tmp1, tmp2;
            MFnNumericData attrNumericDataFn(plg.asMObject());
            attrNumericDataFn.getData(tmp1, tmp2);
            usdAttr.Set(GfVec2i(tmp1, tmp2), usdTime);
            break;
        }
        //case MFnNumericData::k2Long:
        case MFnNumericData::k3Short:
        {
            short tmp1, tmp2, tmp3;
            MFnNumericData attrNumericDataFn(plg.asMObject());
            attrNumericDataFn.getData(tmp1, tmp2, tmp3);
            usdAttr.Set(GfVec3i(tmp1, tmp2, tmp3), usdTime);
            break;
        }
        case MFnNumericData::k3Int:
        {
            int tmp1, tmp2, tmp3;
            MFnNumericData attrNumericDataFn(plg.asMObject());
            attrNumericDataFn.getData(tmp1, tmp2, tmp3);
            usdAttr.Set(GfVec3i(tmp1, tmp2, tmp3), usdTime);
            break;
        }
        //case MFnNumericData::k3Long:
        case MFnNumericData::k2Float:
        {
            float tmp1, tmp2;
            MFnNumericData attrNumericDataFn(plg.asMObject());
            attrNumericDataFn.getData(tmp1, tmp2);
            usdAttr.Set(GfVec2f(tmp1, tmp2), usdTime);
            break;
        }
        case MFnNumericData::k3Float:
        {
            float tmp1, tmp2, tmp3;
            MFnNumericData attrNumericDataFn(plg.asMObject());
            attrNumericDataFn.getData(tmp1, tmp2, tmp3);
            _SetVec(usdAttr, GfVec3f(tmp1, tmp2, tmp3), usdTime);
            break;
        }
        case MFnNumericData::k2Double:
        {
            double tmp1, tmp2;
            MFnNumericData attrNumericDataFn(plg.asMObject());
            attrNumericDataFn.getData(tmp1, tmp2);
            usdAttr.Set(GfVec2d(tmp1, tmp2), usdTime);
            break;
        }
        case MFnNumericData::k3Double:
        {
            double tmp1, tmp2, tmp3;
            MFnNumericData attrNumericDataFn(plg.asMObject());
            attrNumericDataFn.getData(tmp1, tmp2, tmp3);
            _SetVec(usdAttr, GfVec3d(tmp1, tmp2, tmp3), usdTime);
            break;
        }
        case MFnNumericData::k4Double:
        {
            double tmp1, tmp2, tmp3, tmp4;
            MFnNumericData attrNumericDataFn(plg.asMObject());
            attrNumericDataFn.getData(tmp1, tmp2, tmp3, tmp4);
            _SetVec(usdAttr, GfVec4d(tmp1, tmp2, tmp3, tmp4), usdTime);
            break;
        }
        default:
            return false;
        }
    }
    else if (attrObj.hasFn(MFn::kTypedAttribute)) {
        MFnTypedAttribute attrTypedFn(attrObj);
        switch (attrTypedFn.attrType()) 
        {
        case MFnData::kString:
            usdAttr.Set(std::string(plg.asString().asChar()), usdTime);
            break;
        case MFnData::kMatrix:
        {
            MFnMatrixData attrMatrixDataFn(plg.asMObject());
            MMatrix mat1 = attrMatrixDataFn.matrix();
            usdAttr.Set(GfMatrix4d(mat1.matrix), usdTime);
            break;
        }
        case MFnData::kStringArray:
        {
            MFnStringArrayData attrDataFn(plg.asMObject());
            VtArray<std::string> usdVal(attrDataFn.length());
            for (unsigned int i=0; i < attrDataFn.length(); i++) {
                usdVal[i] = std::string(attrDataFn[i].asChar());
            }
            usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kIntArray:
        {
            MFnIntArrayData attrDataFn(plg.asMObject());
            VtArray<int> usdVal(attrDataFn.length());
            for (unsigned int i=0; i < attrDataFn.length(); i++) {
                usdVal[i] = attrDataFn[i];
            }
            usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kFloatArray:
        {
            MFnFloatArrayData attrDataFn(plg.asMObject());
            VtArray<float> usdVal(attrDataFn.length());
            for (unsigned int i=0; i < attrDataFn.length(); i++) {
                usdVal[i] = attrDataFn[i];
            }
            usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kDoubleArray:
        {
            MFnDoubleArrayData attrDataFn(plg.asMObject());
            VtArray<double> usdVal(attrDataFn.length());
            for (unsigned int i=0; i < attrDataFn.length(); i++) {
                usdVal[i] = attrDataFn[i];
            }
            usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kVectorArray:
        {
            MFnVectorArrayData attrDataFn(plg.asMObject());
            VtArray<GfVec3d> usdVal(attrDataFn.length());
            for (unsigned int i=0; i < attrDataFn.length(); i++) {
                MVector tmpMayaVal = attrDataFn[i];
                usdVal[i] = GfVec3d(tmpMayaVal[0], tmpMayaVal[1], tmpMayaVal[2]);
            }
            usdAttr.Set(usdVal, usdTime);
            break;
        }
        case MFnData::kPointArray:
        {
            MFnPointArrayData attrDataFn(plg.asMObject());
            VtArray<GfVec4d> usdVal(attrDataFn.length());
            for (unsigned int i=0; i < attrDataFn.length(); i++) {
                MPoint tmpMayaVal = attrDataFn[i];
                usdVal[i] = GfVec4d(tmpMayaVal[0], tmpMayaVal[1], tmpMayaVal[2], tmpMayaVal[3]);
            }
            usdAttr.Set(usdVal, usdTime);
            break;
        }
        default:
            return false;
        }
    }
    else if (attrObj.hasFn(MFn::kUnitAttribute)) {
        //MFnUnitAttribute attrUnitFn(attrObj);
        return false;
    }
    else if (attrObj.hasFn(MFn::kEnumAttribute)) {
        MFnEnumAttribute attrEnumFn(attrObj);
        short enumIndex = plg.asShort();
        TfToken enumToken( std::string(attrEnumFn.fieldName(enumIndex, &status).asChar()) );
        usdAttr.Set(enumToken, usdTime);
        return false;
    }

    return true;
}

static
std::string
_GetExportAttributeMetadata(
        const JsObject& attrMetadata,
        const TfToken& keyToken)
{
    std::string value;
    JsObject::const_iterator attrMetadataIter = attrMetadata.find(keyToken);
    if (attrMetadataIter != attrMetadata.end()) {
        value = attrMetadataIter->second.GetString();
    }

    return value;
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
    MStatus status;
    MFnDependencyNode depFn(dagPath.node());

    MPlug exportedAttrsJsonPlug = depFn.findPlug(
        _tokens->USD_UserExportedAttributesJson.GetText(), true, &status);
    if (status != MS::kSuccess || exportedAttrsJsonPlug.isNull()) {
        // No attributes specified for export on this node.
        return false;
    }

    std::string exportedAttrsJsonString(exportedAttrsJsonPlug.asString().asChar());
    if (exportedAttrsJsonString.empty()) {
        return false;
    }

    JsParseError jsError;
    JsValue jsValue = JsParseString(exportedAttrsJsonString, &jsError);
    if (not jsValue) {
        MString errorMsg(TfStringPrintf(
            "Failed to parse USD exported attributes JSON on node at dagPath '%s'"
            " at line %d, column %d: %s",
            dagPath.fullPathName().asChar(),
            jsError.line, jsError.column, jsError.reason.c_str()).c_str());
        MGlobal::displayError(errorMsg);
        return false;
    }

    // Maintain a set of USD attribute names that have been processed. If an
    // attribute is multiply-defined, we'll use the first tag encountered and
    // issue warnings for the subsequent definitions. JsObject is really just a
    // std::map, so we'll be considering attributes in sorted order.
    std::set<std::string> exportedUsdAttrNames;

    const JsObject& exportedAttrs = jsValue.GetJsObject();
    for (JsObject::const_iterator iter = exportedAttrs.begin();
         iter != exportedAttrs.end();
         ++iter) {
        const std::string mayaAttrName = iter->first;

        const MPlug attrPlug = depFn.findPlug(mayaAttrName.c_str(), true, &status);
        if (status != MS::kSuccess || attrPlug.isNull()) {
            MString errorMsg(TfStringPrintf(
                "Could not find attribute '%s' for USD export on node at dagPath '%s'",
                mayaAttrName.c_str(), dagPath.fullPathName().asChar()).c_str());
            MGlobal::displayError(errorMsg);
            continue;
        }

        const JsObject& attrMetadata = iter->second.GetJsObject();

        // Check if this is a particular type of attribute (e.g. primvar or
        // usdRi attribute). If we don't recognize the type specified, we'll
        // fall back to a regular USD attribute.
        TfToken usdAttrType(
            _GetExportAttributeMetadata(attrMetadata, _tokens->usdAttrType));

        // Check whether an interpolation type was specified. This is only
        // relevant for primvars.
        TfToken interpolation(
            _GetExportAttributeMetadata(attrMetadata,
                                        UsdGeomTokens->interpolation));

        // Check whether the USD attribute name should be different than the
        // Maya attribute name.
        std::string usdAttrName =
            _GetExportAttributeMetadata(attrMetadata, _tokens->usdAttrName);
        if (usdAttrName.empty()) {
            if (usdAttrType == _tokens->USDAttrTypePrimvar or
                    usdAttrType == _tokens->USDAttrTypeUsdRi) {
                // Primvars and UsdRi attributes will be given a type-specific
                // namespace, so just use the Maya attribute name.
                usdAttrName = mayaAttrName;
            } else {
                // For regular USD attributes, when no name was specified we
                // prepend the userProperties namespace to the Maya attribute
                // name to get the USD attribute name.
                usdAttrName = _tokens->UserPropertiesNamespace.GetString() +
                              mayaAttrName;
            }
        }

        const auto& insertIter = exportedUsdAttrNames.insert(usdAttrName);

        if (not insertIter.second) {
            MString errorMsg(TfStringPrintf(
                "Ignoring duplicate USD export tag for attribute '%s' on node at dagPath '%s'",
                usdAttrName.c_str(), dagPath.fullPathName().asChar()).c_str());
            MGlobal::displayError(errorMsg);
            continue;
        }

        UsdAttribute usdAttr;

        if (usdAttrType == _tokens->USDAttrTypePrimvar) {
            UsdGeomImageable imageable(usdPrim);
            if (not imageable) {
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
                                                        true);
            if (primvar) {
                usdAttr = primvar.GetAttr();
            }
        } else if (usdAttrType == _tokens->USDAttrTypeUsdRi) {
            usdAttr =
                PxrUsdMayaWriteUtil::GetOrCreateUsdRiAttribute(attrPlug,
                                                               usdPrim,
                                                               usdAttrName);
        } else {
            usdAttr = PxrUsdMayaWriteUtil::GetOrCreateUsdAttr(attrPlug,
                                                              usdPrim,
                                                              usdAttrName,
                                                              true);
        }

        if (usdAttr) {
            if (not PxrUsdMayaWriteUtil::SetUsdAttr(attrPlug, usdAttr, usdTime)) {
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
		for (size_t i = 0; i < numValues; i++) {
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
		for (size_t i = 0; i < numValues; i++) {
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
                for (size_t i = 0; i < numValues; i++) {
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



