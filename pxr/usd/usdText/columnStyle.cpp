//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/columnStyle.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextColumnStyle,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ColumnStyle")
    // to find TfType<UsdTextColumnStyle>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdTextColumnStyle>("ColumnStyle");
}

/* virtual */
UsdTextColumnStyle::~UsdTextColumnStyle()
{
}

/* static */
UsdTextColumnStyle
UsdTextColumnStyle::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextColumnStyle();
    }
    return UsdTextColumnStyle(stage->GetPrimAtPath(path));
}

/* static */
UsdTextColumnStyle
UsdTextColumnStyle::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ColumnStyle");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextColumnStyle();
    }
    return UsdTextColumnStyle(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdTextColumnStyle::_GetSchemaKind() const
{
    return UsdTextColumnStyle::schemaKind;
}

/* static */
const TfType &
UsdTextColumnStyle::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextColumnStyle>();
    return tfType;
}

/* static */
bool 
UsdTextColumnStyle::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextColumnStyle::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdTextColumnStyle::GetColumnWidthAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->columnWidth);
}

UsdAttribute
UsdTextColumnStyle::CreateColumnWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->columnWidth,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetColumnHeightAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->columnHeight);
}

UsdAttribute
UsdTextColumnStyle::CreateColumnHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->columnHeight,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetColumnOffsetAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->columnOffset);
}

UsdAttribute
UsdTextColumnStyle::CreateColumnOffsetAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->columnOffset,
                       SdfValueTypeNames->Float2,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetMarginsAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->margins);
}

UsdAttribute
UsdTextColumnStyle::CreateMarginsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->margins,
                       SdfValueTypeNames->Float4,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextColumnStyle::GetColumnAlignmentAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->columnAlignment);
}

UsdAttribute
UsdTextColumnStyle::CreateColumnAlignmentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->columnAlignment,
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
UsdTextColumnStyle::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTextTokens->columnWidth,
        UsdTextTokens->columnHeight,
        UsdTextTokens->columnOffset,
        UsdTextTokens->margins,
        UsdTextTokens->columnAlignment,
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
