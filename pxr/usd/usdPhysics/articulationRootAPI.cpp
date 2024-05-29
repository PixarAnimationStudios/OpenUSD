//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/articulationRootAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsArticulationRootAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdPhysicsArticulationRootAPI::~UsdPhysicsArticulationRootAPI()
{
}

/* static */
UsdPhysicsArticulationRootAPI
UsdPhysicsArticulationRootAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsArticulationRootAPI();
    }
    return UsdPhysicsArticulationRootAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdPhysicsArticulationRootAPI::_GetSchemaKind() const
{
    return UsdPhysicsArticulationRootAPI::schemaKind;
}

/* static */
bool
UsdPhysicsArticulationRootAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdPhysicsArticulationRootAPI>(whyNot);
}

/* static */
UsdPhysicsArticulationRootAPI
UsdPhysicsArticulationRootAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdPhysicsArticulationRootAPI>()) {
        return UsdPhysicsArticulationRootAPI(prim);
    }
    return UsdPhysicsArticulationRootAPI();
}

/* static */
const TfType &
UsdPhysicsArticulationRootAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsArticulationRootAPI>();
    return tfType;
}

/* static */
bool 
UsdPhysicsArticulationRootAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsArticulationRootAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdPhysicsArticulationRootAPI::GetSchemaAttributeNames(bool includeInherited)
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
