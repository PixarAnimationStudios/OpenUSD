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
///
/// \file usdUtils/dependencies.cpp
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/dependencies.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/reference.h"
#include "pxr/usd/sdf/textReferenceParser.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/usd/usd/usdaFileFormat.h"
#include "pxr/base/tracelite/trace.h"

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE


using std::string;
using std::vector;

template <typename T>
static void
_AppendAssetPathIfNotEmpty(
    const T& assetPath,
    vector<string>* deps)
{
    if (TF_VERIFY(deps) && !assetPath.GetAssetPath().empty())
        deps->push_back(assetPath.GetAssetPath());
}

// val should be an attribute holding either SdfAssetPath or
// VtArray<SdfAssetPath>.  it will assetpaths everything to deps.
static void
_AppendAssetValue(
    const VtValue& val,
    vector<string>* deps)
{
    if (val.IsHolding<SdfAssetPath>()) {
        _AppendAssetPathIfNotEmpty(val.UncheckedGet<SdfAssetPath>(), deps);
    }
    else if (val.IsHolding<VtArray<SdfAssetPath> >()) {
        for (const SdfAssetPath& assetPath :
            val.UncheckedGet< VtArray<SdfAssetPath> >()) {
            _AppendAssetPathIfNotEmpty(assetPath, deps);
        }
    }
    else if (val.IsHolding<VtDictionary>()) {
        for (const auto& p : val.UncheckedGet<VtDictionary>()) {
            _AppendAssetValue(p.second, deps);
        }
    }
}

// XXX: i don't even know if it's important to distinguish where
// these asset paths are coming from..  if it's not important, maybe this
// should just go into Sdf's _GatherPrimAssetReferences?  if it is important,
// we could also have another function that takes 3 vectors.
static void
_ExtractDependenciesForBinary(
    const std::string& filePath,
    std::vector<std::string>* sublayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads)
{
    TRACE_FUNCTION();

    SdfLayerRefPtr layer = SdfLayer::OpenAsAnonymous(filePath);
    if (!TF_VERIFY(layer))
        return;

    *sublayers = layer->GetSubLayerPaths();

    std::stack<SdfPrimSpecHandle> dfs;
    dfs.push(layer->GetPseudoRoot());

    while (!dfs.empty()) {
        SdfPrimSpecHandle curr = dfs.top();
        dfs.pop();

        if (curr != layer->GetPseudoRoot()) {
            // references
            const SdfReferencesProxy refList = curr->GetReferenceList();
            for (const SdfReference& ref :
                refList.GetAddedOrExplicitItems()) {
                _AppendAssetPathIfNotEmpty(ref, references);
            }

            // payloads
            if (curr->HasPayload()) {
                const SdfPayload payload = curr->GetPayload();
                _AppendAssetPathIfNotEmpty(payload, payloads);
            }

            // attributes
            //
            // XXX:2016-04-14 Note that we use the field access API
            // here rather than calling GetAttributes, as creating specs for
            // large numbers of attributes, most of which are *not* asset
            // path-valued and therefore not useful here, is expensive.
            //
            const VtValue propertyNames =
                curr->GetField(SdfChildrenKeys->PropertyChildren);
            if (propertyNames.IsHolding<vector<TfToken>>()) {
                for (const auto& name :
                        propertyNames.UncheckedGet<vector<TfToken>>()) {
                    const SdfPath path = curr->GetPath().AppendProperty(name);
                    const VtValue vtTypeName = layer->GetField(
                        path, SdfFieldKeys->TypeName);
                    if (!vtTypeName.IsHolding<TfToken>())
                        continue;

                    const TfToken typeName =
                        vtTypeName.UncheckedGet<TfToken>();
                    if (typeName == SdfValueTypeNames->Asset ||
                        typeName == SdfValueTypeNames->AssetArray) {
                        _AppendAssetValue(layer->GetField(
                            path, SdfFieldKeys->Default), references);
                    }
                }
            }

            // meta data
            for (const TfToken& infoKey : curr->GetMetaDataInfoKeys()) {
                _AppendAssetValue(curr->GetInfo(infoKey), references);
            }
        }

        // variants "children"
        for (const SdfVariantSetsProxy::value_type& p :
            curr->GetVariantSets()) {
            for (const SdfVariantSpecHandle& variantSpec :
                p.second->GetVariantList()) {
                dfs.push(variantSpec->GetPrimSpec());
            }
        }

        // children
        for (const SdfPrimSpecHandle& child : curr->GetNameChildren()) {
            dfs.push(child);
        }
    }
}

// XXX:2014-10-23 It would be great if USD provided this for us.
static bool
_IsUsdBinary(const std::string& filePath)
{
    SdfFileFormatConstPtr textFormat =
        SdfFileFormat::FindById(UsdUsdaFileFormatTokens->Id);
    return !(textFormat && textFormat->CanRead(filePath));
}

void
UsdUtilsExtractExternalReferences(
    const string& filePath,
    vector<string>* subLayers,
    vector<string>* references,
    vector<string>* payloads)
{
    TRACE_FUNCTION();

    if (_IsUsdBinary(filePath)) {
        _ExtractDependenciesForBinary(filePath,
            subLayers, references, payloads);
    } else {
        SdfExtractExternalReferences(filePath,
            subLayers, references, payloads);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE

