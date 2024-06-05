//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_STAGE_CACHE_H
#define PXR_USD_USD_UTILS_STAGE_CACHE_H

/// \file usdUtils/stageCache.h
/// A simple interface for handling a singleton usd stage cache.

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/usd/stageCache.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(SdfLayer);

/// \class UsdUtilsStageCache
///
/// The UsdUtilsStageCache class provides a simple interface for handling a
/// singleton usd stage cache for use by all USD clients. This way code from
/// any location can make use of the same cache to maximize stage reuse.
///
class UsdUtilsStageCache {
public:

    /// Returns the singleton stage cache.
    USDUTILS_API
    static UsdStageCache &Get();

    /// Given variant selections as a vector of pairs (vector in case order
    /// matters to the client), constructs a session layer with overs on the
    /// given root modelName with the variant selections, or returns a cached
    /// session layer with those opinions.
    USDUTILS_API
    static SdfLayerRefPtr GetSessionLayerForVariantSelections(
        const TfToken& modelName,
        const std::vector<std::pair<std::string, std::string> > &variantSelections);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_USD_USD_UTILS_STAGE_CACHE_H */
