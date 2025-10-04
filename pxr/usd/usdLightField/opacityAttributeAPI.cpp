//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/opacityAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldOpacityAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldOpacityAttributeAPI::~UsdLightFieldOpacityAttributeAPI()
{
}

/* static */
UsdLightFieldOpacityAttributeAPI
UsdLightFieldOpacityAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldOpacityAttributeAPI();
    }
    return UsdLightFieldOpacityAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldOpacityAttributeAPI::_GetSchemaKind() const
{
    return UsdLightFieldOpacityAttributeAPI::schemaKind;
}

/* static */
bool
UsdLightFieldOpacityAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldOpacityAttributeAPI>(whyNot);
}

/* static */
UsdLightFieldOpacityAttributeAPI
UsdLightFieldOpacityAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldOpacityAttributeAPI>()) {
        return UsdLightFieldOpacityAttributeAPI(prim);
    }
    return UsdLightFieldOpacityAttributeAPI();
}

/* static */
const TfType &
UsdLightFieldOpacityAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldOpacityAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldOpacityAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldOpacityAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldOpacityAttributeAPI::GetOpacitiesAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->opacities);
}

UsdAttribute
UsdLightFieldOpacityAttributeAPI::CreateOpacitiesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->opacities,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldOpacityAttributeAPI::GetOpacitieshAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->opacitiesh);
}

UsdAttribute
UsdLightFieldOpacityAttributeAPI::CreateOpacitieshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->opacitiesh,
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
UsdLightFieldOpacityAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->opacities,
        UsdLightFieldTokens->opacitiesh,
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
