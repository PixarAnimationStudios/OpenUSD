//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/kernelBaseAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldKernelBaseAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldKernelBaseAPI::~UsdLightFieldKernelBaseAPI()
{
}

/* static */
UsdLightFieldKernelBaseAPI
UsdLightFieldKernelBaseAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldKernelBaseAPI();
    }
    return UsdLightFieldKernelBaseAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldKernelBaseAPI::_GetSchemaKind() const
{
    return UsdLightFieldKernelBaseAPI::schemaKind;
}

/* static */
bool
UsdLightFieldKernelBaseAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldKernelBaseAPI>(whyNot);
}

/* static */
UsdLightFieldKernelBaseAPI
UsdLightFieldKernelBaseAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldKernelBaseAPI>()) {
        return UsdLightFieldKernelBaseAPI(prim);
    }
    return UsdLightFieldKernelBaseAPI();
}

/* static */
const TfType &
UsdLightFieldKernelBaseAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldKernelBaseAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldKernelBaseAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldKernelBaseAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLightFieldKernelBaseAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

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
