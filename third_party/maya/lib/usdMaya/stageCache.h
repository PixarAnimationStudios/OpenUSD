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
#ifndef PXRUSDMAYA_STAGECACHE_H
#define PXRUSDMAYA_STAGECACHE_H

/// \file usdMaya/stageCache.h

#include "pxr/pxr.h"

#include "usdMaya/api.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stageCache.h"

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


class UsdMayaStageCache
{
public:

    /// Return the singleton stage cache for use by all USD clients within Maya.
    /// 2 stage caches are maintained; 1 for stages that have been
    /// force-populated, and 1 for stages that have not been force-populated.
    PXRUSDMAYA_API
    static UsdStageCache& Get(const bool forcePopulate=true);

    /// Clear the cache
    PXRUSDMAYA_API
    static void Clear();

    /// Erase all stages from the stage caches whose root layer path is
    /// \p layerPath.
    ///
    /// The number of stages erased from the caches is returned.
    PXRUSDMAYA_API
    static size_t EraseAllStagesWithRootLayerPath(
            const std::string& layerPath);

    /// Gets (or creates) a shared session layer tied with the given variant
    /// selections and draw mode on the given root path.
    /// The stage is cached for the lifetime of the current Maya scene.
    static SdfLayerRefPtr GetSharedSessionLayer(
            const SdfPath& rootPath,
            const std::map<std::string, std::string>& variantSelections,
            const TfToken& drawMode);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYA_STAGECACHE_H
