//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/gaussianShapeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldGaussianShapeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldGaussianShapeAPI::~UsdLightFieldGaussianShapeAPI()
{
}

/* static */
UsdLightFieldGaussianShapeAPI
UsdLightFieldGaussianShapeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldGaussianShapeAPI();
    }
    return UsdLightFieldGaussianShapeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldGaussianShapeAPI::_GetSchemaKind() const
{
    return UsdLightFieldGaussianShapeAPI::schemaKind;
}

/* static */
bool
UsdLightFieldGaussianShapeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldGaussianShapeAPI>(whyNot);
}

/* static */
UsdLightFieldGaussianShapeAPI
UsdLightFieldGaussianShapeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldGaussianShapeAPI>()) {
        return UsdLightFieldGaussianShapeAPI(prim);
    }
    return UsdLightFieldGaussianShapeAPI();
}

/* static */
const TfType &
UsdLightFieldGaussianShapeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldGaussianShapeAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldGaussianShapeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldGaussianShapeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLightFieldGaussianShapeAPI::GetSchemaAttributeNames(bool includeInherited)
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
