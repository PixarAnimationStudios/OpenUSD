//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/positionBaseAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldPositionBaseAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldPositionBaseAPI::~UsdLightFieldPositionBaseAPI()
{
}

/* static */
UsdLightFieldPositionBaseAPI
UsdLightFieldPositionBaseAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldPositionBaseAPI();
    }
    return UsdLightFieldPositionBaseAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldPositionBaseAPI::_GetSchemaKind() const
{
    return UsdLightFieldPositionBaseAPI::schemaKind;
}

/* static */
bool
UsdLightFieldPositionBaseAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldPositionBaseAPI>(whyNot);
}

/* static */
UsdLightFieldPositionBaseAPI
UsdLightFieldPositionBaseAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldPositionBaseAPI>()) {
        return UsdLightFieldPositionBaseAPI(prim);
    }
    return UsdLightFieldPositionBaseAPI();
}

/* static */
const TfType &
UsdLightFieldPositionBaseAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldPositionBaseAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldPositionBaseAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldPositionBaseAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLightFieldPositionBaseAPI::GetSchemaAttributeNames(bool includeInherited)
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
