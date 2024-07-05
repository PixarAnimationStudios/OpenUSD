//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMtlx/materialXInfoAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdMtlxMaterialXInfoAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdMtlxMaterialXInfoAPI::~UsdMtlxMaterialXInfoAPI()
{
}

/* static */
UsdMtlxMaterialXInfoAPI
UsdMtlxMaterialXInfoAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdMtlxMaterialXInfoAPI();
    }
    return UsdMtlxMaterialXInfoAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdMtlxMaterialXInfoAPI::_GetSchemaKind() const
{
    return UsdMtlxMaterialXInfoAPI::schemaKind;
}

/* static */
bool
UsdMtlxMaterialXInfoAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdMtlxMaterialXInfoAPI>(whyNot);
}

/* static */
UsdMtlxMaterialXInfoAPI
UsdMtlxMaterialXInfoAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdMtlxMaterialXInfoAPI>()) {
        return UsdMtlxMaterialXInfoAPI(prim);
    }
    return UsdMtlxMaterialXInfoAPI();
}

/* static */
const TfType &
UsdMtlxMaterialXInfoAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdMtlxMaterialXInfoAPI>();
    return tfType;
}

/* static */
bool 
UsdMtlxMaterialXInfoAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdMtlxMaterialXInfoAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdMtlxMaterialXInfoAPI::GetInfoMtlxVersionAttr() const
{
    return GetPrim().GetAttribute(UsdMtlxTokens->infoMtlxVersion);
}

UsdAttribute
UsdMtlxMaterialXInfoAPI::CreateInfoMtlxVersionAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdMtlxTokens->infoMtlxVersion,
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
UsdMtlxMaterialXInfoAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdMtlxTokens->infoMtlxVersion,
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
