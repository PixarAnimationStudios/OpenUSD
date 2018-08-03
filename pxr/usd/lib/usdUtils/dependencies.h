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
USDUTILS_API
bool
UsdUtilsCreateNewARKitUsdzPackage(
    const SdfAssetPath &assetPath,
    const std::string &usdzFilePath,
    const std::string &firstLayerName=std::string());

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDUTILS_DEPENDENCIES_H
