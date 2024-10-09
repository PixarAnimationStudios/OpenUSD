//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/textStyle.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextTextStyle,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("TextStyle")
    // to find TfType<UsdTextTextStyle>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdTextTextStyle>("TextStyle");
}

/* virtual */
UsdTextTextStyle::~UsdTextTextStyle()
{
}

/* static */
UsdTextTextStyle
UsdTextTextStyle::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextTextStyle();
    }
    return UsdTextTextStyle(stage->GetPrimAtPath(path));
}

/* static */
UsdTextTextStyle
UsdTextTextStyle::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("TextStyle");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextTextStyle();
    }
    return UsdTextTextStyle(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdTextTextStyle::_GetSchemaKind() const
{
    return UsdTextTextStyle::schemaKind;
}

/* static */
const TfType &
UsdTextTextStyle::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextTextStyle>();
    return tfType;
}

/* static */
bool 
UsdTextTextStyle::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextTextStyle::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdTextTextStyle::GetFontTypefaceAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->fontTypeface);
}

UsdAttribute
UsdTextTextStyle::CreateFontTypefaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->fontTypeface,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetFontFormatAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->fontFormat);
}

UsdAttribute
UsdTextTextStyle::CreateFontFormatAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->fontFormat,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetFontAltTypefaceAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->fontAltTypeface);
}

UsdAttribute
UsdTextTextStyle::CreateFontAltTypefaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->fontAltTypeface,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetFontAltFormatAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->fontAltFormat);
}

UsdAttribute
UsdTextTextStyle::CreateFontAltFormatAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->fontAltFormat,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetFontBoldAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->fontBold);
}

UsdAttribute
UsdTextTextStyle::CreateFontBoldAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->fontBold,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetFontItalicAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->fontItalic);
}

UsdAttribute
UsdTextTextStyle::CreateFontItalicAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->fontItalic,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetFontWeightAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->fontWeight);
}

UsdAttribute
UsdTextTextStyle::CreateFontWeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->fontWeight,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetCharHeightAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->charHeight);
}

UsdAttribute
UsdTextTextStyle::CreateCharHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->charHeight,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetCharWidthFactorAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->charWidthFactor);
}

UsdAttribute
UsdTextTextStyle::CreateCharWidthFactorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->charWidthFactor,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetObliqueAngleAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->obliqueAngle);
}

UsdAttribute
UsdTextTextStyle::CreateObliqueAngleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->obliqueAngle,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetCharSpacingFactorAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->charSpacingFactor);
}

UsdAttribute
UsdTextTextStyle::CreateCharSpacingFactorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->charSpacingFactor,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetUnderlineTypeAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->underlineType);
}

UsdAttribute
UsdTextTextStyle::CreateUnderlineTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->underlineType,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetOverlineTypeAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->overlineType);
}

UsdAttribute
UsdTextTextStyle::CreateOverlineTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->overlineType,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetStrikethroughTypeAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->strikethroughType);
}

UsdAttribute
UsdTextTextStyle::CreateStrikethroughTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->strikethroughType,
                       SdfValueTypeNames->String,
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
UsdTextTextStyle::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTextTokens->fontTypeface,
        UsdTextTokens->fontFormat,
        UsdTextTokens->fontAltTypeface,
        UsdTextTokens->fontAltFormat,
        UsdTextTokens->fontBold,
        UsdTextTokens->fontItalic,
        UsdTextTokens->fontWeight,
        UsdTextTokens->charHeight,
        UsdTextTokens->charWidthFactor,
        UsdTextTokens->obliqueAngle,
        UsdTextTokens->charSpacingFactor,
        UsdTextTokens->underlineType,
        UsdTextTokens->overlineType,
        UsdTextTokens->strikethroughType,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
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
