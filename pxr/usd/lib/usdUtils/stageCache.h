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
#ifndef USDUTILS_STAGECACHE_H
#define USDUTILS_STAGECACHE_H

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

#endif /* USDUTILS_STAGECACHE_H */
