//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/screenSizeHeuristic.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLodScreenSizeHeuristic,
        TfType::Bases< UsdLodHeuristic > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("LODScreenSizeHeuristic")
    // to find TfType<UsdLodScreenSizeHeuristic>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLodScreenSizeHeuristic>("LODScreenSizeHeuristic");
}

/* virtual */
UsdLodScreenSizeHeuristic::~UsdLodScreenSizeHeuristic()
{
}

/* static */
UsdLodScreenSizeHeuristic
UsdLodScreenSizeHeuristic::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLodScreenSizeHeuristic();
    }
    return UsdLodScreenSizeHeuristic(stage->GetPrimAtPath(path));
}

/* static */
UsdLodScreenSizeHeuristic
UsdLodScreenSizeHeuristic::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("LODScreenSizeHeuristic");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLodScreenSizeHeuristic();
    }
    return UsdLodScreenSizeHeuristic(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLodScreenSizeHeuristic::_GetSchemaKind() const
{
    return UsdLodScreenSizeHeuristic::schemaKind;
}

/* static */
const TfType &
UsdLodScreenSizeHeuristic::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLodScreenSizeHeuristic>();
    return tfType;
}

/* static */
bool 
UsdLodScreenSizeHeuristic::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLodScreenSizeHeuristic::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLodScreenSizeHeuristic::GetExtentAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->extent);
}

UsdAttribute
UsdLodScreenSizeHeuristic::CreateExtentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->extent,
                       SdfValueTypeNames->Float3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLodScreenSizeHeuristic::GetProjectionMethodAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->projectionMethod);
}

UsdAttribute
UsdLodScreenSizeHeuristic::CreateProjectionMethodAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->projectionMethod,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLodScreenSizeHeuristic::GetThresholdsAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->thresholds);
}

UsdAttribute
UsdLodScreenSizeHeuristic::CreateThresholdsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->thresholds,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLodScreenSizeHeuristic::GetBlendThresholdsAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->blendThresholds);
}

UsdAttribute
UsdLodScreenSizeHeuristic::CreateBlendThresholdsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->blendThresholds,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLodScreenSizeHeuristic::GetBoundingVolumeRel() const
{
    return GetPrim().GetRelationship(UsdLodTokens->boundingVolume);
}

UsdRelationship
UsdLodScreenSizeHeuristic::CreateBoundingVolumeRel() const
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
UsdLodScreenSizeHeuristic::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLodTokens->extent,
        UsdLodTokens->projectionMethod,
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
UsdLodScreenSizeHeuristicQuery
UsdLodScreenSizeHeuristic::CreateScreenSizeHeuristicQuery(
    const UsdTimeCode time /* = UsdTimeCode::Default() */) const
{
    UsdLodScreenSizeHeuristicQuery query;

    const UsdAttribute lodDomainAttr = GetLodDomainAttr();
    if (!lodDomainAttr.Get(&query.lodDomain, time)) {
        TF_WARN("Unable to get lodDomain value from <%s> at time %s.",
                GetPrim().GetPath().GetText(),
                TfStringify(time).c_str());
    }

    const UsdAttribute extentAttr = GetExtentAttr();
    VtVec3fArray vtExtent;
    if (!extentAttr.Get(&vtExtent, time)) {
        TF_WARN("Unable to get extent value from <%s> at time %s.",
                extentAttr.GetPath().GetText(),
                TfStringify(time).c_str());
    } else if (vtExtent.size() == 2) {
        query.extent = GfRange3f(vtExtent[0], vtExtent[1]);
    } else {
        query.extent.SetEmpty();

        if (vtExtent.size() != 0) {
            TF_WARN("Invalid value for extent in <%s> at time %s.",
                extentAttr.GetPath().GetText(),
                    TfStringify(time).c_str());
        }
    }


    const UsdAttribute projectionMethodAttr = GetProjectionMethodAttr();
    if (!projectionMethodAttr.Get(&query.projectionMethod, time)) {
        TF_WARN("Unable to get projectionMethod value from <%s> at time %s.",
                projectionMethodAttr.GetPath().GetText(),
                TfStringify(time).c_str());
    }

    const UsdRelationship boundingVolumeRel = GetBoundingVolumeRel();
    SdfPathVector targetPaths;
    if (boundingVolumeRel.GetForwardedTargets(&targetPaths)) {
        for (const SdfPath& targetPath : targetPaths) {
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
