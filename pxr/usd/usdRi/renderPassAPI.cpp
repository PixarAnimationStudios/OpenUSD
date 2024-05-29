//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRi/renderPassAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiRenderPassAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdRiRenderPassAPI::~UsdRiRenderPassAPI()
{
}

/* static */
UsdRiRenderPassAPI
UsdRiRenderPassAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiRenderPassAPI();
    }
    return UsdRiRenderPassAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdRiRenderPassAPI::_GetSchemaKind() const
{
    return UsdRiRenderPassAPI::schemaKind;
}

/* static */
bool
UsdRiRenderPassAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdRiRenderPassAPI>(whyNot);
}

/* static */
UsdRiRenderPassAPI
UsdRiRenderPassAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdRiRenderPassAPI>()) {
        return UsdRiRenderPassAPI(prim);
    }
    return UsdRiRenderPassAPI();
}

/* static */
const TfType &
UsdRiRenderPassAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiRenderPassAPI>();
    return tfType;
}

/* static */
bool 
UsdRiRenderPassAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiRenderPassAPI::_GetTfType() const
{
    return _GetStaticTfType();
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
UsdRiRenderPassAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
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
PXR_NAMESPACE_OPEN_SCOPE

UsdCollectionAPI
UsdRiRenderPassAPI::GetCameraVisibilityCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdRiTokens->cameraVisibility);
}

UsdCollectionAPI
UsdRiRenderPassAPI::GetMatteCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdRiTokens->matte);
}

PXR_NAMESPACE_CLOSE_SCOPE
