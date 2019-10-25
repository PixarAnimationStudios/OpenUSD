//
// Copyright 2018 Pixar
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
#include "usdMaya/readUtil.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/colorSpace.h"
#include "usdMaya/util.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/vt/array.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/usd/tokens.h"

#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnFloatArrayData.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnStringArrayData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MIntArray.h>
#include <maya/MMatrix.h>
#include <maya/MPointArray.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MVectorArray.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
        PIXMAYA_READ_FLOAT2_AS_UV, true,
        "Set to false to disable ability to read Float2 type as a UV set");

bool
UsdMayaReadUtil::ReadFloat2AsUV()
{
    static const bool readFloat2AsUV = 
        TfGetEnvSetting(PIXMAYA_READ_FLOAT2_AS_UV);
    return readFloat2AsUV;
}

MObject
UsdMayaReadUtil::FindOrCreateMayaAttr(
        const SdfValueTypeName& typeName,
        const SdfVariability variability,
        MFnDependencyNode& depNode,
        const std::string& attrName,
        const std::string& attrNiceName)
{
    MDGModifier modifier;
    return FindOrCreateMayaAttr(typeName, variability, depNode, attrName,
            attrNiceName, modifier);
}

// Whether a plug's attribute is a typed attribute with given type.
static bool
_HasAttrType(const MPlug& plug, MFnData::Type type)
{
    MObject object = plug.attribute();
    if (!object.hasFn(MFn::kTypedAttribute)) {
        return false;
    }

    MFnTypedAttribute attr(object);
    return attr.attrType() == type;
}

// Whether a plug's attribute is a numeric attribute with given type.
static bool
_HasNumericType(const MPlug& plug, MFnNumericData::Type type)
{
    MObject object = plug.attribute();
    if (!object.hasFn(MFn::kNumericAttribute)) {
        return false;
    }

    MFnNumericAttribute attr(object);
    return attr.unitType() == type;
}

static bool
_HasEnumType(const MPlug& plug)
{
    MObject object = plug.attribute();
    return object.hasFn(MFn::kEnumAttribute);
}

static MObject
_FindOrCreateMayaTypedAttr(
    const std::string& attrName,
    const std::string& attrNiceName,
    const MFnData::Type type,
    const bool keyable,
    const bool usedAsColor,
    const bool usedAsFilename,
    MFnDependencyNode& depNode,
    MDGModifier& modifier)
{
    MString mayaName = attrName.c_str();
    MString niceName =  attrNiceName.empty() ?
            attrName.c_str() : attrNiceName.c_str();

    MPlug plug = depNode.findPlug(mayaName, true);
    if (plug.isNull()) {
        // Create.
        MFnTypedAttribute attr;
        MObject attrObj = attr.create(mayaName, mayaName, type);
        attr.setNiceNameOverride(niceName);
        attr.setKeyable(keyable);

        if (usedAsColor) {
            attr.setUsedAsColor(true);
        }
        if (usedAsFilename) {
            attr.setUsedAsFilename(true);
        }

        modifier.addAttribute(depNode.object(), attrObj);
        modifier.doIt();
        return attrObj;
    }
    else {
        // Found -- verify.
        if (_HasAttrType(plug, type)) {
            return plug.attribute();
        }
        else {
            TF_RUNTIME_ERROR("Plug %s has unexpected type",
                    plug.name().asChar());
            return MObject();
        }
    }
}

static MObject
_FindOrCreateMayaNumericAttr(
    const std::string& attrName,
    const std::string& attrNiceName,
    const MFnNumericData::Type type,
    const bool keyable,
    const bool usedAsColor,
    MFnDependencyNode& depNode,
    MDGModifier& modifier)
{
    MString mayaName = attrName.c_str();
    MString niceName =  attrNiceName.empty() ?
            attrName.c_str() : attrNiceName.c_str();

    MPlug plug = depNode.findPlug(mayaName, true);
    if (plug.isNull()) {
        // Create.
        MFnNumericAttribute attr;
        MObject attrObj = attr.create(mayaName, mayaName, type);
        attr.setNiceNameOverride(niceName);
        attr.setKeyable(keyable);

        if (usedAsColor) {
            attr.setUsedAsColor(true);
        }

        modifier.addAttribute(depNode.object(), attrObj);
        modifier.doIt();
        return attrObj;
    }
    else {
        // Found -- verify.
        if (_HasNumericType(plug, type)
            || (type == MFnNumericData::kInt && _HasEnumType(plug)))
        {
            return plug.attribute();
        }
        else {
            TF_RUNTIME_ERROR("Plug %s has unexpected type",
                    plug.name().asChar());
            return MObject();
        }
    }
}

