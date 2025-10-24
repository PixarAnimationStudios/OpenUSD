//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/kernelConstantTriangleAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldKernelConstantTriangleAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldKernelConstantTriangleAPI::~UsdLightFieldKernelConstantTriangleAPI()
{
}

/* static */
UsdLightFieldKernelConstantTriangleAPI
UsdLightFieldKernelConstantTriangleAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldKernelConstantTriangleAPI();
    }
    return UsdLightFieldKernelConstantTriangleAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldKernelConstantTriangleAPI::_GetSchemaKind() const
{
    return UsdLightFieldKernelConstantTriangleAPI::schemaKind;
}

/* static */
bool
UsdLightFieldKernelConstantTriangleAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldKernelConstantTriangleAPI>(whyNot);
}

/* static */
UsdLightFieldKernelConstantTriangleAPI
UsdLightFieldKernelConstantTriangleAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldKernelConstantTriangleAPI>()) {
        return UsdLightFieldKernelConstantTriangleAPI(prim);
    }
    return UsdLightFieldKernelConstantTriangleAPI();
}

/* static */
const TfType &
UsdLightFieldKernelConstantTriangleAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldKernelConstantTriangleAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldKernelConstantTriangleAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldKernelConstantTriangleAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLightFieldKernelConstantTriangleAPI::GetSchemaAttributeNames(bool includeInherited)
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
UsdLightFieldKernelConstantTriangleAPI::LightFieldKernelBaseAPI() const
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
