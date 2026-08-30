//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/textLayoutAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextTextLayoutAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdTextTextLayoutAPI::~UsdTextTextLayoutAPI()
{
}

/* static */
UsdTextTextLayoutAPI
UsdTextTextLayoutAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextTextLayoutAPI();
    }
    return UsdTextTextLayoutAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdTextTextLayoutAPI::_GetSchemaKind() const
{
    return UsdTextTextLayoutAPI::schemaKind;
}

/* static */
bool
UsdTextTextLayoutAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdTextTextLayoutAPI>(whyNot);
}

/* static */
UsdTextTextLayoutAPI
UsdTextTextLayoutAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdTextTextLayoutAPI>()) {
        return UsdTextTextLayoutAPI(prim);
    }
    return UsdTextTextLayoutAPI();
}

/* static */
const TfType &
UsdTextTextLayoutAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextTextLayoutAPI>();
    return tfType;
}

/* static */
bool 
UsdTextTextLayoutAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextTextLayoutAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdTextTextLayoutAPI::GetLayoutBaselineDirectionAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->layoutBaselineDirection);
}

UsdAttribute
UsdTextTextLayoutAPI::CreateLayoutBaselineDirectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->layoutBaselineDirection,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdTextTextLayoutAPI::GetLayoutLinesStackDirectionAttr() const
{
    return GetPrim().GetAttribute(UsdTextTokens->layoutLinesStackDirection);
}

UsdAttribute
UsdTextTextLayoutAPI::CreateLayoutLinesStackDirectionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdTextTokens->layoutLinesStackDirection,
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
UsdTextTextLayoutAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdTextTokens->layoutBaselineDirection,
        UsdTextTokens->layoutLinesStackDirection,
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
