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

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Parses the file at \p filePath, identifying external references, and
/// sorting them into separate type-based buckets. Sublayers are returned in
/// the \p sublayers vector, references, whether prim references, value clip 
/// references or values from asset path attributes, are returned in the 
/// \p references vector. Payload paths are returned in \p payloads.
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
    std::vector<std::string>* payloads);

/// Recursively computes all the dependencies of the given asset and populates
/// \p layers with all the dependencies that can be opened as an SdfLayer. 
/// All of the resolved non-layer dependencies are populated in \p assets.
/// Any unresolved (layer and non-layer) asset paths are populated in 
/// \p unresolvedPaths.
/// 
/// The input vectors to be populated with the results are *cleared* before 
/// any results are added to them.
/// 
/// Returns true if the given asset was resolved correctly.
USDUTILS_API
bool
UsdUtilsComputeAllDependencies(const SdfAssetPath &assetPath,
                               std::vector<SdfLayerRefPtr> *layers,
                               std::vector<std::string> *assets,
                               std::vector<std::string> *unresolvedPaths);

/// Callback that is used to modify asset paths in a layer.  The \c assetPath
/// will contain the string value that's authored.  The returned value is the
/// new value that should be authored in the layer.  If the function returns
/// an empty string, that value will be removed from the layer.
using UsdUtilsModifyAssetPathFn = std::function<std::string(
        const std::string& assetPath)>;

/// Helper function that visits every asset path in \c layer, calls \c modifyFn
/// and replaces the value with the return value of \c modifyFn.  This modifies
/// \c layer in place.
///
/// This can be useful in preparing a layer for consumption in contexts that do
/// not have access to the ArResolver for which the layer's asset paths were
/// authored: we can replace all paths with their fully resolved equivalents,
/// for example.
USDUTILS_API
void UsdUtilsModifyAssetPaths(
        const SdfLayerHandle& layer,
        const UsdUtilsModifyAssetPathFn& modifyFn);

// Enum class representing the type of dependency.
enum class UsdUtilsDependencyType {
    Reference,
    SubLayer,
    Payload
};

// Signature for user supplied processing function.  Note if the asset path
// that is returned from this function is the empty string then the asset
// path will be removed.
// \param layer The layer containing this dependency
// \param assetPath The asset path as authored in the layer
// \param dependencies  All actual dependencies associated with this asset path. 
// Multiple items may be present in this array if the asset path is, for 
// example, a udim specifier or a clips 'templateAssetPath'
// \param dependencyType enumerates the type of this dependency
using UsdUtilsProcessingFunc = std::function<std::string(
        const SdfLayerRefPtr &layer, 
        const std::string &assetPath, 
        const std::vector<std::string>& dependencies,
        UsdUtilsDependencyType dependencyType)>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_DEPENDENCIES_H
