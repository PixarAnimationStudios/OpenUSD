//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/singleApplyAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedSingleApplyAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdContrivedSingleApplyAPI::~UsdContrivedSingleApplyAPI()
{
}

/* static */
UsdContrivedSingleApplyAPI
UsdContrivedSingleApplyAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedSingleApplyAPI();
    }
    return UsdContrivedSingleApplyAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdContrivedSingleApplyAPI::_GetSchemaKind() const
{
    return UsdContrivedSingleApplyAPI::schemaKind;
}

/* static */
bool
UsdContrivedSingleApplyAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdContrivedSingleApplyAPI>(whyNot);
}

/* static */
UsdContrivedSingleApplyAPI
UsdContrivedSingleApplyAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdContrivedSingleApplyAPI>()) {
        return UsdContrivedSingleApplyAPI(prim);
    }
    return UsdContrivedSingleApplyAPI();
}

/* static */
const TfType &
UsdContrivedSingleApplyAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedSingleApplyAPI>();
    return tfType;
}

/* static */
bool 
UsdContrivedSingleApplyAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedSingleApplyAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdContrivedSingleApplyAPI::GetSchemaAttributeNames(bool includeInherited)
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
