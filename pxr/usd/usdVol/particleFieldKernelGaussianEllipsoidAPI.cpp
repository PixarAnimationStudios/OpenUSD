//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldKernelGaussianEllipsoidAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleFieldKernelGaussianEllipsoidAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdVolParticleFieldKernelGaussianEllipsoidAPI::~UsdVolParticleFieldKernelGaussianEllipsoidAPI()
{
}

/* static */
UsdVolParticleFieldKernelGaussianEllipsoidAPI
UsdVolParticleFieldKernelGaussianEllipsoidAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleFieldKernelGaussianEllipsoidAPI();
    }
    return UsdVolParticleFieldKernelGaussianEllipsoidAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolParticleFieldKernelGaussianEllipsoidAPI::_GetSchemaKind() const
{
    return UsdVolParticleFieldKernelGaussianEllipsoidAPI::schemaKind;
}

/* static */
bool
UsdVolParticleFieldKernelGaussianEllipsoidAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdVolParticleFieldKernelGaussianEllipsoidAPI>(whyNot);
}

/* static */
UsdVolParticleFieldKernelGaussianEllipsoidAPI
UsdVolParticleFieldKernelGaussianEllipsoidAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdVolParticleFieldKernelGaussianEllipsoidAPI>()) {
        return UsdVolParticleFieldKernelGaussianEllipsoidAPI(prim);
    }
    return UsdVolParticleFieldKernelGaussianEllipsoidAPI();
}

/* static */
const TfType &
UsdVolParticleFieldKernelGaussianEllipsoidAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleFieldKernelGaussianEllipsoidAPI>();
    return tfType;
}

/* static */
bool 
UsdVolParticleFieldKernelGaussianEllipsoidAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleFieldKernelGaussianEllipsoidAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdVolParticleFieldKernelGaussianEllipsoidAPI::GetSchemaAttributeNames(bool includeInherited)
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
UsdVolParticleFieldKernelGaussianEllipsoidAPI::ParticleFieldKernelBaseAPI() const
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
