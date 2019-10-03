//
// Copyright 2016 Pixar
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
UsdSchemaType UsdRenderProduct::_GetSchemaType() const {
    return UsdRenderProduct::schemaType;
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
