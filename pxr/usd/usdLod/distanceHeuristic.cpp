//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/distanceHeuristic.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLodDistanceHeuristic,
        TfType::Bases< UsdLodHeuristic > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("LODDistanceHeuristic")
    // to find TfType<UsdLodDistanceHeuristic>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLodDistanceHeuristic>("LODDistanceHeuristic");
}

/* virtual */
UsdLodDistanceHeuristic::~UsdLodDistanceHeuristic()
{
}

/* static */
UsdLodDistanceHeuristic
UsdLodDistanceHeuristic::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLodDistanceHeuristic();
    }
    return UsdLodDistanceHeuristic(stage->GetPrimAtPath(path));
}

/* static */
UsdLodDistanceHeuristic
UsdLodDistanceHeuristic::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("LODDistanceHeuristic");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLodDistanceHeuristic();
    }
    return UsdLodDistanceHeuristic(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLodDistanceHeuristic::_GetSchemaKind() const
{
    return UsdLodDistanceHeuristic::schemaKind;
}

/* static */
const TfType &
UsdLodDistanceHeuristic::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLodDistanceHeuristic>();
    return tfType;
}

/* static */
bool 
UsdLodDistanceHeuristic::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLodDistanceHeuristic::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLodDistanceHeuristic::GetCenterAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->center);
}

UsdAttribute
UsdLodDistanceHeuristic::CreateCenterAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->center,
                       SdfValueTypeNames->Point3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLodDistanceHeuristic::GetThresholdsAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->thresholds);
}

UsdAttribute
UsdLodDistanceHeuristic::CreateThresholdsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->thresholds,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLodDistanceHeuristic::GetBlendThresholdsAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->blendThresholds);
}

UsdAttribute
UsdLodDistanceHeuristic::CreateBlendThresholdsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->blendThresholds,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLodDistanceHeuristic::GetBoundingVolumeRel() const
{
    return GetPrim().GetRelationship(UsdLodTokens->boundingVolume);
}

UsdRelationship
UsdLodDistanceHeuristic::CreateBoundingVolumeRel() const
{
    return GetPrim().CreateRelationship(UsdLodTokens->boundingVolume,
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
UsdLodDistanceHeuristic::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLodTokens->center,
        UsdLodTokens->thresholds,
        UsdLodTokens->blendThresholds,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLodHeuristic::GetSchemaAttributeNames(true),
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

PXR_NAMESPACE_OPEN_SCOPE

USDLOD_API
UsdLodDistanceHeuristicQuery
UsdLodDistanceHeuristic::CreateDistanceHeuristicQuery(
    const UsdTimeCode time /* = UsdTimeCode::Default() */) const
{
    UsdLodDistanceHeuristicQuery query;

    const UsdAttribute lodDomainAttr = GetLodDomainAttr();
    if (!lodDomainAttr.Get(&query.lodDomain, time)) {
        TF_WARN("Unable to get lodDomain value from <%s> at time %s.",
                GetPrim().GetPath().GetText(),
                TfStringify(time).c_str());
    }

    const UsdAttribute centerAttr = GetCenterAttr();
    if (!centerAttr.Get(&query.center, time)) {
        TF_WARN("Unable to get center value from <%s> at time %s.",
                centerAttr.GetPath().GetText(),
                TfStringify(time).c_str());
    }

    query.extent.SetEmpty();

    const UsdRelationship boundingVolumeRel = GetBoundingVolumeRel();
    SdfPathVector targetPaths;
    if (boundingVolumeRel.GetForwardedTargets(&targetPaths)) {
        for (const SdfPath& targetPath : targetPaths) {
            VtVec3fArray vtExtent;
            const UsdPrim targetPrim = GetPrim().GetPrimAtPath(targetPath);
            if (targetPrim &&
                targetPrim.IsA<UsdGeomBoundable>() &&
                UsdGeomBoundable(targetPrim).ComputeExtent(time, &vtExtent) &&
                vtExtent.size() == 2)
            {
                query.extent = GfRange3f(vtExtent[0], vtExtent[1]);
                // Note that the result may still be an empty extent.
                break;
            }
        }
    }

    UsdAttribute thresholdsAttr = GetThresholdsAttr();
    if (!thresholdsAttr.Get(&query.thresholds, time)) {
        TF_WARN("Unable to get thresholds value from <%s> at time %s.",
                thresholdsAttr.GetPath().GetText(),
                TfStringify(time).c_str());
    }

    UsdAttribute blendThresholdsAttr = GetBlendThresholdsAttr();
    if (!blendThresholdsAttr.Get(&query.blendThresholds, time)) {
        TF_WARN("Unable to get blendThresholds value from <%s> at time %s.",
                blendThresholdsAttr.GetPath().GetText(),
                TfStringify(time).c_str());
    }

    return query;
}

PXR_NAMESPACE_CLOSE_SCOPE