MObject
UsdMayaReadUtil::FindOrCreateMayaAttr(
        const SdfValueTypeName& typeName,
        const SdfVariability variability,
        MFnDependencyNode& depNode,
        const std::string& attrName,
        const std::string& attrNiceName,
        MDGModifier& modifier)
{
    return FindOrCreateMayaAttr(
            typeName.GetType(),
            typeName.GetRole(),
            variability,
            depNode,
            attrName,
            attrNiceName,
            modifier);
}

MObject
UsdMayaReadUtil::FindOrCreateMayaAttr(
        const TfType& type,
        const TfToken& role,
        const SdfVariability variability,
        MFnDependencyNode& depNode,
        const std::string& attrName,
        const std::string& attrNiceName,
        MDGModifier& modifier)
{
    MString mayaName = attrName.c_str();
    MString niceName =  attrNiceName.empty() ?
            attrName.c_str() : attrNiceName.c_str();

    // For the majority of things, we don't care about the role, just about
    // the type, e.g. we export point3f/vector3f/float3 the same.
    // (Though for stuff like colors, we'll disambiguate by role.)
    const bool keyable = variability == SdfVariabilityVarying;
    const bool usedAsColor = role == SdfValueRoleNames->Color;

    MObject attrObj;
    if (type.IsA<TfToken>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kString, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<std::string>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kString, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<SdfAssetPath>()) {
        // XXX We're not setting usedAsFilename right now because we can't
        // figure out how to opt-out of Maya's internal path resolution.
        // This is still OK when we round-trip because we'll still generate
        // SdfAssetPaths when we check the schema attribute's value type name.
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kString, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<GfMatrix4d>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kMatrix, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<SdfTokenListOp>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kStringArray, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<SdfStringListOp>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kStringArray, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<VtTokenArray>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kStringArray, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<VtStringArray>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kStringArray, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<VtDoubleArray>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kDoubleArray, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<VtFloatArray>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kFloatArray, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<VtIntArray>()) {
        return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                MFnData::kIntArray, keyable, usedAsColor,
                /*usedAsFilename*/ false, depNode, modifier);
    }
    else if (type.IsA<VtVec3dArray>() ||
            type.IsA<VtVec3fArray>()) {
        if (role == SdfValueRoleNames->Point) {
            return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                    MFnData::kPointArray, keyable, usedAsColor,
                    /*usedAsFilename*/ false, depNode, modifier);
        }
        else {
            return _FindOrCreateMayaTypedAttr(attrName, attrNiceName,
                    MFnData::kVectorArray, keyable, usedAsColor,
                    /*usedAsFilename*/ false, depNode, modifier);
        }
    }
    else if (type.IsA<bool>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::kBoolean, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<int>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::kInt, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<GfVec2i>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::k2Int, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<GfVec3i>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::k3Int, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<float>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::kFloat, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<GfVec2f>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::k2Float, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<GfVec3f>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::k3Float, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<double>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::kDouble, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<GfVec2d>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::k2Double, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<GfVec3d>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::k3Double, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<GfVec4d>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::k4Double, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<GfQuatf>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::k4Double, keyable, usedAsColor,
                depNode, modifier);
    }
    else if (type.IsA<GfQuatd>()) {
        return _FindOrCreateMayaNumericAttr(attrName, attrNiceName,
                MFnNumericData::k4Double, keyable, usedAsColor,
                depNode, modifier);
    }

    TF_RUNTIME_ERROR("Type '%s' isn't supported", type.GetTypeName().c_str());
    return MObject();
}

/// Converts a vec from linear to display color if its attribute is a color.
template <typename T>
T
_ConvertVec(
        const MPlug& plug,
        const T& val) {
    if (MFnAttribute(plug.attribute()).isUsedAsColor()) {
        return UsdMayaColorSpace::ConvertLinearToMaya(val);
    }
    else {
        return val;
    }
}

bool UsdMayaReadUtil::SetMayaAttr(
        MPlug& attrPlug,
        const UsdAttribute& usdAttr)
{
    VtValue val;
    if (usdAttr.Get(&val)) {
        if (SetMayaAttr(attrPlug, val)) {
            SetMayaAttrKeyableState(attrPlug, usdAttr.GetVariability());
            return true;
        }
    }

    return false;
}

