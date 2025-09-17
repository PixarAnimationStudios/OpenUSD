//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "./paramsAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdSchemaExamplesParamsAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdSchemaExamplesParamsAPI::~UsdSchemaExamplesParamsAPI()
{
}

/* static */
UsdSchemaExamplesParamsAPI
UsdSchemaExamplesParamsAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSchemaExamplesParamsAPI();
    }
    return UsdSchemaExamplesParamsAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdSchemaExamplesParamsAPI::_GetSchemaKind() const
{
    return UsdSchemaExamplesParamsAPI::schemaKind;
}

/* static */
bool
UsdSchemaExamplesParamsAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdSchemaExamplesParamsAPI>(whyNot);
}

/* static */
UsdSchemaExamplesParamsAPI
UsdSchemaExamplesParamsAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdSchemaExamplesParamsAPI>()) {
        return UsdSchemaExamplesParamsAPI(prim);
    }
    return UsdSchemaExamplesParamsAPI();
}

/* static */
const TfType &
UsdSchemaExamplesParamsAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdSchemaExamplesParamsAPI>();
    return tfType;
}

/* static */
bool 
UsdSchemaExamplesParamsAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdSchemaExamplesParamsAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdSchemaExamplesParamsAPI::GetMassAttr() const
{
    return GetPrim().GetAttribute(UsdSchemaExamplesTokens->paramsMass);
}

UsdAttribute
UsdSchemaExamplesParamsAPI::CreateMassAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSchemaExamplesTokens->paramsMass,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdSchemaExamplesParamsAPI::GetVelocityAttr() const
{
    return GetPrim().GetAttribute(UsdSchemaExamplesTokens->paramsVelocity);
}

UsdAttribute
UsdSchemaExamplesParamsAPI::CreateVelocityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSchemaExamplesTokens->paramsVelocity,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdSchemaExamplesParamsAPI::GetVolumeAttr() const
{
    return GetPrim().GetAttribute(UsdSchemaExamplesTokens->paramsVolume);
}

UsdAttribute
UsdSchemaExamplesParamsAPI::CreateVolumeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSchemaExamplesTokens->paramsVolume,
                       SdfValueTypeNames->Double,
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
UsdSchemaExamplesParamsAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdSchemaExamplesTokens->paramsMass,
        UsdSchemaExamplesTokens->paramsVelocity,
        UsdSchemaExamplesTokens->paramsVolume,
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
