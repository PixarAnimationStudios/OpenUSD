//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/dashDotPatternAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomDashDotPatternAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdGeomDashDotPatternAPI::~UsdGeomDashDotPatternAPI()
{
}

/* static */
UsdGeomDashDotPatternAPI
UsdGeomDashDotPatternAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomDashDotPatternAPI();
    }
    return UsdGeomDashDotPatternAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdGeomDashDotPatternAPI::_GetSchemaKind() const
{
    return UsdGeomDashDotPatternAPI::schemaKind;
}

/* static */
bool
UsdGeomDashDotPatternAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdGeomDashDotPatternAPI>(whyNot);
}

/* static */
UsdGeomDashDotPatternAPI
UsdGeomDashDotPatternAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdGeomDashDotPatternAPI>()) {
        return UsdGeomDashDotPatternAPI(prim);
    }
    return UsdGeomDashDotPatternAPI();
}

/* static */
const TfType &
UsdGeomDashDotPatternAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomDashDotPatternAPI>();
    return tfType;
}

/* static */
bool 
UsdGeomDashDotPatternAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomDashDotPatternAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomDashDotPatternAPI::GetPatternAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->pattern);
}

UsdAttribute
UsdGeomDashDotPatternAPI::CreatePatternAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->pattern,
                       SdfValueTypeNames->Float2Array,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomDashDotPatternAPI::GetPatternPeriodAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->patternPeriod);
}

UsdAttribute
UsdGeomDashDotPatternAPI::CreatePatternPeriodAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->patternPeriod,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
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
UsdGeomDashDotPatternAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->pattern,
        UsdGeomTokens->patternPeriod,
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
