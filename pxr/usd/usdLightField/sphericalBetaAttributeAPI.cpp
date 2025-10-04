//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/sphericalBetaAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldSphericalBetaAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldSphericalBetaAttributeAPI::~UsdLightFieldSphericalBetaAttributeAPI()
{
}

/* static */
UsdLightFieldSphericalBetaAttributeAPI
UsdLightFieldSphericalBetaAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldSphericalBetaAttributeAPI();
    }
    return UsdLightFieldSphericalBetaAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldSphericalBetaAttributeAPI::_GetSchemaKind() const
{
    return UsdLightFieldSphericalBetaAttributeAPI::schemaKind;
}

/* static */
bool
UsdLightFieldSphericalBetaAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldSphericalBetaAttributeAPI>(whyNot);
}

/* static */
UsdLightFieldSphericalBetaAttributeAPI
UsdLightFieldSphericalBetaAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldSphericalBetaAttributeAPI>()) {
        return UsdLightFieldSphericalBetaAttributeAPI(prim);
    }
    return UsdLightFieldSphericalBetaAttributeAPI();
}

/* static */
const TfType &
UsdLightFieldSphericalBetaAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldSphericalBetaAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldSphericalBetaAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldSphericalBetaAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldSphericalBetaAttributeAPI::GetPrimvarsSphericalBetaBetaAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->primvarsSphericalBetaBeta);
}

UsdAttribute
UsdLightFieldSphericalBetaAttributeAPI::CreatePrimvarsSphericalBetaBetaAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->primvarsSphericalBetaBeta,
                       SdfValueTypeNames->FloatArray,
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
UsdLightFieldSphericalBetaAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->primvarsSphericalBetaBeta,
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
