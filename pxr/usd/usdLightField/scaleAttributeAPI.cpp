//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/scaleAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldScaleAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldScaleAttributeAPI::~UsdLightFieldScaleAttributeAPI()
{
}

/* static */
UsdLightFieldScaleAttributeAPI
UsdLightFieldScaleAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldScaleAttributeAPI();
    }
    return UsdLightFieldScaleAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldScaleAttributeAPI::_GetSchemaKind() const
{
    return UsdLightFieldScaleAttributeAPI::schemaKind;
}

/* static */
bool
UsdLightFieldScaleAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldScaleAttributeAPI>(whyNot);
}

/* static */
UsdLightFieldScaleAttributeAPI
UsdLightFieldScaleAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldScaleAttributeAPI>()) {
        return UsdLightFieldScaleAttributeAPI(prim);
    }
    return UsdLightFieldScaleAttributeAPI();
}

/* static */
const TfType &
UsdLightFieldScaleAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldScaleAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldScaleAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldScaleAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldScaleAttributeAPI::GetScalesAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->scales);
}

UsdAttribute
UsdLightFieldScaleAttributeAPI::CreateScalesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->scales,
                       SdfValueTypeNames->Float3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldScaleAttributeAPI::GetScaleshAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->scalesh);
}

UsdAttribute
UsdLightFieldScaleAttributeAPI::CreateScaleshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->scalesh,
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
UsdLightFieldScaleAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->scales,
        UsdLightFieldTokens->scalesh,
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
