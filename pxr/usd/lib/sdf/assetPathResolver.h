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
/// \file sdf/assetPathResolver.h
///
///

#ifndef SDF_ASSET_PATH_RESOLVER_H
#define SDF_ASSET_PATH_RESOLVER_H

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/layer.h"

#include "pxr/usd/ar/assetInfo.h"
#include "pxr/usd/ar/resolverContext.h"
#include "pxr/base/vt/value.h"

#include <string>

SDF_DECLARE_HANDLES(SdfLayer);

/// \struct Sdf_AssetInfo
///
/// Container for layer asset information.
///
struct Sdf_AssetInfo
{
    std::string identifier;
    std::string realPath;
    ArResolverContext resolverContext;
    ArAssetInfo assetInfo;
};

/// Equality operator for Sdf_AssetInfo structures. Two asset info structures
/// if all fields match exactly.
bool operator==(const Sdf_AssetInfo& lhs, const Sdf_AssetInfo& rhs);

/// Returns true if \p identifier can be used to create a new layer, given
/// characteristics of the identifier itself, and the current path resolver
/// configuration.
bool Sdf_CanCreateNewLayerWithIdentifier(
    const std::string& identifier,
    std::string* whyNot);

/// If \p layerPath is relative, it is first resolved anchored to the
/// current working directory. If the file is found this way, it is returned. 
/// If the file is not found, or \p layerPath is not relative, 
/// the path is resolved as-is.
std::string Sdf_ResolvePath(
    const std::string& layerPath,
    ArAssetInfo* assetInfo = 0);

/// Returns the resolved path for \p layerPath, or the local path if \p
/// layerPath cannot be resolved.
std::string Sdf_ComputeFilePath(
    const std::string& layerPath,
    ArAssetInfo* assetInfo = 0);

/// Returns true if a layer can be written to \p layerPath.
bool Sdf_CanWriteLayerToPath(const std::string& layerPath);

/// Returns a newly allocated Sdf_AssetInfo struct with fields computed using
/// the specified \p identifier and \p filePath. If \p fileVersion is
/// specified, it is used over the discovered revision of the file. It is the
/// responsibility of the caller to delete the returned value.
Sdf_AssetInfo* Sdf_ComputeAssetInfoFromIdentifier(
    const std::string& identifier,
    const std::string& filePath,
    const ArAssetInfo& assetInfo,
    const std::string& fileVersion = std::string());

/// Returns the identifierTemplate, placeholders replaced with information
/// from the specified \p layer.
std::string Sdf_ComputeAnonLayerIdentifier(
    const std::string& identifierTemplate,
    const SdfLayer* layer);

/// Returns true if \p identifier is an anonymous layer identifier.
bool Sdf_IsAnonLayerIdentifier(
    const std::string& identifier);

/// Returns the portion of the anonymous layer identifier to be used as the
/// display name. This is either the identifier tag, if one is present, or the
/// empty string.
std::string Sdf_GetAnonLayerDisplayName(
    const std::string& identifier);

/// Returns the anonymous layer identifier template, from which
/// Sdf_ComputeAnonLayerIdentifier can compute an anonymous layer identifier.
std::string Sdf_GetAnonLayerIdentifierTemplate(
    const std::string& tag);

/// Splits the given \p identifier into two portions: the layer path
/// and the arguments. For example, given the identifier foo.menva?a=b,
/// this function returns ("foo.menva", "?a=b")
bool Sdf_SplitIdentifier(
    const std::string& identifier,
    std::string* layerPath,
    std::string* arguments);

/// Splits the given \p identifier into the layer path and the arguments.
bool Sdf_SplitIdentifier(
    const std::string& identifier,
    std::string* layerPath,
    SdfLayer::FileFormatArguments* arguments);

/// Joins the given \p layerPath and \p arguments into an identifier.
/// These parameters are expected to be in the format returned by 
/// Sdf_SplitIdentifier.
std::string Sdf_CreateIdentifier(
    const std::string& layerPath,
    const std::string& arguments);

/// Joins the given \p layerPath and \p arguments into an identifier.
std::string Sdf_CreateIdentifier(
    const std::string& layerPath,
    const SdfLayer::FileFormatArguments& arguments);

/// Returns true if the given layer \p identifier contains any file
/// format arguments.
bool Sdf_IdentifierContainsArguments(
    const std::string& identifier);

/// Returns the display name for the layer with the given identifier.
/// The identifier may be an anonymous layer identifier, in which case
/// Sdf_GetAnonLayerDisplayName is called.
std::string Sdf_GetLayerDisplayName(
    const std::string& identifier);

#endif // SDF_ASSET_PATH_RESOLVER_H
