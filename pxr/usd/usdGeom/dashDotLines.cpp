//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/dashDotLines.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomDashDotLines,
        TfType::Bases< UsdGeomCurves > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("DashDotLines")
    // to find TfType<UsdGeomDashDotLines>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomDashDotLines>("DashDotLines");
}

/* virtual */
UsdGeomDashDotLines::~UsdGeomDashDotLines()
{
}

/* static */
UsdGeomDashDotLines
UsdGeomDashDotLines::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomDashDotLines();
    }
    return UsdGeomDashDotLines(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomDashDotLines
UsdGeomDashDotLines::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("DashDotLines");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomDashDotLines();
    }
    return UsdGeomDashDotLines(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomDashDotLines::_GetSchemaKind() const
{
    return UsdGeomDashDotLines::schemaKind;
}

/* static */
const TfType &
UsdGeomDashDotLines::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomDashDotLines>();
    return tfType;
}

/* static */
bool 
UsdGeomDashDotLines::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomDashDotLines::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomDashDotLines::GetShapeDetailAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->shapeDetail);
}

UsdAttribute
UsdGeomDashDotLines::CreateShapeDetailAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->shapeDetail,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomDashDotLines::GetScreenSpacePatternAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->screenSpacePattern);
}

UsdAttribute
UsdGeomDashDotLines::CreateScreenSpacePatternAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->screenSpacePattern,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomDashDotLines::GetPatternScaleAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->patternScale);
}

UsdAttribute
UsdGeomDashDotLines::CreatePatternScaleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->patternScale,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomDashDotLines::GetStartCapTypeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->startCapType);
}

UsdAttribute
UsdGeomDashDotLines::CreateStartCapTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->startCapType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomDashDotLines::GetEndCapTypeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->endCapType);
}

UsdAttribute
UsdGeomDashDotLines::CreateEndCapTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->endCapType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomDashDotLines::GetJointTypeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->jointType);
}

UsdAttribute
UsdGeomDashDotLines::CreateJointTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->jointType,
                       SdfValueTypeNames->Token,
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
UsdGeomDashDotLines::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->shapeDetail,
        UsdGeomTokens->screenSpacePattern,
        UsdGeomTokens->patternScale,
        UsdGeomTokens->startCapType,
        UsdGeomTokens->endCapType,
        UsdGeomTokens->jointType,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomCurves::GetSchemaAttributeNames(true),
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

TfToken
UsdGeomDashDotLines::GetTokenAttr(UsdAttribute attr, UsdTimeCode timeCode) const
{
    TfToken token;
    attr.Get(&token, timeCode);
    return token;
}

float
UsdGeomDashDotLines::GetFloatAttr(UsdAttribute attr, UsdTimeCode timeCode) const
{
    float value;
    attr.Get(&value, timeCode);
    return value;
}

int
UsdGeomDashDotLines::GetIntAttr(UsdAttribute attr, UsdTimeCode timeCode) const
{
    int value;
    attr.Get(&value, timeCode);
    return value;
}

bool
UsdGeomDashDotLines::GetBoolAttr(UsdAttribute attr, UsdTimeCode timeCode) const
{
    bool value;
    attr.Get(&value, timeCode);
    return value;
}
PXR_NAMESPACE_CLOSE_SCOPE
