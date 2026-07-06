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
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerUtils.h"

#include "pxr/base/tf/diagnostic.h"
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

class UsdUtils_ComputeAllDependenciesClient : 
    public UsdUtils_ReadOnlyLocalizationClient
{
public:
    UsdUtils_ComputeAllDependenciesClient(
        const std::function<UsdUtilsProcessingFunc> &processingFunc)
            :processingFunc(processingFunc) {}

    UsdUtilsDependencyInfo 
    _ProcessDependency( 
        const SdfLayerRefPtr &layer, 
        const UsdUtilsDependencyInfo &depInfo,
        UsdUtils_DependencyType dependencyType) override
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

        return depInfo;
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

        const std::pair<std::string, std::string> splitIdentifier =
            SdfLayer::SplitIdentifier(anchoredPath);

        const std::string resolvedPath = ArGetResolver().Resolve(
            splitIdentifier.first);

        if (resolvedPath.empty()) {
            if (PathShouldResolve(layer, resolvedPath, dependencyType)) {
                unresolvedPaths.insert(anchoredPath);
            }
        }
        else if (UsdStage::IsSupportedFile(anchoredPath)) {
            SdfLayerRefPtr dependencyLayer = SdfLayer::FindOrOpen(anchoredPath);
            if (dependencyLayer) {
                layers.insert(dependencyLayer);

                // Note: for layers we also want to include any additional asset
                // dependencies.
                const std::set<std::string> externalAssetDependencies = 
                    dependencyLayer->GetExternalAssetDependencies();

                for (const auto& dependency : externalAssetDependencies) {
                    assets.insert(dependency);
                }
            } else {
                TF_WARN("Failed to open dependency layer: %s (%s)", 
                    dependency.c_str(), anchoredPath.c_str());
            }
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
    UsdUtils_LocalizationContext context(&client);
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

class UsdUtils_UsdUtilsModifyAssetPathsClient : 
    public UsdUtils_WritableLocalizationClient 
{
public:
    UsdUtils_UsdUtilsModifyAssetPathsClient(
        const UsdUtilsModifyAssetPathFn &modifyPathFn)
            : modifyPathFn(modifyPathFn) {}

    virtual UsdUtilsDependencyInfo 
    _ProcessDependency( 
        const SdfLayerRefPtr &layer, 
        const UsdUtilsDependencyInfo &depInfo,
        UsdUtils_DependencyType dependencyType) override
    {
        return UsdUtilsDependencyInfo(
            modifyPathFn(depInfo.GetAssetPath()));
    }

    UsdUtilsModifyAssetPathFn modifyPathFn;
};

void 
UsdUtilsModifyAssetPaths(
    const SdfLayerHandle& layer,
    const UsdUtilsModifyAssetPathFn& modifyFn,
    bool keepEmptyPathsInArrays)
{
    UsdUtils_UsdUtilsModifyAssetPathsClient client(modifyFn);
    client.SetEditLayersInPlace(true);
    client.SetKeepEmptyPathsInArrays(keepEmptyPathsInArrays);

    UsdUtils_LocalizationContext context(&client);
    context.SetRecurseLayerDependencies(false);
    context.Process(layer);
}

PXR_NAMESPACE_CLOSE_SCOPE
