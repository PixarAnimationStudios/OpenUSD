//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/derivedNonAppliedAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedDerivedNonAppliedAPI,
        TfType::Bases< UsdContrivedNonAppliedAPI > >();
    
}

/* virtual */
UsdContrivedDerivedNonAppliedAPI::~UsdContrivedDerivedNonAppliedAPI()
{
}

/* static */
UsdContrivedDerivedNonAppliedAPI
UsdContrivedDerivedNonAppliedAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedDerivedNonAppliedAPI();
    }
    return UsdContrivedDerivedNonAppliedAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdContrivedDerivedNonAppliedAPI::_GetSchemaKind() const
{
    return UsdContrivedDerivedNonAppliedAPI::schemaKind;
}

/* static */
const TfType &
UsdContrivedDerivedNonAppliedAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedDerivedNonAppliedAPI>();
    return tfType;
}

/* static */
bool 
UsdContrivedDerivedNonAppliedAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedDerivedNonAppliedAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdContrivedDerivedNonAppliedAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdContrivedNonAppliedAPI::GetSchemaAttributeNames(true);

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
