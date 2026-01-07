//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldOrientationAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleFieldOrientationAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdVolParticleFieldOrientationAttributeAPI::~UsdVolParticleFieldOrientationAttributeAPI()
{
}

/* static */
UsdVolParticleFieldOrientationAttributeAPI
UsdVolParticleFieldOrientationAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleFieldOrientationAttributeAPI();
    }
    return UsdVolParticleFieldOrientationAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolParticleFieldOrientationAttributeAPI::_GetSchemaKind() const
{
    return UsdVolParticleFieldOrientationAttributeAPI::schemaKind;
}

/* static */
bool
UsdVolParticleFieldOrientationAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdVolParticleFieldOrientationAttributeAPI>(whyNot);
}

/* static */
UsdVolParticleFieldOrientationAttributeAPI
UsdVolParticleFieldOrientationAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdVolParticleFieldOrientationAttributeAPI>()) {
        return UsdVolParticleFieldOrientationAttributeAPI(prim);
    }
    return UsdVolParticleFieldOrientationAttributeAPI();
}

/* static */
const TfType &
UsdVolParticleFieldOrientationAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleFieldOrientationAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdVolParticleFieldOrientationAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleFieldOrientationAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolParticleFieldOrientationAttributeAPI::GetOrientationsAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->orientations);
}

UsdAttribute
UsdVolParticleFieldOrientationAttributeAPI::CreateOrientationsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->orientations,
                       SdfValueTypeNames->QuatfArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolParticleFieldOrientationAttributeAPI::GetOrientationshAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->orientationsh);
}

UsdAttribute
UsdVolParticleFieldOrientationAttributeAPI::CreateOrientationshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->orientationsh,
                       SdfValueTypeNames->QuathArray,
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
UsdVolParticleFieldOrientationAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdVolTokens->orientations,
        UsdVolTokens->orientationsh,
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
