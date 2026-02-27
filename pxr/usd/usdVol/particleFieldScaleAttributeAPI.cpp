//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleFieldScaleAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleFieldScaleAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdVolParticleFieldScaleAttributeAPI::~UsdVolParticleFieldScaleAttributeAPI()
{
}

/* static */
UsdVolParticleFieldScaleAttributeAPI
UsdVolParticleFieldScaleAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleFieldScaleAttributeAPI();
    }
    return UsdVolParticleFieldScaleAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolParticleFieldScaleAttributeAPI::_GetSchemaKind() const
{
    return UsdVolParticleFieldScaleAttributeAPI::schemaKind;
}

/* static */
bool
UsdVolParticleFieldScaleAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdVolParticleFieldScaleAttributeAPI>(whyNot);
}

/* static */
UsdVolParticleFieldScaleAttributeAPI
UsdVolParticleFieldScaleAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdVolParticleFieldScaleAttributeAPI>()) {
        return UsdVolParticleFieldScaleAttributeAPI(prim);
    }
    return UsdVolParticleFieldScaleAttributeAPI();
}

/* static */
const TfType &
UsdVolParticleFieldScaleAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleFieldScaleAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdVolParticleFieldScaleAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleFieldScaleAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdVolParticleFieldScaleAttributeAPI::GetScalesAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->scales);
}

UsdAttribute
UsdVolParticleFieldScaleAttributeAPI::CreateScalesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->scales,
                       SdfValueTypeNames->Float3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdVolParticleFieldScaleAttributeAPI::GetScaleshAttr() const
{
    return GetPrim().GetAttribute(UsdVolTokens->scalesh);
}

UsdAttribute
UsdVolParticleFieldScaleAttributeAPI::CreateScaleshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdVolTokens->scalesh,
                       SdfValueTypeNames->Half3Array,
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
UsdVolParticleFieldScaleAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdVolTokens->scales,
        UsdVolTokens->scalesh,
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
