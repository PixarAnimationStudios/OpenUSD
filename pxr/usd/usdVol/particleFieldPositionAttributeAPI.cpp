//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldPositionAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleFieldPositionAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdVolParticleFieldPositionAttributeAPI::~UsdVolParticleFieldPositionAttributeAPI()
{
}

/* static */
UsdVolParticleFieldPositionAttributeAPI
UsdVolParticleFieldPositionAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleFieldPositionAttributeAPI();
    }
    return UsdVolParticleFieldPositionAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolParticleFieldPositionAttributeAPI::_GetSchemaKind() const
{
    return UsdVolParticleFieldPositionAttributeAPI::schemaKind;
}

/* static */
bool
UsdVolParticleFieldPositionAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdVolParticleFieldPositionAttributeAPI>(whyNot);
}

/* static */
UsdVolParticleFieldPositionAttributeAPI
UsdVolParticleFieldPositionAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdVolParticleFieldPositionAttributeAPI>()) {
        return UsdVolParticleFieldPositionAttributeAPI(prim);
    }
    return UsdVolParticleFieldPositionAttributeAPI();
}

/* static */
const TfType &
UsdVolParticleFieldPositionAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleFieldPositionAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdVolParticleFieldPositionAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleFieldPositionAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolParticleFieldPositionAttributeAPI::GetPositionsAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->positions);
}

UsdAttribute
UsdVolParticleFieldPositionAttributeAPI::CreatePositionsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->positions,
                       SdfValueTypeNames->Point3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolParticleFieldPositionAttributeAPI::GetPositionshAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->positionsh);
}

UsdAttribute
UsdVolParticleFieldPositionAttributeAPI::CreatePositionshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->positionsh,
                       SdfValueTypeNames->Point3hArray,
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
UsdVolParticleFieldPositionAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdVolTokens->positions,
        UsdVolTokens->positionsh,
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

UsdVolParticleFieldPositionBaseAPI
UsdVolParticleFieldPositionAttributeAPI::ParticleFieldPositionBaseAPI() const
{
    return UsdVolParticleFieldPositionBaseAPI(GetPrim());
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