bool UsdMayaReadUtil::SetMayaAttr(
        MPlug& attrPlug,
        const VtValue& newValue)
{
    MDGModifier modifier;
    return SetMayaAttr(attrPlug, newValue, modifier);
}

bool UsdMayaReadUtil::SetMayaAttr(
        MPlug& attrPlug,
        const VtValue& newValue,
        MDGModifier& modifier)
{
    bool ok = false;
    if (newValue.IsHolding<TfToken>()) {
        TfToken token = newValue.Get<TfToken>();
        if (_HasAttrType(attrPlug, MFnData::kString)) {
            modifier.newPlugValueString(attrPlug, token.GetText());
            ok = true;
        }
    }
    else if (newValue.IsHolding<std::string>()) {
        if (_HasAttrType(attrPlug, MFnData::kString)) {
            std::string str = newValue.Get<std::string>();
            modifier.newPlugValueString(attrPlug, str.c_str());
            ok = true;
        }
    }
    else if (newValue.IsHolding<SdfAssetPath>()) {
        if (_HasAttrType(attrPlug, MFnData::kString)) {
            SdfAssetPath assetPath = newValue.Get<SdfAssetPath>();
            modifier.newPlugValueString(attrPlug,
                    assetPath.GetAssetPath().c_str());
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfMatrix4d>()) {
        if (_HasAttrType(attrPlug, MFnData::kMatrix)) {
            GfMatrix4d mat = newValue.Get<GfMatrix4d>();
            MMatrix mayaMat;
            for (size_t i = 0; i < 4; ++i) {
                for (size_t j = 0; j < 4; ++j) {
                    mayaMat[i][j] = mat[i][j];
                }
            }
            MFnMatrixData data;
            MObject dataObj = data.create();
            data.set(mayaMat);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<SdfTokenListOp>()) {
        if (_HasAttrType(attrPlug, MFnData::kStringArray)) {
            TfTokenVector result;
            newValue.Get<SdfTokenListOp>().ApplyOperations(&result);
            MStringArray mayaArr;
            for (const TfToken& tok : result) {
                mayaArr.append(tok.GetText());
            }
            MFnStringArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<SdfStringListOp>()) {
        if (_HasAttrType(attrPlug, MFnData::kStringArray)) {
            std::vector<std::string> result;
            newValue.Get<SdfStringListOp>().ApplyOperations(&result);
            MStringArray mayaArr;
            for (const std::string& str : result) {
                mayaArr.append(str.c_str());
            }
            MFnStringArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<VtTokenArray>()) {
        if (_HasAttrType(attrPlug, MFnData::kStringArray)) {
            VtTokenArray arr = newValue.Get<VtTokenArray>();
            MStringArray mayaArr;
            for (const TfToken& tok : arr) {
                mayaArr.append(tok.GetText());
            }
            MFnStringArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<VtStringArray>()) {
        if (_HasAttrType(attrPlug, MFnData::kStringArray)) {
            VtStringArray arr = newValue.Get<VtStringArray>();
            MStringArray mayaArr;
            for (const std::string& str : arr) {
                mayaArr.append(str.c_str());
            }
            MFnStringArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<VtDoubleArray>()) {
        if (_HasAttrType(attrPlug, MFnData::kDoubleArray)) {
            VtDoubleArray arr = newValue.Get<VtDoubleArray>();
            MDoubleArray mayaArr(arr.data(), arr.size());
            MFnDoubleArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<VtFloatArray>()) {
        if (_HasAttrType(attrPlug, MFnData::kFloatArray)) {
            VtFloatArray arr = newValue.Get<VtFloatArray>();
            MFloatArray mayaArr(arr.data(), arr.size());
            MFnFloatArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<VtIntArray>()) {
        if (_HasAttrType(attrPlug, MFnData::kIntArray)) {
            VtIntArray arr = newValue.Get<VtIntArray>();
            MIntArray mayaArr(arr.data(), arr.size());
            MFnIntArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<VtVec3dArray>()) {
        if (_HasAttrType(attrPlug, MFnData::kVectorArray)) {
            VtVec3dArray arr = newValue.Get<VtVec3dArray>();
            MVectorArray mayaArr;
            for (const GfVec3d& v : arr) {
                mayaArr.append(MVector(v[0], v[1], v[2]));
            }
            MFnVectorArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
        else if (_HasAttrType(attrPlug, MFnData::kPointArray)) {
            VtVec3dArray arr = newValue.Get<VtVec3dArray>();
            MPointArray mayaArr;
            for (const GfVec3d& v : arr) {
                mayaArr.append(MPoint(v[0], v[1], v[2]));
            }
            MFnPointArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<VtVec3fArray>()) {
        if (_HasAttrType(attrPlug, MFnData::kVectorArray)) {
            VtVec3fArray arr = newValue.Get<VtVec3fArray>();
            MVectorArray mayaArr;
            for (const GfVec3d& v : arr) {
                mayaArr.append(MVector(v[0], v[1], v[2]));
            }
            MFnVectorArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
        else if (_HasAttrType(attrPlug, MFnData::kPointArray)) {
            VtVec3fArray arr = newValue.Get<VtVec3fArray>();
            MPointArray mayaArr;
            for (const GfVec3d& v : arr) {
                mayaArr.append(MPoint(v[0], v[1], v[2]));
            }
            MFnPointArrayData data;
            MObject dataObj = data.create();
            data.set(mayaArr);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<bool>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::kBoolean)) {
            bool b = newValue.Get<bool>();
            modifier.newPlugValueBool(attrPlug, b);
            ok = true;
        }
    }
    else if (newValue.IsHolding<int>() ||
            newValue.IsHolding<float>() ||
            newValue.IsHolding<double>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::kInt)) {
            int i = VtValue::Cast<int>(newValue).Get<int>();
            modifier.newPlugValueInt(attrPlug, i);
            ok = true;
        }
        else if (_HasNumericType(attrPlug, MFnNumericData::kFloat)) {
            float f = VtValue::Cast<float>(newValue).Get<float>();
            modifier.newPlugValueFloat(attrPlug, f);
            ok = true;
        }
        else if (_HasNumericType(attrPlug, MFnNumericData::kDouble)) {
            double d = VtValue::Cast<double>(newValue).Get<double>();
            modifier.newPlugValueDouble(attrPlug, d);
            ok = true;
        } else if (newValue.IsHolding<int>() && _HasEnumType(attrPlug)) {
            int i = VtValue::Cast<int>(newValue).Get<int>();
            modifier.newPlugValueInt(attrPlug, i);
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfVec2i>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::k2Int)) {
            GfVec2i v = newValue.Get<GfVec2i>();
            MFnNumericData data;
            MObject dataObj = data.create(MFnNumericData::k2Int);
            data.setData2Int(v[0], v[1]);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfVec3i>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::k2Int)) {
            GfVec3i v = newValue.Get<GfVec3i>();
            MFnNumericData data;
            MObject dataObj = data.create(MFnNumericData::k3Int);
            data.setData3Int(v[0], v[1], v[2]);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfVec2f>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::k2Float)) {
            GfVec2f v = newValue.Get<GfVec2f>();
            MFnNumericData data;
            MObject dataObj = data.create(MFnNumericData::k2Float);
            data.setData2Float(v[0], v[1]);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfVec3f>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::k3Float)) {
            GfVec3f v = _ConvertVec(attrPlug, newValue.Get<GfVec3f>());
            MFnNumericData data;
            MObject dataObj = data.create(MFnNumericData::k3Float);
            data.setData3Float(v[0], v[1], v[2]);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfVec2d>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::k2Double)) {
            GfVec2d v = newValue.Get<GfVec2d>();
            MFnNumericData data;
            MObject dataObj = data.create(MFnNumericData::k2Double);
            data.setData2Double(v[0], v[1]);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfVec3d>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::k3Double)) {
            GfVec3d v = _ConvertVec(attrPlug, newValue.Get<GfVec3d>());
            MFnNumericData data;
            MObject dataObj = data.create(MFnNumericData::k3Double);
            data.setData3Double(v[0], v[1], v[2]);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfVec4d>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::k4Double)) {
            GfVec4d v = _ConvertVec(attrPlug, newValue.Get<GfVec4d>());
            MFnNumericData data;
            MObject dataObj = data.create(MFnNumericData::k4Double);
            data.setData4Double(v[0], v[1], v[2], v[3]);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfQuatf>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::k4Double)) {
            GfQuatf q = newValue.Get<GfQuatf>();
            GfVec3f im = q.GetImaginary();
            MFnNumericData data;
            MObject dataObj = data.create(MFnNumericData::k4Double);
            data.setData4Double(q.GetReal(), im[0], im[1], im[2]);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }
    else if (newValue.IsHolding<GfQuatd>()) {
        if (_HasNumericType(attrPlug, MFnNumericData::k4Double)) {
            GfQuatd q = newValue.Get<GfQuatd>();
            GfVec3d im = q.GetImaginary();
            MFnNumericData data;
            MObject dataObj = data.create(MFnNumericData::k4Double);
            data.setData4Double(q.GetReal(), im[0], im[1], im[2]);
            modifier.newPlugValue(attrPlug, dataObj);
            ok = true;
        }
    }

    if (ok) {
        modifier.doIt();
    } else {
        TF_RUNTIME_ERROR(
                "Cannot set value of type '%s' on plug '%s'",
                newValue.GetTypeName().c_str(),
                attrPlug.name().asChar());
    }
    return ok;
}

void
UsdMayaReadUtil::SetMayaAttrKeyableState(
        MPlug& attrPlug,
        const SdfVariability variability)
{
    MDGModifier modifier;
    SetMayaAttrKeyableState(attrPlug, variability, modifier);
}

void
UsdMayaReadUtil::SetMayaAttrKeyableState(
        MPlug& attrPlug,
        const SdfVariability variability,
        MDGModifier& modifier)
{
    modifier.commandToExecute(TfStringPrintf("setAttr -keyable %d %s",
            variability == SdfVariabilityVarying ? 1 : 0,
            attrPlug.name().asChar()).c_str());
    modifier.doIt();
}

bool
UsdMayaReadUtil::ReadMetadataFromPrim(
    const TfToken::Set& includeMetadataKeys,
    const UsdPrim& prim,
    const MObject& mayaObject)
{
    UsdMayaAdaptor adaptor(mayaObject);
    if (!adaptor) {
        return false;
    }

    for (const TfToken& key : includeMetadataKeys) {
        if (key == UsdTokens->apiSchemas) {
            // Never import apiSchemas from metadata. It has a meaning in the
            // UsdMayaAdaptor system, so it should only be added to the
            // Maya node by UsdMayaAdaptor::ApplySchema().
            continue;
        }
        if (!prim.HasAuthoredMetadata(key)) {
            continue;
        }
        VtValue value;
        prim.GetMetadata(key, &value);
        adaptor.SetMetadata(key, value);
    }
    return true;
}

bool
UsdMayaReadUtil::ReadAPISchemaAttributesFromPrim(
    const TfToken::Set& includeAPINames,
    const UsdPrim& prim,
    const MObject& mayaObject)
{
    UsdMayaAdaptor adaptor(mayaObject);
    if (!adaptor) {
        return false;
    }

    for (const TfToken& schemaName : prim.GetAppliedSchemas()) {
        if (includeAPINames.count(schemaName) == 0) {
            continue;
        }
        if (UsdMayaAdaptor::SchemaAdaptor schemaAdaptor =
                adaptor.ApplySchemaByName(schemaName)) {
            for (const TfToken& attrName : schemaAdaptor.GetAttributeNames()) {
                if (UsdAttribute attr = prim.GetAttribute(attrName)) {
                    VtValue value;
                    constexpr UsdTimeCode t = UsdTimeCode::EarliestTime();
                    if (attr.HasAuthoredValue() && attr.Get(&value, t)) {
                        schemaAdaptor.CreateAttribute(attrName).Set(value);
                    }
                }
            }
        }
    }
    return true;
}

/* static */
size_t
UsdMayaReadUtil::ReadSchemaAttributesFromPrim(
    const UsdPrim& prim,
    const MObject& mayaObject,
    const TfType& schemaType,
    const std::vector<TfToken>& attributeNames,
    const UsdTimeCode& usdTime)
{
    UsdMayaAdaptor adaptor(mayaObject);
    if (!adaptor) {
        return 0;
    }

    size_t count = 0;
    if (UsdMayaAdaptor::SchemaAdaptor schemaAdaptor =
            adaptor.GetSchemaOrInheritedSchema(schemaType)) {
        for (const TfToken& attrName : attributeNames) {
            if (UsdAttribute attr = prim.GetAttribute(attrName)) {
                VtValue value;
                if (attr.HasAuthoredValue() && attr.Get(&value, usdTime)) {
                    if (schemaAdaptor.CreateAttribute(attrName).Set(value)) {
                        count++;
                    }
                }
            }
        }
    }

    return count;
}

PXR_NAMESPACE_CLOSE_SCOPE
