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
#include "pxr/usd/usdRi/statements.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiStatements,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdRiStatements::~UsdRiStatements()
{
}

/* static */
UsdRiStatements
UsdRiStatements::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiStatements();
    }
    return UsdRiStatements(stage->GetPrimAtPath(path));
}


/* static */
UsdRiStatements
UsdRiStatements::Apply(const UsdStagePtr &stage, const SdfPath &path)
{
    // Ensure we have a valid stage, path and prim
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiStatements();
    }

    if (path == SdfPath::AbsoluteRootPath()) {
        TF_CODING_ERROR("Cannot apply an api schema on the pseudoroot");
        return UsdRiStatements();
    }

    auto prim = stage->GetPrimAtPath(path);
    if (!prim) {
        TF_CODING_ERROR("Prim at <%s> does not exist.", path.GetText());
        return UsdRiStatements();
    }

    TfToken apiName("Statements");  

    // Get the current listop at the edit target
    UsdEditTarget editTarget = stage->GetEditTarget();
    SdfPrimSpecHandle primSpec = editTarget.GetPrimSpecForScenePath(path);
    SdfTokenListOp listOp = primSpec->GetInfo(UsdTokens->apiSchemas)
                                    .UncheckedGet<SdfTokenListOp>();

    // Append our name to the prepend list, if it doesnt exist locally
    TfTokenVector prepends = listOp.GetPrependedItems();
    if (std::find(prepends.begin(), prepends.end(), apiName) != prepends.end()) { 
        return UsdRiStatements();
    }

    SdfTokenListOp prependListOp;
    prepends.push_back(apiName);
    prependListOp.SetPrependedItems(prepends);
    auto result = listOp.ApplyOperations(prependListOp);
    if (!result) {
        TF_CODING_ERROR("Failed to prepend api name to current listop.");
        return UsdRiStatements();
    }

    // Set the listop at the current edit target and return the API prim
    primSpec->SetInfo(UsdTokens->apiSchemas, VtValue(*result));
    return UsdRiStatements(prim);
}

/* static */
const TfType &
UsdRiStatements::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiStatements>();
    return tfType;
}

/* static */
bool 
UsdRiStatements::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiStatements::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiStatements::GetFocusRegionAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->riFocusRegion);
}

UsdAttribute
UsdRiStatements::CreateFocusRegionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->riFocusRegion,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdRiStatements::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->riFocusRegion,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdSchemaBase::GetSchemaAttributeNames(true),
            localNames);

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
#include <string>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((fullAttributeNamespace, "ri:attributes:"))
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
UsdRiStatements::CreateRiAttribute(
    const TfToken& name, 
    const string &riType,
    const string &nameSpace)
{
    TfToken fullName = _MakeRiAttrNamespace(nameSpace, name.GetString());
    SdfValueTypeName usdType = UsdRi_GetUsdType(riType);
    UsdAttribute attr = GetPrim().CreateAttribute(fullName, usdType, 
                                                  /* custom = */ false);
    if (!TF_VERIFY(attr)) {
        return UsdAttribute();
    }
    return attr;
}

UsdAttribute
UsdRiStatements::CreateRiAttribute(
    const TfToken &name, 
    const TfType &tfType,
    const string &nameSpace)
{
    TfToken fullName = _MakeRiAttrNamespace(nameSpace, name.GetString());
    SdfValueTypeName usdType = SdfSchema::GetInstance().FindType(tfType);
    UsdAttribute attr = GetPrim().CreateAttribute(fullName, usdType,
                                                  /* custom = */ false);
    if (!TF_VERIFY(attr)) {
        return UsdAttribute();
    }
    return attr;
}

UsdRelationship
UsdRiStatements::CreateRiAttributeAsRel(
    const TfToken& name,
    const string &nameSpace)
{
    TfToken fullName = _MakeRiAttrNamespace(nameSpace, name.GetString());
    return GetPrim().CreateRelationship(fullName, /* custom = */ false);
}

