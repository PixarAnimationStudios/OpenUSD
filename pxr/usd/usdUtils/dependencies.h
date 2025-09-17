//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_DEPENDENCIES_H
#define PXR_USD_USD_UTILS_DEPENDENCIES_H

/// \file usdUtils/dependencies.h
///
/// Utilities for the following tasks that require consideration of a USD
/// asset's external dependencies:
/// * extracting asset dependencies from a USD file.
/// * creating a USDZ package containing a given asset and all of its external 
/// dependencies.
/// * some time in the future, localize a given asset and all of its 
/// dependencies into a specified directory.
/// 

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/usdUtils/usdzPackage.h"
#include "pxr/usd/usdUtils/userProcessingFunc.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Structure which controls aspects of the 
/// \ref UsdUtilsExtractExternalReferences function.
class UsdUtilsExtractExternalReferencesParams {
public:
    /// Specifies whether UDIM template paths should be resolved when extracting
    /// references. If true, the resolved paths for all discovered UDIM tiles
    /// will be included in the references bucket and the template path will be
    /// discarded. If false, no resolution will take place and the template path
    /// will appear in the references bucket.
    inline void SetResolveUdimPaths(bool resolveUdimPaths) {
        _resolveUdimPaths = resolveUdimPaths;
    }


    inline bool GetResolveUdimPaths() const { return _resolveUdimPaths; }

private:
    bool _resolveUdimPaths = false;
};

/// Parses the file at \p filePath, identifying external references, and
/// sorting them into separate type-based buckets. Sublayers are returned in
/// the \p sublayers vector, references, whether prim references, value clip 
/// references or values from asset path attributes, are returned in the 
/// \p references vector. Payload paths are returned in \p payloads.
/// The \p params parameter controls various settings that affect the
/// extraction process. See \ref UsdUtilsExtractExternalReferencesParams for
/// additional details.
/// 
/// \note No recursive chasing of dependencies is performed; that is the
/// client's responsibility, if desired.
/// 
/// \note Not all returned references are actually authored explicitly in the 
/// layer. For example, templated clip asset paths are resolved and expanded 
/// to include all available clip files that match the specified pattern.
USDUTILS_API
void UsdUtilsExtractExternalReferences(
    const std::string& filePath,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads,
    const UsdUtilsExtractExternalReferencesParams& params = {});

/// Recursively computes all the dependencies of the given asset and populates
/// \p layers with all the dependencies that can be opened as an SdfLayer. 
/// All of the resolved non-layer dependencies are populated in \p assets.
/// Any unresolved (layer and non-layer) asset paths are populated in 
/// \p unresolvedPaths.
///
/// If a function is provided for the \p processingFunc parameter, it will be
/// invoked on every asset path that is discovered during localization.  
/// Refer to \ref UsdUtilsDependencyInfo for general information on User 
/// processing functions.  Any changes made to the paths during the 
/// invocation of this function will not be written to processed layers.
///
/// The input vectors to be populated with the results are *cleared* before 
/// any results are added to them.
/// 
/// Returns true if the given asset was resolved correctly.
USDUTILS_API
bool
UsdUtilsComputeAllDependencies(
    const SdfAssetPath &assetPath,
    std::vector<SdfLayerRefPtr> *layers,
    std::vector<std::string> *assets,
    std::vector<std::string> *unresolvedPaths,
    const std::function<UsdUtilsProcessingFunc> &processingFunc = 
        std::function<UsdUtilsProcessingFunc>());

/// Callback that is used to modify asset paths in a layer.  The \c assetPath
/// will contain the string value that's authored.  The returned value is the
/// new value that should be authored in the layer.  If the function returns
/// an empty string, that value will be removed from the layer.
using UsdUtilsModifyAssetPathFn = std::function<std::string(
    const std::string& assetPath)>;

/// Helper function that visits every asset path in \c layer, calls \c modifyFn
/// and replaces the value with the return value of \c modifyFn.  This modifies
/// \c layer in place. If the \c keepEmptyPathsInArrays parameter is true, empty 
/// asset paths will be written in into arrays even if \c modifyFn returns an 
/// empty string.  This functionality is useful in cases where arrays are 
/// expected to have a specific length or such values may be meaningful.
///
/// This can be useful in preparing a layer for consumption in contexts that do
/// not have access to the ArResolver for which the layer's asset paths were
/// authored: we can replace all paths with their fully resolved equivalents,
/// for example.
USDUTILS_API
void UsdUtilsModifyAssetPaths(
    const SdfLayerHandle& layer,
    const UsdUtilsModifyAssetPathFn& modifyFn,
    bool keepEmptyPathsInArrays = false);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_DEPENDENCIES_H
