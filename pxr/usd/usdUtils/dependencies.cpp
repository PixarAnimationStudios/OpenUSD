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
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUtils/assetLocalization.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/debugCodes.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layerUtils.h"

#include "pxr/base/trace/trace.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// XXX: don't even know if it's important to distinguish where
// these asset paths are coming from..  if it's not important, maybe this
// should just go into Sdf's _GatherPrimAssetReferences?  if it is important,
// we could also have another function that takes 3 vectors.
void 
UsdUtilsExtractExternalReferences(
    const std::string& filePath,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads)
{
    UsdUtils_ExtractExternalReferences(filePath, 
        UsdUtils_LocalizationContext::ReferenceType::All,
        subLayers, references, payloads);
}

bool
UsdUtilsComputeAllDependencies(
    const SdfAssetPath &assetPath,
    std::vector<SdfLayerRefPtr> *outLayers,
    std::vector<std::string> *outAssets,
    std::vector<std::string> *outUnresolvedPaths)
{
    std::vector<SdfLayerRefPtr> layers;
    std::vector<std::string> assets, unresolvedPaths;

    const auto processFunc = [&layers, &assets, &unresolvedPaths]( 
        const SdfLayerRefPtr &layer, 
        const std::string &,
        const std::vector<std::string> &dependencies,
        UsdUtilsDependencyType)
    {
        for (const auto & dependency : dependencies) {
            const std::string anchoredPath = 
                SdfComputeAssetPathRelativeToLayer(layer, dependency);
            const std::string resolvedPath = ArGetResolver().Resolve(anchoredPath);

            if (resolvedPath.empty()) {
                unresolvedPaths.emplace_back(anchoredPath);
            }
            else if (UsdStage::IsSupportedFile(anchoredPath)) {
                layers.push_back(SdfLayer::FindOrOpen(anchoredPath));
            }
            else {
                assets.push_back(resolvedPath);
            }
        }
    };

    UsdUtils_ReadOnlyLocalizationDelegate delegate(processFunc);
    UsdUtils_LocalizationContext context(&delegate);
    context.SetMetadataFilteringEnabled(true);

    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(assetPath.GetAssetPath());

    if (!rootLayer) {
        return false;
    }

    layers.emplace_back(rootLayer);

    if (!context.Process(rootLayer)) {
        return false;
    }

    if (outLayers) {
        *outLayers = std::move(layers);
    }
    if (outAssets) {
        *outAssets = std::move(assets);
    }
    if (outUnresolvedPaths) {
        *outUnresolvedPaths = std::move(unresolvedPaths);
    }

    return true;
}

void 
UsdUtilsModifyAssetPaths(
    const SdfLayerHandle& layer,
    const UsdUtilsModifyAssetPathFn& modifyFn)
{
    using DependencyType = UsdUtilsDependencyType;

    auto processingFunc = 
        [&modifyFn](const SdfLayerRefPtr&, const std::string& assetPath, 
            const std::vector<std::string>& additionalPaths, DependencyType)
        {
            return modifyFn(assetPath);
        };

    UsdUtils_WritableLocalizationDelegate delegate(processingFunc);
    UsdUtils_LocalizationContext context(&delegate);
    context.Process(layer);
}

PXR_NAMESPACE_CLOSE_SCOPE
