//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdUI/backdrop.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdUIBackdrop,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Backdrop")
    // to find TfType<UsdUIBackdrop>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdUIBackdrop>("Backdrop");
}

/* virtual */
UsdUIBackdrop::~UsdUIBackdrop()
{
}

/* static */
UsdUIBackdrop
UsdUIBackdrop::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdUIBackdrop();
    }
    return UsdUIBackdrop(stage->GetPrimAtPath(path));
}

/* static */
UsdUIBackdrop
UsdUIBackdrop::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Backdrop");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdUIBackdrop();
    }
    return UsdUIBackdrop(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdUIBackdrop::_GetSchemaKind() const
{
    return UsdUIBackdrop::schemaKind;
}

/* static */
const TfType &
UsdUIBackdrop::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdUIBackdrop>();
    return tfType;
}

/* static */
bool 
UsdUIBackdrop::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdUIBackdrop::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdUIBackdrop::GetDescriptionAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiDescription);
}

UsdAttribute
UsdUIBackdrop::CreateDescriptionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiDescription,
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
UsdUIBackdrop::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdUITokens->uiDescription,
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
