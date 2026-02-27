//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldPositionBaseAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleFieldPositionBaseAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdVolParticleFieldPositionBaseAPI::~UsdVolParticleFieldPositionBaseAPI()
{
}

/* static */
UsdVolParticleFieldPositionBaseAPI
UsdVolParticleFieldPositionBaseAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleFieldPositionBaseAPI();
    }
    return UsdVolParticleFieldPositionBaseAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolParticleFieldPositionBaseAPI::_GetSchemaKind() const
{
    return UsdVolParticleFieldPositionBaseAPI::schemaKind;
}

/* static */
bool
UsdVolParticleFieldPositionBaseAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdVolParticleFieldPositionBaseAPI>(whyNot);
}

/* static */
UsdVolParticleFieldPositionBaseAPI
UsdVolParticleFieldPositionBaseAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdVolParticleFieldPositionBaseAPI>()) {
        return UsdVolParticleFieldPositionBaseAPI(prim);
    }
    return UsdVolParticleFieldPositionBaseAPI();
}

/* static */
const TfType &
UsdVolParticleFieldPositionBaseAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleFieldPositionBaseAPI>();
    return tfType;
}

/* static */
bool 
UsdVolParticleFieldPositionBaseAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleFieldPositionBaseAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdVolParticleFieldPositionBaseAPI::GetSchemaAttributeNames(bool includeInherited)
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
