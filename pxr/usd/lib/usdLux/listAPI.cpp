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
#include "pxr/usd/usdLux/listAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxListAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdLuxListAPI::~UsdLuxListAPI()
{
}

/* static */
UsdLuxListAPI
UsdLuxListAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxListAPI();
    }
    return UsdLuxListAPI(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdLuxListAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxListAPI>();
    return tfType;
}

/* static */
bool 
UsdLuxListAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxListAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxListAPI::GetLightListIsValidAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->lightListIsValid);
}

UsdAttribute
UsdLuxListAPI::CreateLightListIsValidAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->lightListIsValid,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLuxListAPI::GetLightListRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->lightList);
}

UsdRelationship
UsdLuxListAPI::CreateLightListRel() const
{
    return GetPrim().CreateRelationship(UsdLuxTokens->lightList,
                       /* custom = */ false);
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
UsdLuxListAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->lightListIsValid,
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

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdLux/light.h"
#include "pxr/usd/usdLux/lightFilter.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdLuxListAPI::StoredListConsult,
                     "Consult stored lightList");
    TF_ADD_ENUM_NAME(UsdLuxListAPI::StoredListIgnore,
                     "Ignore stored lightList");
}

static void
_Traverse(const UsdPrim &prim,
          UsdLuxListAPI::StoredListBehavior listBehavior,
          SdfPathSet *lights)
{
    if (listBehavior == UsdLuxListAPI::StoredListConsult &&
        prim.GetPath().IsPrimPath()) {
        UsdLuxListAPI list_api(prim);
        if (list_api.IsLightListValid()) {
            UsdRelationship rel = list_api.GetLightListRel();
            SdfPathVector targets;
            rel.GetForwardedTargets(&targets);
            lights->insert(targets.begin(), targets.end());
        }
    }
    if (prim.IsA<UsdLuxLight>() || prim.IsA<UsdLuxLightFilter>()) {
        lights->insert(prim.GetPath());
    }
    // Allow unloaded and not-yet-defined prims
    UsdPrim::SiblingRange range =
        prim.GetFilteredChildren(UsdTraverseInstanceProxies(
                                UsdPrimIsActive && !UsdPrimIsAbstract));
    for (auto i = range.begin(); i != range.end(); ++i) {
        _Traverse(*i, listBehavior, lights);
    }
}

SdfPathSet
UsdLuxListAPI::ComputeLightList(UsdLuxListAPI::StoredListBehavior b) const
{
    SdfPathSet result;
    _Traverse(GetPrim(), b, &result);
    return result;
}

void
UsdLuxListAPI::StoreLightList(const SdfPathSet &lights) const
{
    SdfPathVector targets;
    for (const SdfPath &p: lights) {
        if (p.IsAbsolutePath() && !p.HasPrefix(GetPath())) {
            // Light path does not have this prim as a prefix; ignore.
            continue;
        }
        targets.push_back(p);
    }
    CreateLightListRel().SetTargets(targets);
    CreateLightListIsValidAttr().Set(true);
}

void
UsdLuxListAPI::InvalidateLightList() const
{
    CreateLightListIsValidAttr().Set(false);
}

bool
UsdLuxListAPI::IsLightListValid() const
{
    bool valid = false;
    if (GetPrim()) {
        GetLightListIsValidAttr().Get(&valid);
    }
    return valid;
}

PXR_NAMESPACE_CLOSE_SCOPE
