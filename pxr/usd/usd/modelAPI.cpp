//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdModelAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdModelAPI::~UsdModelAPI()
{
}

/* static */
UsdModelAPI
UsdModelAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdModelAPI();
    }
    return UsdModelAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdModelAPI::_GetSchemaKind() const
{
    return UsdModelAPI::schemaKind;
}

/* static */
const TfType &
UsdModelAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdModelAPI>();
    return tfType;
}

/* static */
bool 
UsdModelAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdModelAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdModelAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

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
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/clip.h"
#include "pxr/usd/kind/registry.h"

#include <string>
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdModelAPIAssetInfoKeys, USDMODEL_ASSET_INFO_KEYS);

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdModelAPI::KindValidationNone);
    TF_ADD_ENUM_NAME(UsdModelAPI::KindValidationModelHierarchy);
}


bool
UsdModelAPI::GetKind(TfToken* retValue) const
{
    return GetPrim().GetKind(retValue);
}

bool
UsdModelAPI::SetKind(const TfToken& value) const
{
    return GetPrim().SetKind(value);
}

bool
UsdModelAPI::IsKind(const TfToken& baseKind,
                    UsdModelAPI::KindValidation validation) const{
    if (validation == UsdModelAPI::KindValidationModelHierarchy){
        if (KindRegistry::IsA(baseKind, KindTokens->model) && !IsModel())
            return false;
    }
    TfToken primKind;
    if (!GetKind(&primKind))
        return false;
    return KindRegistry::IsA(primKind, baseKind);
}

bool
UsdModelAPI::IsModel() const
{
    return GetPrim().IsModel();
}

bool 
UsdModelAPI::IsGroup() const
{
    return GetPrim().IsGroup();
}

////////////////////////////////////////////////////////////////////////
// Asset Info API
////////////////////////////////////////////////////////////////////////

bool 
UsdModelAPI::GetAssetIdentifier(SdfAssetPath *identifier) const
{
    return _GetAssetInfoByKey(UsdModelAPIAssetInfoKeys->identifier, identifier);
}

void 
UsdModelAPI::SetAssetIdentifier(const SdfAssetPath &identifier) const
{
    GetPrim().SetAssetInfoByKey(UsdModelAPIAssetInfoKeys->identifier, 
                                VtValue(identifier));
}

bool 
UsdModelAPI::GetAssetName(string *assetName) const
{
    return _GetAssetInfoByKey(UsdModelAPIAssetInfoKeys->name, assetName);
}

void 
UsdModelAPI::SetAssetName(const string &assetName) const
{
    GetPrim().SetAssetInfoByKey(UsdModelAPIAssetInfoKeys->name, 
                                VtValue(assetName));
}

bool 
UsdModelAPI::GetAssetVersion(string *version) const
{
    return _GetAssetInfoByKey(UsdModelAPIAssetInfoKeys->version, version);
}

void
UsdModelAPI::SetAssetVersion(const string &version) const
{
    GetPrim().SetAssetInfoByKey(UsdModelAPIAssetInfoKeys->version, 
                                VtValue(version));
}

bool 
UsdModelAPI::GetPayloadAssetDependencies(VtArray<SdfAssetPath> *assetDeps) const
{
    return _GetAssetInfoByKey(UsdModelAPIAssetInfoKeys->payloadAssetDependencies, 
                              assetDeps);
}


void 
UsdModelAPI::SetPayloadAssetDependencies(const VtArray<SdfAssetPath> &assetDeps) const
{
    GetPrim().SetAssetInfoByKey(UsdModelAPIAssetInfoKeys->payloadAssetDependencies, 
                                VtValue(assetDeps));    
}

bool 
UsdModelAPI::GetAssetInfo(VtDictionary *info) const
{
    if (GetPrim().HasAssetInfo()) {
        *info = GetPrim().GetAssetInfo();
        return true;
    }

    return false;
}

void
UsdModelAPI::SetAssetInfo(const VtDictionary &info) const
{
    GetPrim().SetAssetInfo(info);
}

PXR_NAMESPACE_CLOSE_SCOPE
