//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/filteredPairsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsFilteredPairsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdPhysicsFilteredPairsAPI::~UsdPhysicsFilteredPairsAPI()
{
}

/* static */
UsdPhysicsFilteredPairsAPI
UsdPhysicsFilteredPairsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsFilteredPairsAPI();
    }
    return UsdPhysicsFilteredPairsAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdPhysicsFilteredPairsAPI::_GetSchemaKind() const
{
    return UsdPhysicsFilteredPairsAPI::schemaKind;
}

/* static */
bool
UsdPhysicsFilteredPairsAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdPhysicsFilteredPairsAPI>(whyNot);
}

/* static */
UsdPhysicsFilteredPairsAPI
UsdPhysicsFilteredPairsAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdPhysicsFilteredPairsAPI>()) {
        return UsdPhysicsFilteredPairsAPI(prim);
    }
    return UsdPhysicsFilteredPairsAPI();
}

/* static */
const TfType &
UsdPhysicsFilteredPairsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsFilteredPairsAPI>();
    return tfType;
}

/* static */
bool 
UsdPhysicsFilteredPairsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsFilteredPairsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdRelationship
UsdPhysicsFilteredPairsAPI::GetFilteredPairsRel() const
{
    return GetPrim().GetRelationship(UsdPhysicsTokens->physicsFilteredPairs);
}

UsdRelationship
UsdPhysicsFilteredPairsAPI::CreateFilteredPairsRel() const
{
    return GetPrim().CreateRelationship(UsdPhysicsTokens->physicsFilteredPairs,
                       /* custom = */ false);
}

/*static*/
const TfTokenVector&
UsdPhysicsFilteredPairsAPI::GetSchemaAttributeNames(bool includeInherited)
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
