//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/gaussianFalloffFunctionAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldGaussianFalloffFunctionAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldGaussianFalloffFunctionAPI::~UsdLightFieldGaussianFalloffFunctionAPI()
{
}

/* static */
UsdLightFieldGaussianFalloffFunctionAPI
UsdLightFieldGaussianFalloffFunctionAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldGaussianFalloffFunctionAPI();
    }
    return UsdLightFieldGaussianFalloffFunctionAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldGaussianFalloffFunctionAPI::_GetSchemaKind() const
{
    return UsdLightFieldGaussianFalloffFunctionAPI::schemaKind;
}

/* static */
bool
UsdLightFieldGaussianFalloffFunctionAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldGaussianFalloffFunctionAPI>(whyNot);
}

/* static */
UsdLightFieldGaussianFalloffFunctionAPI
UsdLightFieldGaussianFalloffFunctionAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldGaussianFalloffFunctionAPI>()) {
        return UsdLightFieldGaussianFalloffFunctionAPI(prim);
    }
    return UsdLightFieldGaussianFalloffFunctionAPI();
}

/* static */
const TfType &
UsdLightFieldGaussianFalloffFunctionAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldGaussianFalloffFunctionAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldGaussianFalloffFunctionAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldGaussianFalloffFunctionAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLightFieldGaussianFalloffFunctionAPI::GetSchemaAttributeNames(bool includeInherited)
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
