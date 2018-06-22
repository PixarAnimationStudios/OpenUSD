//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdModelAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (ModelAPI)
);

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
UsdSchemaType UsdModelAPI::_GetSchemaType() const {
    return UsdModelAPI::schemaType;
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
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usd/clip.h"
#include "pxr/usd/kind/registry.h"

#include <string>
using std::string;

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(UsdModelAPIAssetInfoKeys, USDMODEL_ASSET_INFO_KEYS);

bool
UsdModelAPI::GetKind(TfToken* retValue) const
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().GetMetadata(SdfFieldKeys->Kind, retValue);
}
bool
UsdModelAPI::SetKind(const TfToken& value)
{
    if (GetPath() == SdfPath::AbsoluteRootPath()) {
        // Special-case to pre-empt coding errors.
        return false;
    }

    return GetPrim().SetMetadata(SdfFieldKeys->Kind, value);
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
