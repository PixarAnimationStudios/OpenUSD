//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file usdUtils/dependencies.cpp
#include "pxr/pxr.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUtils/assetLocalization.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/debugCodes.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/sdf/layerUtils.h"

#include "pxr/base/trace/trace.h"

#include <functional>
#include <unordered_set>
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
    std::vector<std::string>* payloads,
    const UsdUtilsExtractExternalReferencesParams& params)
{
    UsdUtils_ExtractExternalReferences(filePath, 
        UsdUtils_LocalizationContext::ReferenceType::All,
        subLayers, references, payloads, params);
}

struct UsdUtils_ComputeAllDependenciesClient
{
    UsdUtils_ComputeAllDependenciesClient(
        const std::function<UsdUtilsProcessingFunc> &processingFunc)
            :processingFunc(processingFunc) {}

    UsdUtilsDependencyInfo 
    Process( 
        const SdfLayerRefPtr &layer, 
        const UsdUtilsDependencyInfo &depInfo,
        UsdUtils_DependencyType dependencyType)
    {
        
        if (processingFunc) {
            UsdUtilsDependencyInfo processedInfo = 
                processingFunc(layer, depInfo);
            
            if (processedInfo.GetAssetPath().empty()) {
                return {};
            }

            // When using a processing function with template asset paths
            // such as clips or udim, if the user does not modify the
            // asset path, we do not want to place it in the resulting arrays
            // We always want to add dependencies, however
            bool originalPathIsTemplate = !depInfo.GetDependencies().empty();
            if (processedInfo != depInfo || !originalPathIsTemplate) {
                PlaceAsset(layer, processedInfo.GetAssetPath(), dependencyType);
            }
            
            for (const auto & dependency : processedInfo.GetDependencies()) {
                PlaceAsset(layer, dependency, dependencyType);
            }

            return processedInfo;
        }

        if (depInfo.GetDependencies().empty()) {
            PlaceAsset(layer, depInfo.GetAssetPath(), dependencyType);
        }
        else {
            for (const auto & dependency : depInfo.GetDependencies()) {
                PlaceAsset(layer, dependency, dependencyType);
            }
        }

        return {};
    }

    bool 
    PathShouldResolve(
        const SdfLayerRefPtr &layer, 
        const std::string& resolvedPath,
        UsdUtils_DependencyType dependencyType)
    {
        // XXX: We do not currently support resolving clip template asset paths
        // from packages in the asset localization code (refer to 
        // UsdUtils_LocalizationContext::_GetTemplatedClips).  In this case we
        // do not want to treat these paths as unresolved.
        if (dependencyType != UsdUtils_DependencyType::ClipTemplateAssetPath) {
            return true;
        }
        else {
            return !ArIsPackageRelativePath(layer->GetRealPath()) &&
                   !layer->GetFileFormat()->IsPackage();
        }
    }

    void 
    PlaceAsset(
        const SdfLayerRefPtr &layer, 
        const std::string& dependency,
        UsdUtils_DependencyType dependencyType)
    {
        const std::string anchoredPath = 
            SdfComputeAssetPathRelativeToLayer(layer, dependency);
        const std::string resolvedPath = ArGetResolver().Resolve(anchoredPath);

        if (resolvedPath.empty()) {
            if (PathShouldResolve(layer, resolvedPath, dependencyType)) {
                unresolvedPaths.insert(anchoredPath);
            }
        }
        else if (UsdStage::IsSupportedFile(anchoredPath)) {
            layers.insert(SdfLayer::FindOrOpen(anchoredPath));
        }
        else {
            assets.insert(resolvedPath);
        }
    }

    std::unordered_set<SdfLayerRefPtr, TfHash> layers;
    std::unordered_set<std::string> assets, unresolvedPaths;
    std::function<UsdUtilsProcessingFunc> processingFunc;
};

bool
UsdUtilsComputeAllDependencies(
    const SdfAssetPath &assetPath,
    std::vector<SdfLayerRefPtr> *outLayers,
    std::vector<std::string> *outAssets,
    std::vector<std::string> *outUnresolvedPaths,
    const std::function<UsdUtilsProcessingFunc> &processingFunc)
{
    SdfLayerRefPtr rootLayer = SdfLayer::FindOrOpen(assetPath.GetAssetPath());

    if (!rootLayer) {
        return false;
    }

    UsdUtils_ComputeAllDependenciesClient client(processingFunc);
    UsdUtils_ReadOnlyLocalizationDelegate delegate(
        std::bind(&UsdUtils_ComputeAllDependenciesClient::Process, &client,
            std::placeholders::_1, std::placeholders::_2, 
            std::placeholders::_3));
    UsdUtils_LocalizationContext context(&delegate);
    context.SetMetadataFilteringEnabled(true);

    if (!context.Process(rootLayer)) {
        return false;
    }

    if (outLayers) {
        outLayers->push_back(rootLayer);
        outLayers->insert(outLayers->end(), 
            client.layers.begin(), client.layers.end());
        std::sort(outLayers->begin() + 1, outLayers->end(), [](const SdfLayerRefPtr& a, const SdfLayerRefPtr& b) {
            return a->GetRealPath() < b->GetRealPath();
        });
    }
    if (outAssets) {
        outAssets->assign(client.assets.begin(), client.assets.end());
        std::sort(outAssets->begin(), outAssets->end());
    }
    if (outUnresolvedPaths) {
        outUnresolvedPaths->assign(
            client.unresolvedPaths.begin(), client.unresolvedPaths.end());
        std::sort(outUnresolvedPaths->begin(), outUnresolvedPaths->end());
    }

    return true;
}

void 
UsdUtilsModifyAssetPaths(
    const SdfLayerHandle& layer,
    const UsdUtilsModifyAssetPathFn& modifyFn,
    bool keepEmptyPathsInArrays)
{
    auto processingFunc = 
        [&modifyFn](const SdfLayerRefPtr&, 
            const UsdUtilsDependencyInfo &dependencyInfo,
            UsdUtils_DependencyType)
        {
            return UsdUtilsDependencyInfo(
                modifyFn(dependencyInfo.GetAssetPath()));
        };

    UsdUtils_WritableLocalizationDelegate delegate(processingFunc);
    UsdUtils_LocalizationContext context(&delegate);
    delegate.SetEditLayersInPlace(true);
    context.SetRecurseLayerDependencies(false);

    delegate.SetKeepEmptyPathsInArrays(keepEmptyPathsInArrays);

    context.Process(layer);
}

PXR_NAMESPACE_CLOSE_SCOPE
