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
#include "pxr/usd/usdLux/shadowAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxShadowAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdLuxShadowAPI::~UsdLuxShadowAPI()
{
}

/* static */
UsdLuxShadowAPI
UsdLuxShadowAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxShadowAPI();
    }
    return UsdLuxShadowAPI(stage->GetPrimAtPath(path));
}


/* static */
UsdLuxShadowAPI
UsdLuxShadowAPI::Apply(const UsdStagePtr &stage, const SdfPath &path)
{
    // Ensure we have a valid stage, path and prim
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxShadowAPI();
    }

    if (path == SdfPath::AbsoluteRootPath()) {
        TF_CODING_ERROR("Cannot apply an api schema on the pseudoroot");
        return UsdLuxShadowAPI();
    }

    auto prim = stage->GetPrimAtPath(path);
    if (!prim) {
        TF_CODING_ERROR("Prim at <%s> does not exist.", path.GetText());
        return UsdLuxShadowAPI();
    }

    TfToken apiName("ShadowAPI");  

    // Get the current listop at the edit target
    UsdEditTarget editTarget = stage->GetEditTarget();
    SdfPrimSpecHandle primSpec = editTarget.GetPrimSpecForScenePath(path);
    SdfTokenListOp listOp = primSpec->GetInfo(UsdTokens->apiSchemas)
                                    .UncheckedGet<SdfTokenListOp>();

    // Append our name to the prepend list, if it doesnt exist locally
    TfTokenVector prepends = listOp.GetPrependedItems();
    if (std::find(prepends.begin(), prepends.end(), apiName) != prepends.end()) { 
        return UsdLuxShadowAPI();
    }

    SdfTokenListOp prependListOp;
    prepends.push_back(apiName);
    prependListOp.SetPrependedItems(prepends);
    auto result = listOp.ApplyOperations(prependListOp);
    if (!result) {
        TF_CODING_ERROR("Failed to prepend api name to current listop.");
        return UsdLuxShadowAPI();
    }

    // Set the listop at the current edit target and return the API prim
    primSpec->SetInfo(UsdTokens->apiSchemas, VtValue(*result));
    return UsdLuxShadowAPI(prim);
}

/* static */
const TfType &
UsdLuxShadowAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxShadowAPI>();
    return tfType;
}

/* static */
bool 
UsdLuxShadowAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxShadowAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxShadowAPI::GetShadowEnableAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shadowEnable);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowEnableAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shadowEnable,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShadowAPI::GetShadowColorAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shadowColor);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shadowColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShadowAPI::GetShadowDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shadowDistance);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shadowDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShadowAPI::GetShadowFalloffAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shadowFalloff);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowFalloffAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shadowFalloff,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShadowAPI::GetShadowFalloffGammaAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->shadowFalloffGamma);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowFalloffGammaAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->shadowFalloffGamma,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLuxShadowAPI::GetShadowIncludeRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->shadowInclude);
}

UsdRelationship
UsdLuxShadowAPI::CreateShadowIncludeRel() const
{
    return GetPrim().CreateRelationship(UsdLuxTokens->shadowInclude,
                       /* custom = */ false);
}

UsdRelationship
UsdLuxShadowAPI::GetShadowExcludeRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->shadowExclude);
}

UsdRelationship
UsdLuxShadowAPI::CreateShadowExcludeRel() const
{
    return GetPrim().CreateRelationship(UsdLuxTokens->shadowExclude,
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
UsdLuxShadowAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->shadowEnable,
        UsdLuxTokens->shadowColor,
        UsdLuxTokens->shadowDistance,
        UsdLuxTokens->shadowFalloff,
        UsdLuxTokens->shadowFalloffGamma,
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
