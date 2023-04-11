//
// Copyright 2023 Pixar
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
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/trace/trace.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/usdShade/udimUtils.h"

static const char UDIM_PATTERN[] = "<UDIM>";
static const int UDIM_START_TILE = 1001;
static const int UDIM_END_TILE = 1100;
static const std::string::size_type UDIM_TILE_NUMBER_LENGTH = 4;

PXR_NAMESPACE_OPEN_SCOPE


// Split a udim file path such as /someDir/myFile.<UDIM>.exr into a
// prefix (/someDir/myFile.) and suffix (.exr).
//
// We might support other patterns such as /someDir/myFile._MAPID_.exr
// in the future.
static
std::pair<std::string, std::string>
_SplitUdimPattern(const std::string &path)
{
    static const std::vector<std::string> patterns = { UDIM_PATTERN };

    for (const std::string &pattern : patterns) {
        const std::string::size_type pos = path.find(pattern);
        if (pos != std::string::npos) {
            return { path.substr(0, pos), path.substr(pos + pattern.size()) };
        }
    }

    return { std::string(), std::string() };
}

/* static */
bool
UsdShadeUdimUtils::IsUdimIdentifier(const std::string &identifier)
{
    const std::pair<std::string, std::string> 
        splitPath = _SplitUdimPattern(identifier);
    return !(splitPath.first.empty() && splitPath.second.empty());
}

// Given a udim path and layer, this function will split the path and then
// attempt to resolve all potential udim files that may match.  Returning
// a pair containing the path and the tile number provides additional
// flexibility when working with the results downstream by preventing
// users from having to re-split the resolved path if the tile part is needed.
static
std::vector<UsdShadeUdimUtils::ResolvedPathAndTile> _ResolveUdimPaths(
    const std::string &udimPath, 
    const SdfLayerHandle& layer,
    bool stopAtFirst)
{
    TRACE_FUNCTION();

    std::vector<UsdShadeUdimUtils::ResolvedPathAndTile> resolvedPaths;

    // Check for bookends, and exit early if it's not a UDIM path
    const std::pair<std::string, std::string>
        splitPath = _SplitUdimPattern(udimPath);
    if (splitPath.first.empty() && splitPath.second.empty()) {
        return resolvedPaths;
    }

    ArResolver& resolver = ArGetResolver();
    
    for (int i = UDIM_START_TILE; i < UDIM_END_TILE; i++) {
        const std::string tile = std::to_string(i);

        // Fill in integer
        std::string path = splitPath.first + tile + splitPath.second;
        if (layer) {
            // Deal with layer-relative paths.
            path = SdfComputeAssetPathRelativeToLayer(layer, path);
        }

        path = resolver.Resolve(path);
        if (!path.empty()) {
            resolvedPaths.push_back(std::make_pair(path, tile));

            if (stopAtFirst) {
                break;
            }
        }
    }

    return resolvedPaths;
}
    
/* static*/
std::vector<UsdShadeUdimUtils::ResolvedPathAndTile> 
UsdShadeUdimUtils::ResolveUdimTilePaths(
    const std::string &udimPath,
    const SdfLayerHandle &layer)
{
    return _ResolveUdimPaths(udimPath, layer, /* stopAtFirst = */ false);
}

/* static */
std::string 
UsdShadeUdimUtils::ResolveUdimPath(
    const std::string &udimPath,
    const SdfLayerHandle& layer)
{
    // Return empty if passed path is a non-UDIM path or just doesn't 
    // resolve as a UDIM
    std::vector<ResolvedPathAndTile> udimPaths = 
        _ResolveUdimPaths(udimPath, layer, /* stopAtFirst = */ true);

    if (udimPaths.empty()) {
        return std::string();
    }

    std::pair<std::string, std::string> splitPath = _SplitUdimPattern(udimPath);
    
    // Just need first tile to verify and then revert to <UDIM>
    std::string firstTilePackage;
    std::string firstTilePath = udimPaths[0].first;

    // If the resolved path of the first tile is located in a packaged asset,
    // like /foo/bar/baz.usdz[myImage.0001.exr], we need to separate the
    // paths to restore the "<UDIM>" prefix to the image filename in the
    // code below, then join the path back togther before we return.
    if (ArIsPackageRelativePath(firstTilePath)) {
        std::tie(firstTilePackage, firstTilePath) =
            ArSplitPackageRelativePathInner(firstTilePath);
    }

    // Construct the file path /filePath/myImage.<UDIM>.exr by using
    // the first part from the first resolved tile, "<UDIM>" and the
    // suffix.
    const std::string suffix = _SplitUdimPattern(udimPath).second;

    // Sanity check that the part after <UDIM> did not change.
    if (!TfStringEndsWith(firstTilePath, suffix)) {
        TF_WARN(
            "Resolution of first udim tile gave ambigious result. "
            "First tile for '%s' is '%s'.",
            udimPath.c_str(), firstTilePath.c_str());
        return std::string();
    }

    // Length of the part /filePath/myImage.<UDIM>.exr.
    const std::string::size_type prefixLength =
        firstTilePath.size() - suffix.size() - UDIM_TILE_NUMBER_LENGTH;

    firstTilePath = 
        firstTilePath.substr(0, prefixLength) + UDIM_PATTERN + suffix;

    return firstTilePackage.empty() 
        ? firstTilePath 
        : ArJoinPackageRelativePath(firstTilePackage, firstTilePath);
}

/* static */
std::string 
UsdShadeUdimUtils::ReplaceUdimPattern(
    const std::string &identifierWithPattern,
    const std::string &replacement)
{
    const std::pair<std::string, std::string>
        splitPath = _SplitUdimPattern(identifierWithPattern);
    if (splitPath.first.empty() && splitPath.second.empty()) {
        return identifierWithPattern;
    }

    return splitPath.first + replacement + splitPath.second;
}

PXR_NAMESPACE_CLOSE_SCOPE

