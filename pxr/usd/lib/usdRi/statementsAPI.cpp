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
#include "pxr/usd/usdRi/statementsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiStatementsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (StatementsAPI)
);

/* virtual */
UsdRiStatementsAPI::~UsdRiStatementsAPI()
{
}

/* static */
UsdRiStatementsAPI
UsdRiStatementsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiStatementsAPI();
    }
    return UsdRiStatementsAPI(stage->GetPrimAtPath(path));
}


/* static */
UsdRiStatementsAPI
UsdRiStatementsAPI::Apply(const UsdPrim &prim)
{
    return UsdAPISchemaBase::_ApplyAPISchema<UsdRiStatementsAPI>(
            prim, _schemaTokens->StatementsAPI);
}

/* static */
const TfType &
UsdRiStatementsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiStatementsAPI>();
    return tfType;
}

/* static */
bool 
UsdRiStatementsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiStatementsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdRiStatementsAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "typeUtils.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/envSetting.h"
#include <string>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    USDRI_STATEMENTS_WRITE_NEW_ATTR_ENCODING, false,
    "If off, UsdRiStatementsAPI will write old-style attributes.  "
    "Otherwise, primvars in the ri: namespace will be written instead.");

TF_DEFINE_ENV_SETTING(
    USDRI_STATEMENTS_READ_OLD_ATTR_ENCODING, true,
    "If on, UsdRiStatementsAPI will read old-style attributes.  "
    "Otherwise, primvars in the ri: namespace will be read instead.");

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((fullAttributeNamespace, "ri:attributes:"))
    ((primvarAttrNamespace, "primvars:ri:attributes:"))
    ((rootNamespace, "ri"))
    ((attributeNamespace, "attributes"))
    ((coordsys, "ri:coordinateSystem"))
    ((scopedCoordsys, "ri:scopedCoordinateSystem"))
    ((modelCoordsys, "ri:modelCoordinateSystems"))
    ((modelScopedCoordsys, "ri:modelScopedCoordinateSystems"))
);

static TfToken
_MakeRiAttrNamespace(const string &nameSpace, const string &attrName)
{
    return TfToken(_tokens->fullAttributeNamespace.GetString() + 
                   nameSpace + ":" + attrName);
}

UsdAttribute
UsdRiStatementsAPI::CreateRiAttribute(
    const TfToken& name, 
    const string &riType,
    const string &nameSpace)
{
    TfToken fullName = _MakeRiAttrNamespace(nameSpace, name.GetString());
    SdfValueTypeName usdType = UsdRi_GetUsdType(riType);
    if (TfGetEnvSetting(USDRI_STATEMENTS_WRITE_NEW_ATTR_ENCODING)) {
        return UsdGeomPrimvarsAPI(GetPrim())
            .CreatePrimvar(fullName, usdType).GetAttr();
    } else {
        UsdAttribute attr = GetPrim().CreateAttribute(fullName, usdType, 
                                                      /* custom = */ false);
        if (!TF_VERIFY(attr)) {
            return UsdAttribute();
        }
        return attr;
    }
}

UsdAttribute
UsdRiStatementsAPI::CreateRiAttribute(
    const TfToken &name, 
    const TfType &tfType,
    const string &nameSpace)
{
    TfToken fullName = _MakeRiAttrNamespace(nameSpace, name.GetString());
    SdfValueTypeName usdType = SdfSchema::GetInstance().FindType(tfType);
    if (TfGetEnvSetting(USDRI_STATEMENTS_WRITE_NEW_ATTR_ENCODING)) {
        return UsdGeomPrimvarsAPI(GetPrim())
            .CreatePrimvar(fullName, usdType).GetAttr();
    } else {
        UsdAttribute attr = GetPrim().CreateAttribute(fullName, usdType,
                                                      /* custom = */ false);
        if (!TF_VERIFY(attr)) {
            return UsdAttribute();
        }
        return attr;
    }
}

UsdAttribute
UsdRiStatementsAPI::GetRiAttribute(
    const TfToken &name, 
    const std::string &nameSpace)
{
    TfToken fullName = _MakeRiAttrNamespace(nameSpace, name.GetString());
    if (UsdGeomPrimvar p = UsdGeomPrimvarsAPI(GetPrim()).GetPrimvar(fullName)) {
        return p;
    }
    if (TfGetEnvSetting(USDRI_STATEMENTS_READ_OLD_ATTR_ENCODING)) {
        return GetPrim().GetAttribute(fullName);
    }
    return UsdAttribute();
}

