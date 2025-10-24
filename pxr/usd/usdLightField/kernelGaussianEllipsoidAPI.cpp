//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/kernelGaussianEllipsoidAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldKernelGaussianEllipsoidAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldKernelGaussianEllipsoidAPI::~UsdLightFieldKernelGaussianEllipsoidAPI()
{
}

/* static */
UsdLightFieldKernelGaussianEllipsoidAPI
UsdLightFieldKernelGaussianEllipsoidAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldKernelGaussianEllipsoidAPI();
    }
    return UsdLightFieldKernelGaussianEllipsoidAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldKernelGaussianEllipsoidAPI::_GetSchemaKind() const
{
    return UsdLightFieldKernelGaussianEllipsoidAPI::schemaKind;
}

/* static */
bool
UsdLightFieldKernelGaussianEllipsoidAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldKernelGaussianEllipsoidAPI>(whyNot);
}

/* static */
UsdLightFieldKernelGaussianEllipsoidAPI
UsdLightFieldKernelGaussianEllipsoidAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldKernelGaussianEllipsoidAPI>()) {
        return UsdLightFieldKernelGaussianEllipsoidAPI(prim);
    }
    return UsdLightFieldKernelGaussianEllipsoidAPI();
}

/* static */
const TfType &
UsdLightFieldKernelGaussianEllipsoidAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldKernelGaussianEllipsoidAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldKernelGaussianEllipsoidAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldKernelGaussianEllipsoidAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLightFieldKernelGaussianEllipsoidAPI::GetSchemaAttributeNames(bool includeInherited)
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
UsdLightFieldKernelGaussianEllipsoidAPI::LightFieldKernelBaseAPI() const
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
