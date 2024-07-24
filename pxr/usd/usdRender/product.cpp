//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRender/product.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRenderProduct,
        TfType::Bases< UsdRenderSettingsBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("RenderProduct")
    // to find TfType<UsdRenderProduct>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRenderProduct>("RenderProduct");
}

/* virtual */
UsdRenderProduct::~UsdRenderProduct()
{
}

/* static */
UsdRenderProduct
UsdRenderProduct::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderProduct();
    }
    return UsdRenderProduct(stage->GetPrimAtPath(path));
}

/* static */
UsdRenderProduct
UsdRenderProduct::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("RenderProduct");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderProduct();
    }
    return UsdRenderProduct(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdRenderProduct::_GetSchemaKind() const
{
    return UsdRenderProduct::schemaKind;
}

/* static */
const TfType &
UsdRenderProduct::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRenderProduct>();
    return tfType;
}

/* static */
bool 
UsdRenderProduct::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRenderProduct::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRenderProduct::GetProductTypeAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->productType);
}

UsdAttribute
UsdRenderProduct::CreateProductTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->productType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderProduct::GetProductNameAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->productName);
}

UsdAttribute
UsdRenderProduct::CreateProductNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->productName,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdRenderProduct::GetOrderedVarsRel() const
{
    return GetPrim().GetRelationship(UsdRenderTokens->orderedVars);
}

UsdRelationship
UsdRenderProduct::CreateOrderedVarsRel() const
{
    return GetPrim().CreateRelationship(UsdRenderTokens->orderedVars,
                       /* custom = */ false);
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
UsdRenderProduct::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRenderTokens->productType,
        UsdRenderTokens->productName,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdRenderSettingsBase::GetSchemaAttributeNames(true),
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