std::vector<UsdProperty>
UsdRiStatements::GetRiAttributes(
    const string &nameSpace) const
{
    std::vector<UsdProperty> props = 
        GetPrim().GetPropertiesInNamespace(_tokens->fullAttributeNamespace);
    
    std::vector<UsdProperty> validProps;
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
    return validProps;
}


bool
UsdRiStatements::_IsCompatible(const UsdPrim &prim) const
{
    // HasA schemas compatible with all types for now.
    return true;
}

TfToken UsdRiStatements::GetRiAttributeNameSpace(const UsdProperty &prop)
{
    std::vector<string> names = prop.SplitName();
    if (names.size()<4) {
        return TfToken("");
    }

    return TfToken(TfStringJoin(names.begin() + 2, names.end()-1, ":"));
}

bool
UsdRiStatements::IsRiAttribute(const UsdProperty &attr)
{
    return TfStringStartsWith(attr.GetName(), _tokens->fullAttributeNamespace);
}

std::string
UsdRiStatements::MakeRiAttributePropertyName(const std::string &attrName)
{
    std::vector<string> names = TfStringTokenize(attrName, ":");
    if (names.size() == 4 && TfStringStartsWith(attrName, _tokens->fullAttributeNamespace)) {
        return attrName;
    }
    if (names.size() == 1) {
        names = TfStringTokenize(attrName, ".");
    }
    if (names.size() == 1) {
        names = TfStringTokenize(attrName, "_");
    }
    
    if (names.size() == 1) {
        names.insert(names.begin(), "user");
    }

    string fullName = _tokens->fullAttributeNamespace.GetString() + 
        names[0] + ":" + ( names.size() > 2 ?
                           TfStringJoin(names.begin() + 1, names.end(), "_")
                           : names[1]);

    return SdfPath::IsValidNamespacedIdentifier(fullName) ? fullName : string();
}

void
UsdRiStatements::SetCoordinateSystem(const std::string &coordSysName)
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
                    rel.AddTarget(GetPrim().GetPath(), UsdListPositionBack);
                }
                break;
            }

            currPrim = currPrim.GetParent();
        }
    }
}

std::string
UsdRiStatements::GetCoordinateSystem() const
{
    std::string result;
    UsdAttribute attr = GetPrim().GetAttribute(_tokens->coordsys);
    if (attr) {
        attr.Get(&result);
    }
    return result;
}

bool
UsdRiStatements::HasCoordinateSystem() const
{
    std::string result;
    UsdAttribute attr = GetPrim().GetAttribute(_tokens->coordsys);
    if (attr) {
        return attr.Get(&result);
    }
    return false;
}

void
UsdRiStatements::SetScopedCoordinateSystem(const std::string &coordSysName)
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
                    rel.AddTarget(GetPrim().GetPath(), UsdListPositionBack);
                }
                break;
            }

            currPrim = currPrim.GetParent();
        }
    }
}

std::string
UsdRiStatements::GetScopedCoordinateSystem() const
{
    std::string result;
    UsdAttribute attr = GetPrim().GetAttribute(_tokens->scopedCoordsys);
    if (attr) {
        attr.Get(&result);
    }
    return result;
}

bool
UsdRiStatements::HasScopedCoordinateSystem() const
{
    std::string result;
    UsdAttribute attr = GetPrim().GetAttribute(_tokens->scopedCoordsys);
    if (attr) {
        return attr.Get(&result);
    }
    return false;
}

bool
UsdRiStatements::GetModelCoordinateSystems(SdfPathVector *targets) const
{
    if (GetPrim().IsModel()) {
        UsdRelationship rel =
            GetPrim().GetRelationship(_tokens->modelCoordsys);
        return rel && rel.GetForwardedTargets(targets);
    }

    return true;
}

bool
UsdRiStatements::GetModelScopedCoordinateSystems(SdfPathVector *targets) const
{
    if (GetPrim().IsModel()) {
        UsdRelationship rel =
            GetPrim().GetRelationship(_tokens->modelScopedCoordsys);
        return rel && rel.GetForwardedTargets(targets);
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