std::vector<UsdProperty>
UsdRiStatementsAPI::GetRiAttributes(
    const string &nameSpace) const
{
    std::vector<UsdProperty> validProps;

    // Read as primvars.
    std::string const& ns = _tokens->fullAttributeNamespace.GetString();
    for (UsdGeomPrimvar const& pv:
         UsdGeomPrimvarsAPI(GetPrim()).GetPrimvars()) {
        if (TfStringStartsWith(pv.GetPrimvarName().GetString(), ns)) {
            validProps.push_back(pv.GetAttr());
        }
    }

    // If none found yet, try to read as old-style regular attributes.
    // We do not support a mix of old- and new-style.
    if (validProps.empty() &&
        TfGetEnvSetting(USDRI_STATEMENTS_READ_OLD_ATTR_ENCODING)) {
        std::vector<UsdProperty> props = 
            GetPrim().GetPropertiesInNamespace(_tokens->fullAttributeNamespace);
        std::vector<string> names;
        bool requestedNameSpace = (nameSpace != "");
        TF_FOR_ALL(propItr, props){
            UsdProperty prop = *propItr;
            names = prop.SplitName();
            if (requestedNameSpace && names[2] != nameSpace) {
                // wrong namespace
                continue;
            }
            validProps.push_back(prop);
        }
    }

    return validProps;
}


bool
UsdRiStatementsAPI::_IsCompatible(const UsdPrim &prim) const
{
    // HasA schemas compatible with all types for now.
    return true;
}

TfToken UsdRiStatementsAPI::GetRiAttributeNameSpace(const UsdProperty &prop)
{
    const std::vector<string> names = prop.SplitName();
    // Parse primvar encoding.
    if (TfStringStartsWith(prop.GetName(), _tokens->primvarAttrNamespace)) {
        if (names.size() >= 5) {
            // Primvar with N custom namespaces:
            // "primvars:ri:attributes:$(NS_1):...:$(NS_N):$(NAME)"
            return TfToken(TfStringJoin(names.begin() + 3, names.end()-1, ":"));
        }
        return TfToken();
    }
    // Optionally parse old-style attribute encoding.
    if (TfStringStartsWith(prop.GetName(), _tokens->fullAttributeNamespace) &&
        TfGetEnvSetting(USDRI_STATEMENTS_READ_OLD_ATTR_ENCODING)) {
        if (names.size() >= 4) {
            // Old-style attribute with N custom namespaces:
            // "ri:attributes:$(NS_1):...:$(NS_N):$(NAME)"
            return TfToken(TfStringJoin(names.begin() + 2, names.end()-1, ":"));
        }
    }
    return TfToken();
}

bool
UsdRiStatementsAPI::IsRiAttribute(const UsdProperty &attr)
{
    // Accept primvar encoding.
    if (TfStringStartsWith(attr.GetName(), _tokens->primvarAttrNamespace)) {
        return true;
    }
    // Optionally accept old-style attribute encoding.
    if (TfStringStartsWith(attr.GetName(), _tokens->fullAttributeNamespace) &&
        TfGetEnvSetting(USDRI_STATEMENTS_READ_OLD_ATTR_ENCODING)) {
        return true;
    }
    return false;
}

std::string
UsdRiStatementsAPI::MakeRiAttributePropertyName(const std::string &attrName)
{
    std::vector<string> names = TfStringTokenize(attrName, ":");

    // If this is an already-encoded name, return it unchanged.
    if (TfGetEnvSetting(USDRI_STATEMENTS_WRITE_NEW_ATTR_ENCODING)) {
        if (names.size() == 5 &&
            TfStringStartsWith(attrName, _tokens->primvarAttrNamespace)) {
            return attrName;
        }
    }
    if (names.size() == 4 &&
        TfStringStartsWith(attrName, _tokens->fullAttributeNamespace)) {
        return attrName;
    }

    // Attempt to parse namespaces in different forms.
    if (names.size() == 1) {
        names = TfStringTokenize(attrName, ".");
    }
    if (names.size() == 1) {
        names = TfStringTokenize(attrName, "_");
    }

    // Fallback to user namespace if no other exists.
    if (names.size() == 1) {
        names.insert(names.begin(), "user");
    }

    TfToken prefix =
        TfGetEnvSetting(USDRI_STATEMENTS_WRITE_NEW_ATTR_ENCODING) ?
        _tokens->primvarAttrNamespace :
        _tokens->fullAttributeNamespace;

    string fullName = prefix.GetString() + 
        names[0] + ":" + ( names.size() > 2 ?
                           TfStringJoin(names.begin() + 1, names.end(), "_")
                           : names[1]);

    return SdfPath::IsValidNamespacedIdentifier(fullName) ? fullName : string();
}

