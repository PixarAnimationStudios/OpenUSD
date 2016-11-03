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
#include "usdMaya/UserTaggedAttribute.h"

#include "pxr/base/js/json.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>

#include <set>

TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaUserTaggedAttributeTokens,
    PXRUSDMAYA_ATTR_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (USD_UserExportedAttributesJson)
    (usdAttrName)
    (usdAttrType)
    ((UserPropertiesNamespace, "userProperties:"))
);

PxrUsdMayaUserTaggedAttribute::PxrUsdMayaUserTaggedAttribute(
        const MPlug plug,
        const std::string& name,
        const TfToken& type,
        const TfToken& interpolation)
        : _plug(plug), _name(name), _type(type), _interpolation(interpolation)
{
}

MPlug
PxrUsdMayaUserTaggedAttribute::GetMayaPlug() const
{
    return _plug;
}

std::string
PxrUsdMayaUserTaggedAttribute::GetMayaName() const
{
    MString name = _plug.partialName();
    return std::string(name.asChar());
}

std::string
PxrUsdMayaUserTaggedAttribute::GetUsdName() const
{
    return _name;
}

TfToken
PxrUsdMayaUserTaggedAttribute::GetUsdType() const
{
    return _type;
}

TfToken
PxrUsdMayaUserTaggedAttribute::GetUsdInterpolation() const
{
    return _interpolation;
}

static std::string
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

/* static */
std::vector<PxrUsdMayaUserTaggedAttribute>
PxrUsdMayaUserTaggedAttribute::GetUserTaggedAttributesForNode(
        const MDagPath& dagPath)
{
    MStatus status;
    MFnDependencyNode depFn(dagPath.node());
    std::vector<PxrUsdMayaUserTaggedAttribute> result;
    std::set<std::string> processedAttributeNames;

    MPlug exportedAttrsJsonPlug = depFn.findPlug(
        _tokens->USD_UserExportedAttributesJson.GetText(), true, &status);
    if (status != MS::kSuccess || exportedAttrsJsonPlug.isNull()) {
        // No attributes specified for export on this node.
        return result;
    }

    std::string exportedAttrsJsonString(
        exportedAttrsJsonPlug.asString().asChar());
    if (exportedAttrsJsonString.empty()) {
        return result;
    }

    JsParseError jsError;
    JsValue jsValue = JsParseString(exportedAttrsJsonString, &jsError);
    if (not jsValue) {
        MString errorMsg(TfStringPrintf(
            "Failed to parse USD exported attributes JSON on node at"
            " dagPath '%s' at line %d, column %d: %s",
            dagPath.fullPathName().asChar(),
            jsError.line, jsError.column, jsError.reason.c_str()).c_str());
        MGlobal::displayError(errorMsg);
        return result;
    }

    // If an attribute is multiply-defined, we'll use the first tag encountered
    // and issue warnings for the subsequent definitions. JsObject is really
    // just a std::map, so we'll be considering attributes in sorted order.
    const JsObject& exportedAttrs = jsValue.GetJsObject();
    for (JsObject::const_iterator iter = exportedAttrs.begin();
         iter != exportedAttrs.end();
         ++iter) {
        const std::string mayaAttrName = iter->first;

        const MPlug attrPlug = depFn.findPlug(
                mayaAttrName.c_str(), true, &status);
        if (status != MS::kSuccess || attrPlug.isNull()) {
            MString errorMsg(TfStringPrintf(
                "Could not find attribute '%s' for USD export on node at"
                " dagPath '%s'",
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
            const auto& tokens = PxrUsdMayaUserTaggedAttributeTokens;
            if (usdAttrType == tokens->USDAttrTypePrimvar or
                    usdAttrType == tokens->USDAttrTypeUsdRi) {
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

        const auto& insertIter = processedAttributeNames.emplace(usdAttrName);
        if (not insertIter.second) {
            MString errorMsg(TfStringPrintf(
                "Ignoring duplicate USD export tag for attribute '%s' on node"
                " at dagPath '%s'",
                usdAttrName.c_str(), dagPath.fullPathName().asChar()).c_str());
            MGlobal::displayError(errorMsg);
            continue;
        }

        result.emplace_back(attrPlug, usdAttrName, usdAttrType, interpolation);
    }

    return result;
}
