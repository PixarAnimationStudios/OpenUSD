//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_INTROSPECTION_H
#define PXR_USD_USD_UTILS_INTROSPECTION_H

/// \file usdUtils/introspection.h
///
/// Collection of module-scoped utilities for introspecting a given USD stage.
/// Future additions might include full-on dependency extraction, queries like
/// "Does this stage contain this asset?", "usd grep" functionality, etc.

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/vt/dictionary.h"

PXR_NAMESPACE_OPEN_SCOPE


SDF_DECLARE_HANDLES(SdfLayer);

#define USDUTILS_USDSTAGE_STATS         \
    (approxMemoryInMb)                  \
    (totalPrimCount)                    \
    (modelCount)                        \
    (instancedModelCount)               \
    (assetCount)                        \
    (prototypeCount)                    \
    (totalInstanceCount)                \
    (usedLayerCount)                    \
    (primary)                           \
    (prototypes)                        \
    (primCounts)                        \
        /*(totalPrimCount)*/            \
        (activePrimCount)               \
        (inactivePrimCount)             \
        (pureOverCount)                 \
        (instanceCount)                 \
    (primCountsByType)                  \
        (untyped)

TF_DECLARE_PUBLIC_TOKENS(UsdUtilsUsdStageStatsKeys,
                         USDUTILS_API, USDUTILS_USDSTAGE_STATS);

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
///  * prototypeCount - number of prototypes
///  * totalInstanceCount - total number of instances (including nested instances)
///  * two sub-dictionaries, 'primary' and 'prototypes' for the "primary" prim tree
///  and for all the prototype subtrees respectively, containing the following 
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
/// The "prototypes" subdictionary is populated only if the stage has one or more 
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
USDUTILS_API
UsdStageRefPtr UsdUtilsComputeUsdStageStats(const std::string &rootLayerPath, 
                                            VtDictionary *stats);

/// \overload
/// Computes stats on an already opened USD stage.
/// 
/// Returns the total number of prims on the stage, including active, inactive.
/// pure overs, prims inside prototypes etc.
/// 
USDUTILS_API
size_t UsdUtilsComputeUsdStageStats(const UsdStageWeakPtr &stage, 
                                    VtDictionary *stats);


PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_USD_USD_UTILS_INTROSPECTION_H */
