//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldKernelConstantSurfletAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleFieldKernelConstantSurfletAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdVolParticleFieldKernelConstantSurfletAPI::~UsdVolParticleFieldKernelConstantSurfletAPI()
{
}

/* static */
UsdVolParticleFieldKernelConstantSurfletAPI
UsdVolParticleFieldKernelConstantSurfletAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleFieldKernelConstantSurfletAPI();
    }
    return UsdVolParticleFieldKernelConstantSurfletAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolParticleFieldKernelConstantSurfletAPI::_GetSchemaKind() const
{
    return UsdVolParticleFieldKernelConstantSurfletAPI::schemaKind;
}

/* static */
bool
UsdVolParticleFieldKernelConstantSurfletAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdVolParticleFieldKernelConstantSurfletAPI>(whyNot);
}

/* static */
UsdVolParticleFieldKernelConstantSurfletAPI
UsdVolParticleFieldKernelConstantSurfletAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdVolParticleFieldKernelConstantSurfletAPI>()) {
        return UsdVolParticleFieldKernelConstantSurfletAPI(prim);
    }
    return UsdVolParticleFieldKernelConstantSurfletAPI();
}

/* static */
const TfType &
UsdVolParticleFieldKernelConstantSurfletAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleFieldKernelConstantSurfletAPI>();
    return tfType;
}

/* static */
bool 
UsdVolParticleFieldKernelConstantSurfletAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleFieldKernelConstantSurfletAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdVolParticleFieldKernelConstantSurfletAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

UsdVolParticleFieldKernelBaseAPI
UsdVolParticleFieldKernelConstantSurfletAPI::ParticleFieldKernelBaseAPI() const
{
    return UsdVolParticleFieldKernelBaseAPI(GetPrim());
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
