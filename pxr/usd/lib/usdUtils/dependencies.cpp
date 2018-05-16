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
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/variantSetSpec.h"
#include "pxr/usd/sdf/variantSpec.h"
#include "pxr/base/trace/trace.h"

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
_ExtractDependencies(
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

            // payloads
            if (curr->HasPayload()) {
                const SdfPayload payload = curr->GetPayload();
                _AppendAssetPathIfNotEmpty(payload, payloads);
            }

            // properties
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
                    // For every property
                    // Build an SdfPath to the property
                    const SdfPath path = curr->GetPath().AppendProperty(name);

                    // Check property metadata
                    for (const TfToken& infoKey : layer->ListFields(path)) {
                        if (infoKey != SdfFieldKeys->Default &&
                            infoKey != SdfFieldKeys->TimeSamples)
                            _AppendAssetValue(layer->GetField(path, infoKey),
                                references);
                    }

                    // Check property existence
                    const VtValue vtTypeName =
                        layer->GetField(path, SdfFieldKeys->TypeName);
                    if (!vtTypeName.IsHolding<TfToken>())
                        continue;

                    const TfToken typeName =
                        vtTypeName.UncheckedGet<TfToken>();
                    if (typeName == SdfValueTypeNames->Asset ||
                        typeName == SdfValueTypeNames->AssetArray) {

                        // Check default value
                        _AppendAssetValue(layer->GetField(path,
                            SdfFieldKeys->Default), references);

                        // Check timeSample values
                        for (double t : layer->ListTimeSamplesForPath(path)) {
                            VtValue timeSampleVal;
                            if (layer->QueryTimeSample(path,
                                t, &timeSampleVal)) {
                                _AppendAssetValue(timeSampleVal, references);
                            }
                        }
                    }
                }
            }

            // metadata
            for (const TfToken& infoKey : curr->GetMetaDataInfoKeys()) {
                _AppendAssetValue(curr->GetInfo(infoKey), references);
            }

            // references
            const SdfReferencesProxy refList = curr->GetReferenceList();
            for (const SdfReference& ref :
                refList.GetAddedOrExplicitItems()) {
                _AppendAssetPathIfNotEmpty(ref, references);
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

    // Remove duplicates
    std::sort(references->begin(), references->end());
    references->erase(std::unique(references->begin(), references->end()),
        references->end());
    std::sort(payloads->begin(), payloads->end());
    payloads->erase(std::unique(payloads->begin(), payloads->end()),
        payloads->end());

}

void
UsdUtilsExtractExternalReferences(
    const string& filePath,
    vector<string>* subLayers,
    vector<string>* references,
    vector<string>* payloads)
{
    TRACE_FUNCTION();
    _ExtractDependencies(filePath, subLayers, references, payloads);
}

PXR_NAMESPACE_CLOSE_SCOPE

