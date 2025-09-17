//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdUI/sceneGraphPrimAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdUISceneGraphPrimAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdUISceneGraphPrimAPI::~UsdUISceneGraphPrimAPI()
{
}

/* static */
UsdUISceneGraphPrimAPI
UsdUISceneGraphPrimAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdUISceneGraphPrimAPI();
    }
    return UsdUISceneGraphPrimAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdUISceneGraphPrimAPI::_GetSchemaKind() const
{
    return UsdUISceneGraphPrimAPI::schemaKind;
}

/* static */
bool
UsdUISceneGraphPrimAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdUISceneGraphPrimAPI>(whyNot);
}

/* static */
UsdUISceneGraphPrimAPI
UsdUISceneGraphPrimAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdUISceneGraphPrimAPI>()) {
        return UsdUISceneGraphPrimAPI(prim);
    }
    return UsdUISceneGraphPrimAPI();
}

/* static */
const TfType &
UsdUISceneGraphPrimAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdUISceneGraphPrimAPI>();
    return tfType;
}

/* static */
bool 
UsdUISceneGraphPrimAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdUISceneGraphPrimAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdUISceneGraphPrimAPI::GetDisplayNameAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiDisplayName);
}

UsdAttribute
UsdUISceneGraphPrimAPI::CreateDisplayNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiDisplayName,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdUISceneGraphPrimAPI::GetDisplayGroupAttr() const
{
    return GetPrim().GetAttribute(UsdUITokens->uiDisplayGroup);
}

UsdAttribute
UsdUISceneGraphPrimAPI::CreateDisplayGroupAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdUITokens->uiDisplayGroup,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
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
UsdUISceneGraphPrimAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdUITokens->uiDisplayName,
        UsdUITokens->uiDisplayGroup,
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
