//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMtlx/materialXConfigAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdMtlxMaterialXConfigAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdMtlxMaterialXConfigAPI::~UsdMtlxMaterialXConfigAPI()
{
}

/* static */
UsdMtlxMaterialXConfigAPI
UsdMtlxMaterialXConfigAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdMtlxMaterialXConfigAPI();
    }
    return UsdMtlxMaterialXConfigAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdMtlxMaterialXConfigAPI::_GetSchemaKind() const
{
    return UsdMtlxMaterialXConfigAPI::schemaKind;
}

/* static */
bool
UsdMtlxMaterialXConfigAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdMtlxMaterialXConfigAPI>(whyNot);
}

/* static */
UsdMtlxMaterialXConfigAPI
UsdMtlxMaterialXConfigAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdMtlxMaterialXConfigAPI>()) {
        return UsdMtlxMaterialXConfigAPI(prim);
    }
    return UsdMtlxMaterialXConfigAPI();
}

/* static */
const TfType &
UsdMtlxMaterialXConfigAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdMtlxMaterialXConfigAPI>();
    return tfType;
}

/* static */
bool 
UsdMtlxMaterialXConfigAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdMtlxMaterialXConfigAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdMtlxMaterialXConfigAPI::GetConfigMtlxVersionAttr() const
{
    return GetPrim().GetAttribute(UsdMtlxTokens->configMtlxVersion);
}

UsdAttribute
UsdMtlxMaterialXConfigAPI::CreateConfigMtlxVersionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdMtlxTokens->configMtlxVersion,
                       SdfValueTypeNames->String,
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
UsdMtlxMaterialXConfigAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdMtlxTokens->configMtlxVersion,
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
