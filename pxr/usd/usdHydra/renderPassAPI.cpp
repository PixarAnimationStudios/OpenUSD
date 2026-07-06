//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdHydra/renderPassAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdHydraRenderPassAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdHydraRenderPassAPI::~UsdHydraRenderPassAPI()
{
}

/* static */
UsdHydraRenderPassAPI
UsdHydraRenderPassAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdHydraRenderPassAPI();
    }
    return UsdHydraRenderPassAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdHydraRenderPassAPI::_GetSchemaKind() const
{
    return UsdHydraRenderPassAPI::schemaKind;
}

/* static */
bool
UsdHydraRenderPassAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdHydraRenderPassAPI>(whyNot);
}

/* static */
UsdHydraRenderPassAPI
UsdHydraRenderPassAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdHydraRenderPassAPI>()) {
        return UsdHydraRenderPassAPI(prim);
    }
    return UsdHydraRenderPassAPI();
}

/* static */
const TfType &
UsdHydraRenderPassAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdHydraRenderPassAPI>();
    return tfType;
}

/* static */
bool 
UsdHydraRenderPassAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdHydraRenderPassAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdHydraRenderPassAPI::GetHydraRendererNameAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->hydraRendererName);
}

UsdAttribute
UsdHydraRenderPassAPI::CreateHydraRendererNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->hydraRendererName,
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
UsdHydraRenderPassAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdHydraTokens->hydraRendererName,
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
