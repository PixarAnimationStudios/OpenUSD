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
#include "pxr/usd/usdLux/lightListAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxLightListAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLuxLightListAPI::~UsdLuxLightListAPI()
{
}

/* static */
UsdLuxLightListAPI
UsdLuxLightListAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxLightListAPI();
    }
    return UsdLuxLightListAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLuxLightListAPI::_GetSchemaKind() const
{
    return UsdLuxLightListAPI::schemaKind;
}

/* static */
bool
UsdLuxLightListAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLuxLightListAPI>(whyNot);
}

/* static */
UsdLuxLightListAPI
UsdLuxLightListAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLuxLightListAPI>()) {
        return UsdLuxLightListAPI(prim);
    }
    return UsdLuxLightListAPI();
}

/* static */
const TfType &
UsdLuxLightListAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxLightListAPI>();
    return tfType;
}

/* static */
bool 
UsdLuxLightListAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxLightListAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxLightListAPI::GetLightListCacheBehaviorAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->lightListCacheBehavior);
}

UsdAttribute
UsdLuxLightListAPI::CreateLightListCacheBehaviorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->lightListCacheBehavior,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLuxLightListAPI::GetLightListRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->lightList);
}

UsdRelationship
UsdLuxLightListAPI::CreateLightListRel() const
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
UsdLuxLightListAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->lightListCacheBehavior,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
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
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/lightFilter.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdLuxLightListAPI::ComputeModeConsultModelHierarchyCache,
                     "Consult lightList cache");
    TF_ADD_ENUM_NAME(UsdLuxLightListAPI::ComputeModeIgnoreCache,
                     "Ignore lightList cache");
}

static void
_Traverse(const UsdPrim &prim,
          UsdLuxLightListAPI::ComputeMode mode,
          SdfPathSet *lights)
{
    // If requested, check lightList cache.
    if (mode == UsdLuxLightListAPI::ComputeModeConsultModelHierarchyCache &&
        prim.GetPath().IsPrimPath() /* no cache on pseudoRoot */) {
        UsdLuxLightListAPI listAPI(prim);
        TfToken cacheBehavior;
        if (listAPI.GetLightListCacheBehaviorAttr().Get(&cacheBehavior)) {
            if (cacheBehavior == UsdLuxTokens->consumeAndContinue ||
                cacheBehavior == UsdLuxTokens->consumeAndHalt) {
                // Check stored lightList.
                UsdRelationship rel = listAPI.GetLightListRel();
                SdfPathVector targets;
                rel.GetForwardedTargets(&targets);
                lights->insert(targets.begin(), targets.end());
                if (cacheBehavior == UsdLuxTokens->consumeAndHalt) {
                    return;
                }
            }
        }
    }
    // Accumulate discovered prims.
    if (prim.HasAPI<UsdLuxLightAPI>() || prim.IsA<UsdLuxLightFilter>()) {
        lights->insert(prim.GetPath());
    }
    // Traverse descendants.
    auto flags = UsdPrimIsActive && !UsdPrimIsAbstract && UsdPrimIsDefined;
    if (mode == UsdLuxLightListAPI::ComputeModeConsultModelHierarchyCache) {
        // When consulting the cache we only traverse model hierarchy.
        flags = flags && UsdPrimIsModel;
    }
    for (const UsdPrim &child:
         prim.GetFilteredChildren(UsdTraverseInstanceProxies(flags))) {
        _Traverse(child, mode, lights);
    }
}

SdfPathSet
UsdLuxLightListAPI::ComputeLightList(UsdLuxLightListAPI::ComputeMode mode) const
{
    SdfPathSet result;
    _Traverse(GetPrim(), mode, &result);
    return result;
}

void
UsdLuxLightListAPI::StoreLightList(const SdfPathSet &lights) const
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
    CreateLightListCacheBehaviorAttr().Set(UsdLuxTokens->consumeAndContinue);
}

void
UsdLuxLightListAPI::InvalidateLightList() const
{
    CreateLightListCacheBehaviorAttr().Set(UsdLuxTokens->ignore);
}

PXR_NAMESPACE_CLOSE_SCOPE
