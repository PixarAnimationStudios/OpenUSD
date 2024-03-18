//
// Copyright 2023 Pixar
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
UsdTextTextStyle::GetTypefaceAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->typeface);
}

UsdAttribute
UsdTextTextStyle::CreateTypefaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->typeface,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetBoldAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->bold);
}

UsdAttribute
UsdTextTextStyle::CreateBoldAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->bold,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetItalicAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->italic);
}

UsdAttribute
UsdTextTextStyle::CreateItalicAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->italic,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetWeightAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->weight);
}

UsdAttribute
UsdTextTextStyle::CreateWeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->weight,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetTextHeightAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->textHeight);
}

UsdAttribute
UsdTextTextStyle::CreateTextHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->textHeight,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextStyle::GetTextWidthFactorAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->textWidthFactor);
}

UsdAttribute
UsdTextTextStyle::CreateTextWidthFactorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->textWidthFactor,
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
UsdTextTextStyle::GetCharSpacingAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->charSpacing);
}

UsdAttribute
UsdTextTextStyle::CreateCharSpacingAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->charSpacing,
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
        UsdTextTokens->typeface,
        UsdTextTokens->bold,
        UsdTextTokens->italic,
        UsdTextTokens->weight,
        UsdTextTokens->textHeight,
        UsdTextTokens->textWidthFactor,
        UsdTextTokens->obliqueAngle,
        UsdTextTokens->charSpacing,
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
