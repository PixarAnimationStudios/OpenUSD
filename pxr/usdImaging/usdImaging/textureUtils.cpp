//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/textureUtils.h"

#include "pxr/base/tf/fileUtils.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverScopedCache.h"

#include "pxr/usd/sdf/layerUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

std::vector<std::tuple<int, TfToken>>
UsdImaging_GetUdimTiles(
    std::string const& basePath,
    int tileLimit,
    SdfLayerHandle const& layerHandle) {
    const std::string::size_type pos = basePath.find("<UDIM>");
    if (pos == std::string::npos) {
        return {};
    }
    std::string formatString = basePath;
    formatString.replace(pos, 6, "%i");

    ArResolverScopedCache resolverCache;
    ArResolver& resolver = ArGetResolver();

    constexpr int startTile = 1001;
    const int endTile = startTile + tileLimit;
    std::vector<std::tuple<int, TfToken>> ret;
    ret.reserve(tileLimit);
    for (int t = startTile; t <= endTile; ++t) {
        const std::string path =
            layerHandle
            ? SdfComputeAssetPathRelativeToLayer(
                layerHandle, TfStringPrintf(formatString.c_str(), t))
            : TfStringPrintf(formatString.c_str(), t);
        if (!resolver.Resolve(path).empty()) {
            ret.emplace_back(t - startTile, TfToken(path));
        }
    }
    ret.shrink_to_fit();
    return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
