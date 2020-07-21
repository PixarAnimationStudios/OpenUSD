//
// Copyright 2020 Pixar
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
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"

#include "pxr/usd/ar/resolver.h"

#include "pxr/usd/usd/attribute.h"

#include "pxr/usd/sdf/layerUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

static const char UDIM_PATTERN[] = "<UDIM>";
static const int UDIM_START_TILE = 1001;
static const int UDIM_END_TILE = 1100;
static const std::string::size_type UDIM_TILE_NUMBER_LENGTH = 4;

// We need to find the first layer that changes the value
// of the parameter so that we anchor relative paths to that.
static
SdfLayerHandle 
_FindLayerHandle(const UsdAttribute& attr, const UsdTimeCode& time) {
    for (const auto& spec: attr.GetPropertyStack(time)) {
        if (spec->HasDefaultValue() ||
            spec->GetLayer()->GetNumTimeSamplesForPath(
                spec->GetPath()) > 0) {
            return spec->GetLayer();
        }
    }
    return TfNullPtr;
}

// Given the prefix (e.g., //SHOW/myImage.) and suffix (e.g., .exr),
// add integer between them and try to resolve. Iterate until
// resolution succeeded.
static
std::string
_ResolvedPathForFirstTile(
    const std::pair<std::string, std::string> &splitPath,
    SdfLayerHandle const &layer)
{
    TRACE_FUNCTION();

    ArResolver& resolver = ArGetResolver();
    
    for (int i = UDIM_START_TILE; i < UDIM_END_TILE; i++) {
        // Fill in integer
        std::string path =
            splitPath.first + std::to_string(i) + splitPath.second;
        if (layer) {
            // Deal with layer-relative paths.
            path = SdfComputeAssetPathRelativeToLayer(layer, path);
        }
        // Resolve
        path = resolver.Resolve(path);
        if (!path.empty()) {
            return path;
        }
    }
    return std::string();
}

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

// If given assetPath contains UDIM pattern, resolve the UDIM pattern.
// Otherwise, leave assetPath untouched.
static
SdfAssetPath
_ResolveAssetAttribute(
    const SdfAssetPath& assetPath,
    const UsdAttribute& attr,
    const UsdTimeCode& time)
{
    TRACE_FUNCTION();

    // See whether the asset path contains UDIM pattern.
    const std::pair<std::string, std::string>
        splitPath = _SplitUdimPattern(assetPath.GetAssetPath());

    if (splitPath.first.empty() && splitPath.second.empty()) {
        // Leave untouched.
        return assetPath;
    }

    // Find first tile.
    const std::string firstTilePath =
        _ResolvedPathForFirstTile(splitPath, _FindLayerHandle(attr, time));

    if (firstTilePath.empty()) {
        return assetPath;
    }
    
    // Construct the file path /filePath/myImage.<UDIM>.exr by using
    // the first part from the first resolved tile, "<UDIM>" and the
    // suffix.

    const std::string &suffix = splitPath.second;

    // Sanity check that the part after <UDIM> did not change.
    if (!TfStringEndsWith(firstTilePath, suffix)) {
        TF_WARN(
            "Resolution of first udim tile gave ambigious result. "
            "First tile for '%s' is '%s'.",
            assetPath.GetAssetPath().c_str(), firstTilePath.c_str());
        return assetPath;
    }

    // Length of the part /filePath/myImage.<UDIM>.exr.
    const std::string::size_type prefixLength =
        firstTilePath.size() - suffix.size() - UDIM_TILE_NUMBER_LENGTH;

    return
        SdfAssetPath( 
            assetPath.GetAssetPath(),
            firstTilePath.substr(0, prefixLength) + UDIM_PATTERN + suffix);
}

VtValue
UsdImaging_ResolveMaterialParamValue(
    const UsdAttribute& attr,
    const UsdTimeCode& time)
{
    TRACE_FUNCTION();

    VtValue value;

    if (!attr.Get(&value, time)) {
        return value;
    }
    
    if (!value.IsHolding<SdfAssetPath>()) {
        return value;
    }

    return VtValue(
        _ResolveAssetAttribute(
            value.UncheckedGet<SdfAssetPath>(), attr, time));
}

PXR_NAMESPACE_CLOSE_SCOPE
