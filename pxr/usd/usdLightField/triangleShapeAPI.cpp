//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/triangleShapeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldTriangleShapeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldTriangleShapeAPI::~UsdLightFieldTriangleShapeAPI()
{
}

/* static */
UsdLightFieldTriangleShapeAPI
UsdLightFieldTriangleShapeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldTriangleShapeAPI();
    }
    return UsdLightFieldTriangleShapeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldTriangleShapeAPI::_GetSchemaKind() const
{
    return UsdLightFieldTriangleShapeAPI::schemaKind;
}

/* static */
bool
UsdLightFieldTriangleShapeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldTriangleShapeAPI>(whyNot);
}

/* static */
UsdLightFieldTriangleShapeAPI
UsdLightFieldTriangleShapeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldTriangleShapeAPI>()) {
        return UsdLightFieldTriangleShapeAPI(prim);
    }
    return UsdLightFieldTriangleShapeAPI();
}

/* static */
const TfType &
UsdLightFieldTriangleShapeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldTriangleShapeAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldTriangleShapeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldTriangleShapeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldTriangleShapeAPI::GetKernelShapeTriangleEdgeLengthAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->kernelShapeTriangleEdgeLength);
}

UsdAttribute
UsdLightFieldTriangleShapeAPI::CreateKernelShapeTriangleEdgeLengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->kernelShapeTriangleEdgeLength,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
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
UsdLightFieldTriangleShapeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->kernelShapeTriangleEdgeLength,
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
