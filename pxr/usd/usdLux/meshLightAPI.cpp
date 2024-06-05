//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/meshLightAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxMeshLightAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLuxMeshLightAPI::~UsdLuxMeshLightAPI()
{
}

/* static */
UsdLuxMeshLightAPI
UsdLuxMeshLightAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxMeshLightAPI();
    }
    return UsdLuxMeshLightAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLuxMeshLightAPI::_GetSchemaKind() const
{
    return UsdLuxMeshLightAPI::schemaKind;
}

/* static */
bool
UsdLuxMeshLightAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLuxMeshLightAPI>(whyNot);
}

/* static */
UsdLuxMeshLightAPI
UsdLuxMeshLightAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLuxMeshLightAPI>()) {
        return UsdLuxMeshLightAPI(prim);
    }
    return UsdLuxMeshLightAPI();
}

/* static */
const TfType &
UsdLuxMeshLightAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxMeshLightAPI>();
    return tfType;
}

/* static */
bool 
UsdLuxMeshLightAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxMeshLightAPI::_GetTfType() const
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
UsdLuxMeshLightAPI::GetSchemaAttributeNames(bool includeInherited)
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
