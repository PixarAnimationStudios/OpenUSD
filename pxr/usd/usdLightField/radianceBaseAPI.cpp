//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/radianceBaseAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldRadianceBaseAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldRadianceBaseAPI::~UsdLightFieldRadianceBaseAPI()
{
}

/* static */
UsdLightFieldRadianceBaseAPI
UsdLightFieldRadianceBaseAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldRadianceBaseAPI();
    }
    return UsdLightFieldRadianceBaseAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldRadianceBaseAPI::_GetSchemaKind() const
{
    return UsdLightFieldRadianceBaseAPI::schemaKind;
}

/* static */
bool
UsdLightFieldRadianceBaseAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldRadianceBaseAPI>(whyNot);
}

/* static */
UsdLightFieldRadianceBaseAPI
UsdLightFieldRadianceBaseAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldRadianceBaseAPI>()) {
        return UsdLightFieldRadianceBaseAPI(prim);
    }
    return UsdLightFieldRadianceBaseAPI();
}

/* static */
const TfType &
UsdLightFieldRadianceBaseAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldRadianceBaseAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldRadianceBaseAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldRadianceBaseAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLightFieldRadianceBaseAPI::GetSchemaAttributeNames(bool includeInherited)
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
