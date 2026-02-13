//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldKernelGaussianSurfletAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleFieldKernelGaussianSurfletAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdVolParticleFieldKernelGaussianSurfletAPI::~UsdVolParticleFieldKernelGaussianSurfletAPI()
{
}

/* static */
UsdVolParticleFieldKernelGaussianSurfletAPI
UsdVolParticleFieldKernelGaussianSurfletAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleFieldKernelGaussianSurfletAPI();
    }
    return UsdVolParticleFieldKernelGaussianSurfletAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolParticleFieldKernelGaussianSurfletAPI::_GetSchemaKind() const
{
    return UsdVolParticleFieldKernelGaussianSurfletAPI::schemaKind;
}

/* static */
bool
UsdVolParticleFieldKernelGaussianSurfletAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdVolParticleFieldKernelGaussianSurfletAPI>(whyNot);
}

/* static */
UsdVolParticleFieldKernelGaussianSurfletAPI
UsdVolParticleFieldKernelGaussianSurfletAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdVolParticleFieldKernelGaussianSurfletAPI>()) {
        return UsdVolParticleFieldKernelGaussianSurfletAPI(prim);
    }
    return UsdVolParticleFieldKernelGaussianSurfletAPI();
}

/* static */
const TfType &
UsdVolParticleFieldKernelGaussianSurfletAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleFieldKernelGaussianSurfletAPI>();
    return tfType;
}

/* static */
bool 
UsdVolParticleFieldKernelGaussianSurfletAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleFieldKernelGaussianSurfletAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdVolParticleFieldKernelGaussianSurfletAPI::GetSchemaAttributeNames(bool includeInherited)
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
UsdVolParticleFieldKernelGaussianSurfletAPI::ParticleFieldKernelBaseAPI() const
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
