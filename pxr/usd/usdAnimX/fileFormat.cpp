//
// Copyright 2020 benmalartre
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
#include "pxr/pxr.h"
#include "data.h"
#include "fileFormat.h"
#include "pxr/usd/sdf/fileIO.h"
#include "pxr/usd/sdf/fileIO_Common.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/pcp/dynamicFileFormatContext.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/atomicOfstreamWrapper.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/arch/fileSystem.h"

#include <boost/assign.hpp>
#include <ostream>

#include <fstream>
#include <string>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
    UsdAnimXFileFormatTokens, 
    USD_ANIMX_FILE_FORMAT_TOKENS);

TF_REGISTRY_FUNCTION(TfType)
{
    SDF_DEFINE_FILE_FORMAT(UsdAnimXFileFormat, SdfFileFormat);
}

UsdAnimXFileFormat::UsdAnimXFileFormat()
    : SdfTextFileFormat(
        UsdAnimXFileFormatTokens->Id,
        UsdAnimXFileFormatTokens->Version,
        UsdAnimXFileFormatTokens->Target)
{
}

UsdAnimXFileFormat::~UsdAnimXFileFormat()
{
}

SdfAbstractDataRefPtr
UsdAnimXFileFormat::InitData(
    const FileFormatArguments &args) const
{
    std::cout << "USD ANIM X INIT DATA !!!" << std::endl;
    return UsdAnimXData::New(UsdAnimXDataParams::FromArgs(args));
}

void 
UsdAnimXFileFormat::ComposeFieldsForFileFormatArguments(
    const std::string &assetPath, 
    const PcpDynamicFileFormatContext &context,
    FileFormatArguments *args,
    VtValue *contextDependencyData) const
{
    UsdAnimXDataParams params;

    // There is one relevant metadata field that should be dictionary valued.
    // Compose this field's value and extract any param values from the 
    // resulting dictionary.
    VtValue val;
    if (context.ComposeValue(UsdAnimXFileFormatTokens->Params, &val) && 
            val.IsHolding<VtDictionary>()) {
        params = UsdAnimXDataParams::FromDict(
            val.UncheckedGet<VtDictionary>());
    }

    // Convert the entire params object to file format arguments. We always 
    // convert all parameters even if they're default as the args are part of
    // the identity of the layer.
    *args = params.ToArgs();
}

bool 
UsdAnimXFileFormat::CanFieldChangeAffectFileFormatArguments(
    const TfToken &field,
    const VtValue &oldValue,
    const VtValue &newValue,
    const VtValue &contextDependencyData) const
{
    // Theres only one relevant field and its values should hold a dictionary.
    const VtDictionary &oldDict = oldValue.IsHolding<VtDictionary>() ?
        oldValue.UncheckedGet<VtDictionary>() : VtGetEmptyDictionary();
    const VtDictionary &newDict = newValue.IsHolding<VtDictionary>() ?
        newValue.UncheckedGet<VtDictionary>() : VtGetEmptyDictionary();

    // The dictionary values for our metadata key are not restricted as to what
    // they may contain so it's possible they may have keys that are completely
    // irrelevant to generating the this file format's parameters. Here we're 
    // demonstrating how we can do a more fine grained analysis based on this
    // fact. In some cases this can provide a better experience for users if 
    // the extra processing in this function can prevent expensive prim 
    // recompositions for changes that don't require it. But keep in mind that
    // there can easily be cases where making this function more expensive can
    // outweigh the benefits of avoiding unnecessary recompositions.

    // Compare relevant values in the old and new dictionaries.
    // If both the old and new dictionaries are empty, there's no change.
    if (oldDict.empty() && newDict.empty()) {
        return false;
    }

    // Otherwise we iterate through each possible parameter value looking for
    // any one that has a value change between the two dictionaries.
    for (const TfToken &token : UsdAnimXDataParamsTokens->allTokens) {
        auto oldIt = oldDict.find(token);
        auto newIt = newDict.find(token);
        const bool oldValExists = oldIt != oldDict.end();
        const bool newValExists = newIt != newDict.end();

        // If param value exists in one or not the other, we have change.
        if (oldValExists != newValExists) {
            return true;
        }
        // Otherwise if it's both and the value differs, we also have a change.
        if (newValExists && oldIt->second != newIt->second) {
            return true;
        }
    }

    // None of the relevant data params changed between the two dictionaries.
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE



