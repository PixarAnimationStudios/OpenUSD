//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldOpacityAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleFieldOpacityAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdVolParticleFieldOpacityAttributeAPI::~UsdVolParticleFieldOpacityAttributeAPI()
{
}

/* static */
UsdVolParticleFieldOpacityAttributeAPI
UsdVolParticleFieldOpacityAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleFieldOpacityAttributeAPI();
    }
    return UsdVolParticleFieldOpacityAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolParticleFieldOpacityAttributeAPI::_GetSchemaKind() const
{
    return UsdVolParticleFieldOpacityAttributeAPI::schemaKind;
}

/* static */
bool
UsdVolParticleFieldOpacityAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdVolParticleFieldOpacityAttributeAPI>(whyNot);
}

/* static */
UsdVolParticleFieldOpacityAttributeAPI
UsdVolParticleFieldOpacityAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdVolParticleFieldOpacityAttributeAPI>()) {
        return UsdVolParticleFieldOpacityAttributeAPI(prim);
    }
    return UsdVolParticleFieldOpacityAttributeAPI();
}

/* static */
const TfType &
UsdVolParticleFieldOpacityAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleFieldOpacityAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdVolParticleFieldOpacityAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleFieldOpacityAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolParticleFieldOpacityAttributeAPI::GetOpacitiesAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->opacities);
}

UsdAttribute
UsdVolParticleFieldOpacityAttributeAPI::CreateOpacitiesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->opacities,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolParticleFieldOpacityAttributeAPI::GetOpacitieshAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->opacitiesh);
}

UsdAttribute
UsdVolParticleFieldOpacityAttributeAPI::CreateOpacitieshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->opacitiesh,
                       SdfValueTypeNames->HalfArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdVolParticleFieldOpacityAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdVolTokens->opacities,
        UsdVolTokens->opacitiesh,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

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
