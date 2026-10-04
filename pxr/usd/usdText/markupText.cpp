//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/markupText.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextMarkupText,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("MarkupText")
    // to find TfType<UsdTextMarkupText>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdTextMarkupText>("MarkupText");
}

/* virtual */
UsdTextMarkupText::~UsdTextMarkupText()
{
}

/* static */
UsdTextMarkupText
UsdTextMarkupText::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextMarkupText();
    }
    return UsdTextMarkupText(stage->GetPrimAtPath(path));
}

/* static */
UsdTextMarkupText
UsdTextMarkupText::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("MarkupText");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextMarkupText();
    }
    return UsdTextMarkupText(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdTextMarkupText::_GetSchemaKind() const
{
    return UsdTextMarkupText::schemaKind;
}

/* static */
const TfType &
UsdTextMarkupText::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextMarkupText>();
    return tfType;
}

/* static */
bool 
UsdTextMarkupText::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextMarkupText::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdTextMarkupText::GetMarkupAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->markup);
}

UsdAttribute
UsdTextMarkupText::CreateMarkupAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->markup,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetMarkupPlainAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->markupPlain);
}

UsdAttribute
UsdTextMarkupText::CreateMarkupPlainAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->markupPlain,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetMarkupLanguageAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->markupLanguage);
}

UsdAttribute
UsdTextMarkupText::CreateMarkupLanguageAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->markupLanguage,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetBackgroundColorAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->primvarsBackgroundColor);
}

UsdAttribute
UsdTextMarkupText::CreateBackgroundColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->primvarsBackgroundColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetBackgroundOpacityAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->primvarsBackgroundOpacity);
}

UsdAttribute
UsdTextMarkupText::CreateBackgroundOpacityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->primvarsBackgroundOpacity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextMarkupText::GetTextMetricsUnitAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->textMetricsUnit);
}

UsdAttribute
UsdTextMarkupText::CreateTextMetricsUnitAttr(VtValue const &defaultValue, bool writeSparsely) const
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
UsdTextMarkupText::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTextTokens->markup,
        UsdTextTokens->markupPlain,
        UsdTextTokens->markupLanguage,
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
