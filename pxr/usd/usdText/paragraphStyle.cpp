//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/paragraphStyle.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextParagraphStyle,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ParagraphStyle")
    // to find TfType<UsdTextParagraphStyle>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdTextParagraphStyle>("ParagraphStyle");
}

/* virtual */
UsdTextParagraphStyle::~UsdTextParagraphStyle()
{
}

/* static */
UsdTextParagraphStyle
UsdTextParagraphStyle::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextParagraphStyle();
    }
    return UsdTextParagraphStyle(stage->GetPrimAtPath(path));
}

/* static */
UsdTextParagraphStyle
UsdTextParagraphStyle::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ParagraphStyle");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextParagraphStyle();
    }
    return UsdTextParagraphStyle(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdTextParagraphStyle::_GetSchemaKind() const
{
    return UsdTextParagraphStyle::schemaKind;
}

/* static */
const TfType &
UsdTextParagraphStyle::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextParagraphStyle>();
    return tfType;
}

/* static */
bool 
UsdTextParagraphStyle::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextParagraphStyle::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdTextParagraphStyle::GetFirstLineIndentAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->firstLineIndent);
}

UsdAttribute
UsdTextParagraphStyle::CreateFirstLineIndentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->firstLineIndent,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextParagraphStyle::GetLeftIndentAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->leftIndent);
}

UsdAttribute
UsdTextParagraphStyle::CreateLeftIndentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->leftIndent,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextParagraphStyle::GetRightIndentAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->rightIndent);
}

UsdAttribute
UsdTextParagraphStyle::CreateRightIndentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->rightIndent,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextParagraphStyle::GetParagraphSpaceAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->paragraphSpace);
}

UsdAttribute
UsdTextParagraphStyle::CreateParagraphSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->paragraphSpace,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextParagraphStyle::GetParagraphAlignmentAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->paragraphAlignment);
}

UsdAttribute
UsdTextParagraphStyle::CreateParagraphAlignmentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->paragraphAlignment,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextParagraphStyle::GetTabStopPositionsAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->tabStopPositions);
}

UsdAttribute
UsdTextParagraphStyle::CreateTabStopPositionsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->tabStopPositions,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextParagraphStyle::GetTabStopTypesAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->tabStopTypes);
}

UsdAttribute
UsdTextParagraphStyle::CreateTabStopTypesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->tabStopTypes,
                       SdfValueTypeNames->TokenArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextParagraphStyle::GetLineSpaceAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->lineSpace);
}

UsdAttribute
UsdTextParagraphStyle::CreateLineSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->lineSpace,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextParagraphStyle::GetLineSpaceTypeAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->lineSpaceType);
}

UsdAttribute
UsdTextParagraphStyle::CreateLineSpaceTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->lineSpaceType,
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
UsdTextParagraphStyle::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTextTokens->firstLineIndent,
        UsdTextTokens->leftIndent,
        UsdTextTokens->rightIndent,
        UsdTextTokens->paragraphSpace,
        UsdTextTokens->paragraphAlignment,
        UsdTextTokens->tabStopPositions,
        UsdTextTokens->tabStopTypes,
        UsdTextTokens->lineSpace,
        UsdTextTokens->lineSpaceType,
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