void
UsdRiStatementsAPI::SetCoordinateSystem(const std::string &coordSysName)
{
    UsdAttribute attr = GetPrim().CreateAttribute(_tokens->coordsys, 
                                                  SdfValueTypeNames->String, 
                                                  /* custom = */ false);
    if (TF_VERIFY(attr)) {
        attr.Set(coordSysName);

        UsdPrim currPrim = GetPrim();
        while (currPrim && currPrim.GetPath() != SdfPath::AbsoluteRootPath()) {
            if (currPrim.IsModel() && !currPrim.IsGroup() &&
                currPrim.GetPath() != SdfPath::AbsoluteRootPath()) {
                UsdRelationship rel =
                    currPrim.CreateRelationship(_tokens->modelCoordsys,
                                                /* custom = */ false);
                if (TF_VERIFY(rel)) {
                    // Order should not matter, since these are a set,
                    // but historically we have appended these.
                    rel.AddTarget(GetPrim().GetPath());
                }
                break;
            }

            currPrim = currPrim.GetParent();
        }
    }
}

std::string
UsdRiStatementsAPI::GetCoordinateSystem() const
{
    std::string result;
    UsdAttribute attr = GetPrim().GetAttribute(_tokens->coordsys);
    if (attr) {
        attr.Get(&result);
    }
    return result;
}

bool
UsdRiStatementsAPI::HasCoordinateSystem() const
{
    std::string result;
    UsdAttribute attr = GetPrim().GetAttribute(_tokens->coordsys);
    if (attr) {
        return attr.Get(&result);
    }
    return false;
}

void
UsdRiStatementsAPI::SetScopedCoordinateSystem(const std::string &coordSysName)
{
    UsdAttribute attr = GetPrim().CreateAttribute(_tokens->scopedCoordsys, 
                                                  SdfValueTypeNames->String,
                                                  /* custom = */ false);
    if (TF_VERIFY(attr)) {
        attr.Set(coordSysName);

        UsdPrim currPrim = GetPrim();
        while (currPrim) {
            if (currPrim.IsModel() && !currPrim.IsGroup() &&
                currPrim.GetPath() != SdfPath::AbsoluteRootPath()) {
                UsdRelationship rel =
                    currPrim.CreateRelationship(_tokens->modelScopedCoordsys,
                                                /* custom = */ false);
                if (TF_VERIFY(rel)) {
                    rel.AddTarget(GetPrim().GetPath());
                }
                break;
            }

            currPrim = currPrim.GetParent();
        }
    }
}

std::string
UsdRiStatementsAPI::GetScopedCoordinateSystem() const
{
    std::string result;
    UsdAttribute attr = GetPrim().GetAttribute(_tokens->scopedCoordsys);
    if (attr) {
        attr.Get(&result);
    }
    return result;
}

bool
UsdRiStatementsAPI::HasScopedCoordinateSystem() const
{
    std::string result;
    UsdAttribute attr = GetPrim().GetAttribute(_tokens->scopedCoordsys);
    if (attr) {
        return attr.Get(&result);
    }
    return false;
}

bool
UsdRiStatementsAPI::GetModelCoordinateSystems(SdfPathVector *targets) const
{
    if (GetPrim().IsModel()) {
        UsdRelationship rel =
            GetPrim().GetRelationship(_tokens->modelCoordsys);
        return rel && rel.GetForwardedTargets(targets);
    }

    return true;
}

bool
UsdRiStatementsAPI::GetModelScopedCoordinateSystems(SdfPathVector *targets) const
{
    if (GetPrim().IsModel()) {
        UsdRelationship rel =
            GetPrim().GetRelationship(_tokens->modelScopedCoordsys);
        return rel && rel.GetForwardedTargets(targets);
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
