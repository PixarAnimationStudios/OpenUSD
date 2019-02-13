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
#ifndef USDUTILS_DEPENDENCIES_H
#define USDUTILS_DEPENDENCIES_H

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
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdUtils/api.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

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

/// Creates a USDZ package containing the specified asset, identified by its 
/// \p assetPath. The created package will include a localized version of the 
/// asset itself and all of its external dependencies. Due to localization, the 
/// packaged layers might be modified to have different asset paths.
///
/// You can optionally specify a different package-internal name for the first
/// layer of the asset by specifying \p firstLayerName. By default,
/// \p firstLayerName is empty, meaning that the original name is preserved.
/// 
/// Returns true if the package was created successfully.
/// 
/// \note Clients of this function must take care of configuring the asset 
/// resolver context before invoking the function. To create a default 
/// resolver context, use \ref CreateDefaultContextForAsset() with the 
/// asset path.
/// 
/// \note If the given asset has a dependency on a directory (i.e. an external 
/// reference to a directory path), the dependency is ignored and the contents 
/// of the directory are not included in the created package. 
/// 
/// \note This function modifies the layers referenced by \p assetPath 
/// (including the root layer and all transitive layer dependencies) in-place. 
/// However, it does not save the layers before copying them into the package 
/// that is created. It also does not revert the changes it makes to the 
/// layers. Therefore, it is strongly recommended that you run this function in 
/// isolation after any source UsdStages have been closed. If you have UsdStages 
/// open during the function call that reference the layers being modified, you 
/// may receive warnings or composition errors which may not affect the 
/// resulting package adversely.
/// 
/// \sa UsdUtilsCreateNewARKitUsdzPackage()
USDUTILS_API
bool
UsdUtilsCreateNewUsdzPackage(
    const SdfAssetPath& assetPath,
    const std::string& usdzFilePath,
    const std::string& firstLayerName=std::string());

/// Similar to UsdUtilsCreateNewUsdzPackage, this function packages all of the 
/// dependencies of the given asset. Assets targeted at the initial usdz 
/// implementation in ARKit operate under greater constraints than usdz files 
/// for more general 'in house' uses, and this option attempts to ensure that
/// these constraints are honored; this may involve more transformations to the 
/// data, which may cause loss of features such as VariantSets.
///
/// If \p firstLayerName is specified, it is modified to have the ".usdc" 
/// extension, as required by the initial usdz implementation in ARKit.
/// 
/// Returns true if the package was created successfully.
/// 
/// \note Clients of this function must take care of configuring the asset 
/// resolver context before invoking the function. To create a default 
/// resolver context, use \ref CreateDefaultContextForAsset() with the 
/// asset path.
/// 
/// \note If the given asset has a dependency on a directory (i.e. an external 
/// reference to a directory path), the dependency is ignored and the contents 
/// of the directory are not included in the created package. 
/// 
/// \note This function modifies the layers referenced by \p assetPath 
/// (including the root layer and all transitive layer dependencies) in-place. 
/// However, it does not save the layers before copying them into the package 
/// that is created. It also does not revert the changes it makes to the 
/// layers. Therefore, it is strongly recommended that you run this function in 
/// isolation after any source UsdStages have been closed. If you have UsdStages 
/// open during the function call that reference the layers being modified, you 
/// may receive warnings or composition errors which may not affect the 
/// resulting package adversely.  
/// 
/// \sa UsdUtilsCreateNewUsdzPackage()
USDUTILS_API
bool
UsdUtilsCreateNewARKitUsdzPackage(
    const SdfAssetPath &assetPath,
    const std::string &usdzFilePath,
    const std::string &firstLayerName=std::string());

/// Recursively computes all the dependencies of the given asset and populates
/// \p layers with all the dependencies that can be opened as an SdfLayer. 
/// All of the resolved non-layer dependencies are populated in \p assets.
/// Any unresolved (layer and non-layer) asset paths are populated in 
/// \p unresolvedPaths.
/// 
/// The input vectors to be populated with the results are are *cleared* before 
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
/// new value that should be authored in the layer.
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDUTILS_DEPENDENCIES_H
