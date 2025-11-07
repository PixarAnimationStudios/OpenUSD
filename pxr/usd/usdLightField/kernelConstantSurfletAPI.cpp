//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/kernelConstantSurfletAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldKernelConstantSurfletAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldKernelConstantSurfletAPI::~UsdLightFieldKernelConstantSurfletAPI()
{
}

/* static */
UsdLightFieldKernelConstantSurfletAPI
UsdLightFieldKernelConstantSurfletAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldKernelConstantSurfletAPI();
    }
    return UsdLightFieldKernelConstantSurfletAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldKernelConstantSurfletAPI::_GetSchemaKind() const
{
    return UsdLightFieldKernelConstantSurfletAPI::schemaKind;
}

/* static */
bool
UsdLightFieldKernelConstantSurfletAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldKernelConstantSurfletAPI>(whyNot);
}

/* static */
UsdLightFieldKernelConstantSurfletAPI
UsdLightFieldKernelConstantSurfletAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldKernelConstantSurfletAPI>()) {
        return UsdLightFieldKernelConstantSurfletAPI(prim);
    }
    return UsdLightFieldKernelConstantSurfletAPI();
}

/* static */
const TfType &
UsdLightFieldKernelConstantSurfletAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldKernelConstantSurfletAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldKernelConstantSurfletAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldKernelConstantSurfletAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLightFieldKernelConstantSurfletAPI::GetSchemaAttributeNames(bool includeInherited)
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
UsdLightFieldKernelConstantSurfletAPI::LightFieldKernelBaseAPI() const
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
