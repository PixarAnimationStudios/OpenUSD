//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/simpleText.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextSimpleText,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("SimpleText")
    // to find TfType<UsdTextSimpleText>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdTextSimpleText>("SimpleText");
}

/* virtual */
UsdTextSimpleText::~UsdTextSimpleText()
{
}

/* static */
UsdTextSimpleText
UsdTextSimpleText::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextSimpleText();
    }
    return UsdTextSimpleText(stage->GetPrimAtPath(path));
}

/* static */
UsdTextSimpleText
UsdTextSimpleText::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("SimpleText");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextSimpleText();
    }
    return UsdTextSimpleText(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdTextSimpleText::_GetSchemaKind() const
{
    return UsdTextSimpleText::schemaKind;
}

/* static */
const TfType &
UsdTextSimpleText::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextSimpleText>();
    return tfType;
}

/* static */
bool 
UsdTextSimpleText::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextSimpleText::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdTextSimpleText::GetTextDataAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->textData);
}

UsdAttribute
UsdTextSimpleText::CreateTextDataAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->textData,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextSimpleText::GetBackgroundColorAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->primvarsBackgroundColor);
}

UsdAttribute
UsdTextSimpleText::CreateBackgroundColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->primvarsBackgroundColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextSimpleText::GetBackgroundOpacityAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->primvarsBackgroundOpacity);
}

UsdAttribute
UsdTextSimpleText::CreateBackgroundOpacityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->primvarsBackgroundOpacity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextSimpleText::GetTextMetricsUnitAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->textMetricsUnit);
}

UsdAttribute
UsdTextSimpleText::CreateTextMetricsUnitAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->textMetricsUnit,
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
UsdTextSimpleText::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTextTokens->textData,
        UsdTextTokens->primvarsBackgroundColor,
        UsdTextTokens->primvarsBackgroundOpacity,
        UsdTextTokens->textMetricsUnit,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomGprim::GetSchemaAttributeNames(true),
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

#include "pxr/usd/usdGeom/boundableComputeExtent.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/usdGeom/pointBased.h"

PXR_NAMESPACE_OPEN_SCOPE

// A temporary function to calculate the extent for the simpleText. The extent is hardcoded to
// [0.0, -500.0, 500.0, 0.0]. This calculation need to be rewrite after the simpleText is moved
// to usdText project.
static bool
_ComputeExtentForSimpleText(
    const UsdGeomBoundable& boundable,
    const UsdTimeCode& time,
    const GfMatrix4d* transform,
    VtVec3fArray* extent)
{
    const UsdTextSimpleText simpleText(boundable);
    if (!TF_VERIFY(simpleText)) {
        return false;
    }

    extent->resize(2);
    (*extent)[0] = GfVec3f(0.0f, -500.0f, -1.0f);
    (*extent)[1] = GfVec3f(500.0f, 0.0f, 1.0f);
    return true;
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdTextSimpleText>(
        _ComputeExtentForSimpleText);
}

PXR_NAMESPACE_CLOSE_SCOPE
