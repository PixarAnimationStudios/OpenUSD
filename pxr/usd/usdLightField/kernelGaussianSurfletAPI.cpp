//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/kernelGaussianSurfletAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldKernelGaussianSurfletAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldKernelGaussianSurfletAPI::~UsdLightFieldKernelGaussianSurfletAPI()
{
}

/* static */
UsdLightFieldKernelGaussianSurfletAPI
UsdLightFieldKernelGaussianSurfletAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldKernelGaussianSurfletAPI();
    }
    return UsdLightFieldKernelGaussianSurfletAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldKernelGaussianSurfletAPI::_GetSchemaKind() const
{
    return UsdLightFieldKernelGaussianSurfletAPI::schemaKind;
}

/* static */
bool
UsdLightFieldKernelGaussianSurfletAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldKernelGaussianSurfletAPI>(whyNot);
}

/* static */
UsdLightFieldKernelGaussianSurfletAPI
UsdLightFieldKernelGaussianSurfletAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldKernelGaussianSurfletAPI>()) {
        return UsdLightFieldKernelGaussianSurfletAPI(prim);
    }
    return UsdLightFieldKernelGaussianSurfletAPI();
}

/* static */
const TfType &
UsdLightFieldKernelGaussianSurfletAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldKernelGaussianSurfletAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldKernelGaussianSurfletAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldKernelGaussianSurfletAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLightFieldKernelGaussianSurfletAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

UsdLightFieldKernelBaseAPI
UsdLightFieldKernelGaussianSurfletAPI::LightFieldKernelBaseAPI() const
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
