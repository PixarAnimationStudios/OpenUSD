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
#ifndef _USDUTILS_PINTROSPECTION_H_
#define _USDUTILS_PINTROSPECTION_H_

/// \file usdUtils/introspection.h
///
/// Collection of module-scoped utilities for introspecting a given USD stage.
/// Future additions might include full-on dependency extraction, queries like
/// "Does this stage contain this asset?", "usd grep" functionality, etc.

#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/vt/dictionary.h"

SDF_DECLARE_HANDLES(SdfLayer);

#define USDUTILS_USDSTAGE_STATS         \
    (approxMemoryInMb)                  \
    (totalPrimCount)                    \
    (modelCount)                        \
    (instancedModelCount)               \
    (assetCount)                        \
    (masterCount)                       \
    (totalInstanceCount)                \
    (usedLayerCount)                    \
    (primary)                           \
    (masters)                           \
    (primCounts)                        \
        /*(totalPrimCount)*/            \
        (activePrimCount)               \
        (inactivePrimCount)             \
        (pureOverCount)                 \
        (instanceCount)                 \
    (primCountsByType)                  \
        (untyped)

TF_DECLARE_PUBLIC_TOKENS(UsdUtilsUsdStageStatsKeys, USDUTILS_USDSTAGE_STATS);

/// Opens the given layer on a USD stage and collects various stats. 
/// The stats are populated in the dictionary-valued output param \p stats.
/// 
/// The set of stats include:
///  * approxMemoryInMb - approximate memory allocated when opening the stage 
///  with all the models loaded. 
///  * totalPrimCount - total number of prims
///  * modelCount - number of models
///  * instancedModelCount - number of instanced models
///  * assetCount - number of assets
///  * masterCount - number of masters
///  * totalInstanceCount - total number of instances (including nested instances)
///  * two sub-dictionaries, 'primary' and 'masters' for the "primary" prim tree
///  and for all the master subtrees respectively, containing the following 
///  stats:
///  * primCounts - a sub-dictionary containing the following 
///     * totalPrimCount - number of prims
///     * activePrimCount - number of active prims
///     * inactivePrimCount - number of inactive prims
///     * pureOverCount - number of pure overs
///     * instanceCount - number of instances
///  * primCountsByType - a sub-dictionary containing prim counts keyed by the 
///  prim type.
///
/// Returns the stage that was opened.
/// 
/// The "masters" subdictionary is populated only if the stage has one ore more 
/// instanced models.
/// 
/// \note The approximate memory allocated when opening the stage is computed 
/// and reported *only* if the TfMallocTag system has already been initialized 
/// by the client, and the number will represent only *additional* consumed 
/// memory, so if some of the layers the stage uses are already open, the true 
/// memory consumption for the stage may be higher than reported.
/// \sa TfMallocTag::IsInitialized()
///
/// \note Only component models are included in 'modelCount' and 
/// 'instancedModelCount'.
///
UsdStageRefPtr UsdUtilsComputeUsdStageStats(const std::string &rootLayerPath, 
                                            VtDictionary *stats);

/// \overload
/// Computes stats on an already opened USD stage.
/// 
/// Returns the total number of prims on the stage, including active, inactive.
/// pure overs, prims inside masters etc.
/// 
size_t UsdUtilsComputeUsdStageStats(const UsdStageWeakPtr &stage, 
                                    VtDictionary *stats);

#endif /* _USDUTILS_INTROSPECTION_H_ */
