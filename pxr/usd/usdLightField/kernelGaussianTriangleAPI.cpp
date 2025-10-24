//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/kernelGaussianTriangleAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldKernelGaussianTriangleAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldKernelGaussianTriangleAPI::~UsdLightFieldKernelGaussianTriangleAPI()
{
}

/* static */
UsdLightFieldKernelGaussianTriangleAPI
UsdLightFieldKernelGaussianTriangleAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldKernelGaussianTriangleAPI();
    }
    return UsdLightFieldKernelGaussianTriangleAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldKernelGaussianTriangleAPI::_GetSchemaKind() const
{
    return UsdLightFieldKernelGaussianTriangleAPI::schemaKind;
}

/* static */
bool
UsdLightFieldKernelGaussianTriangleAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldKernelGaussianTriangleAPI>(whyNot);
}

/* static */
UsdLightFieldKernelGaussianTriangleAPI
UsdLightFieldKernelGaussianTriangleAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldKernelGaussianTriangleAPI>()) {
        return UsdLightFieldKernelGaussianTriangleAPI(prim);
    }
    return UsdLightFieldKernelGaussianTriangleAPI();
}

/* static */
const TfType &
UsdLightFieldKernelGaussianTriangleAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldKernelGaussianTriangleAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldKernelGaussianTriangleAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldKernelGaussianTriangleAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldKernelGaussianTriangleAPI::GetKernelTriangleEdgeLengthAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->kernelTriangleEdgeLength);
}

UsdAttribute
UsdLightFieldKernelGaussianTriangleAPI::CreateKernelTriangleEdgeLengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->kernelTriangleEdgeLength,
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
UsdLightFieldKernelGaussianTriangleAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->kernelTriangleEdgeLength,
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

UsdLightFieldKernelBaseAPI
UsdLightFieldKernelGaussianTriangleAPI::LightFieldKernelBaseAPI() const
{
    return UsdLightFieldKernelBaseAPI(GetPrim());
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
