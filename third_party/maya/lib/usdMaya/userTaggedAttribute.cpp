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
#include "usdMaya/userTaggedAttribute.h"

#include "usdMaya/util.h"

#include "pxr/base/js/json.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <set>
#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PUBLIC_TOKENS(
    UsdMayaUserTaggedAttributeTokens,
    PXRUSDMAYA_ATTR_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (USD_UserExportedAttributesJson)
    (usdAttrName)
    (usdAttrType)
    (translateMayaDoubleToUsdSinglePrecision)
    ((UserPropertiesNamespace, "userProperties:"))
);


UsdMayaUserTaggedAttribute::UsdMayaUserTaggedAttribute(
        const MPlug& plug,
        const std::string& name,
        const TfToken& type,
        const TfToken& interpolation,
        const bool translateMayaDoubleToUsdSinglePrecision)
        : _plug(plug),
          _name(name),
          _type(type),
          _interpolation(interpolation),
          _translateMayaDoubleToUsdSinglePrecision(
              translateMayaDoubleToUsdSinglePrecision)
{
}

MPlug
UsdMayaUserTaggedAttribute::GetMayaPlug() const
{
    return _plug;
}

std::string
UsdMayaUserTaggedAttribute::GetMayaName() const
{
    MString name = _plug.partialName();
    return std::string(name.asChar());
}

std::string
UsdMayaUserTaggedAttribute::GetUsdName() const
{
    return _name;
}

TfToken
UsdMayaUserTaggedAttribute::GetUsdType() const
{
    return _type;
}

TfToken
UsdMayaUserTaggedAttribute::GetUsdInterpolation() const
{
    return _interpolation;
}

bool
UsdMayaUserTaggedAttribute::GetTranslateMayaDoubleToUsdSinglePrecision() const
{
    return _translateMayaDoubleToUsdSinglePrecision;
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

static
bool
_GetExportAttributeMetadata(
        const JsObject& attrMetadata,
        const TfToken& keyToken,
        const bool defaultValue)
{
    bool value = defaultValue;
    JsObject::const_iterator attrMetadataIter = attrMetadata.find(keyToken);
    if (attrMetadataIter != attrMetadata.end()) {
        value = attrMetadataIter->second.GetBool();
    }

    return value;
}

/* static */
std::vector<UsdMayaUserTaggedAttribute>
UsdMayaUserTaggedAttribute::GetUserTaggedAttributesForNode(
        const MObject& mayaNode)
{
    std::vector<UsdMayaUserTaggedAttribute> result;

    MStatus status;
    const MFnDependencyNode depNodeFn(mayaNode, &status);
    if (status != MS::kSuccess) {
        return result;
    }

    const MPlug exportedAttrsJsonPlug =
        depNodeFn.findPlug(
            _tokens->USD_UserExportedAttributesJson.GetText(),
            true,
            &status);
    if (status != MS::kSuccess || exportedAttrsJsonPlug.isNull()) {
        // No attributes specified for export on this node.
        return result;
    }

    const std::string exportedAttrsJsonString(
        exportedAttrsJsonPlug.asString().asChar());
    if (exportedAttrsJsonString.empty()) {
        return result;
    }

    JsParseError jsError;
    const JsValue jsValue = JsParseString(exportedAttrsJsonString, &jsError);
    if (!jsValue) {
        TF_RUNTIME_ERROR(
            "Failed to parse USD exported attributes JSON on node '%s' "
            "at line %d, column %d: %s",
            UsdMayaUtil::GetMayaNodeName(mayaNode).c_str(),
            jsError.line, jsError.column, jsError.reason.c_str());
        return result;
    }

    // If an attribute is multiply-defined, we'll use the first tag encountered
    // and issue warnings for the subsequent definitions. JsObject is really
    // just a std::map, so we'll be considering attributes in sorted order.
    std::set<std::string> processedAttributeNames;
    const JsObject& exportedAttrs = jsValue.GetJsObject();
    for (const auto& exportedAttr : exportedAttrs) {
        const std::string mayaAttrName = exportedAttr.first;

        const MPlug attrPlug =
            depNodeFn.findPlug(mayaAttrName.c_str(), true, &status);
        if (status != MS::kSuccess || attrPlug.isNull()) {
            TF_RUNTIME_ERROR(
                "Could not find attribute '%s' for USD export on node '%s'",
                mayaAttrName.c_str(),
                UsdMayaUtil::GetMayaNodeName(mayaNode).c_str());
            continue;
        }

        const JsObject& attrMetadata = exportedAttr.second.GetJsObject();

        // Check if this is a particular type of attribute (e.g. primvar or
        // usdRi attribute). If we don't recognize the type specified, we'll
        // fall back to a regular USD attribute.
        const TfToken usdAttrType(
            _GetExportAttributeMetadata(attrMetadata, _tokens->usdAttrType));

        // Check whether an interpolation type was specified. This is only
        // relevant for primvars.
        const TfToken interpolation(
            _GetExportAttributeMetadata(attrMetadata,
                                        UsdGeomTokens->interpolation));

        // Check whether it was specified that the double precision Maya
        // attribute type should be mapped to a single precision USD type.
        // If it wasn't specified, use the fallback value.
        const bool translateMayaDoubleToUsdSinglePrecision(
            _GetExportAttributeMetadata(
                attrMetadata,
                _tokens->translateMayaDoubleToUsdSinglePrecision,
                GetFallbackTranslateMayaDoubleToUsdSinglePrecision()));

        // Check whether the USD attribute name should be different than the
        // Maya attribute name.
        std::string usdAttrName =
            _GetExportAttributeMetadata(attrMetadata, _tokens->usdAttrName);
        if (usdAttrName.empty()) {
            const auto& tokens = UsdMayaUserTaggedAttributeTokens;
            if (usdAttrType == tokens->USDAttrTypePrimvar ||
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
        if (!insertIter.second) {
            TF_RUNTIME_ERROR(
                "Ignoring duplicate USD export tag for attribute '%s' "
                "on node '%s'",
                usdAttrName.c_str(),
                UsdMayaUtil::GetMayaNodeName(mayaNode).c_str());
            continue;
        }

        result.emplace_back(attrPlug,
                            usdAttrName,
                            usdAttrType,
                            interpolation,
                            translateMayaDoubleToUsdSinglePrecision);
    }

    return result;
}


PXR_NAMESPACE_CLOSE_SCOPE
