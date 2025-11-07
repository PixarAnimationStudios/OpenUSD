//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/positionAttributeAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldPositionAttributeAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldPositionAttributeAPI::~UsdLightFieldPositionAttributeAPI()
{
}

/* static */
UsdLightFieldPositionAttributeAPI
UsdLightFieldPositionAttributeAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldPositionAttributeAPI();
    }
    return UsdLightFieldPositionAttributeAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldPositionAttributeAPI::_GetSchemaKind() const
{
    return UsdLightFieldPositionAttributeAPI::schemaKind;
}

/* static */
bool
UsdLightFieldPositionAttributeAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldPositionAttributeAPI>(whyNot);
}

/* static */
UsdLightFieldPositionAttributeAPI
UsdLightFieldPositionAttributeAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldPositionAttributeAPI>()) {
        return UsdLightFieldPositionAttributeAPI(prim);
    }
    return UsdLightFieldPositionAttributeAPI();
}

/* static */
const TfType &
UsdLightFieldPositionAttributeAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldPositionAttributeAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldPositionAttributeAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldPositionAttributeAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldPositionAttributeAPI::GetPositionsAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->positions);
}

UsdAttribute
UsdLightFieldPositionAttributeAPI::CreatePositionsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->positions,
                       SdfValueTypeNames->Point3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLightFieldPositionAttributeAPI::GetPositionshAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->positionsh);
}

UsdAttribute
UsdLightFieldPositionAttributeAPI::CreatePositionshAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->positionsh,
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
UsdLightFieldPositionAttributeAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->positions,
        UsdLightFieldTokens->positionsh,
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

UsdLightFieldPositionBaseAPI
UsdLightFieldPositionAttributeAPI::LightFieldPositionBaseAPI() const
{
    return UsdLightFieldPositionBaseAPI(GetPrim());
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
