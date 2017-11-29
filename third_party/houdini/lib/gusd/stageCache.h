//
// Copyright 2017 Pixar
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
#ifndef _GUSD_STAGECACHE_H_
#define _GUSD_STAGECACHE_H_

#include <UT/UT_Array.h>
#include <UT/UT_Set.h>

#include "gusd/defaultArray.h"
#include "gusd/stageEdit.h"
#include "gusd/stageOpts.h"
#include "gusd/USD_Utils.h"

#include <pxr/pxr.h>
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/stage.h"


class DEP_MicroNode;
class UT_StringHolder;
class UT_StringSet;


PXR_NAMESPACE_OPEN_SCOPE

class GusdUSD_DataCache;
class UsdPrim;

/// Cache for USD stages.
/// Clients interact with the cache via the GusdStageCacheReader
/// and GusdStageCacheWriter classes.
class GUSD_API GusdStageCache
{
public:
    GusdStageCache();

    ~GusdStageCache();

    static GusdStageCache&  GetInstance();

    /// Add/remove auxiliary data caches.
    /// Auxiliary data caches are cleared in response to changes
    /// to stages on this cache.
    /// @{
    void    AddDataCache(GusdUSD_DataCache& cache);

    void    RemoveDataCache(GusdUSD_DataCache& cache);
    /// @}

    /// \section GusdStageCache_Reloading Reloading
    ///
    /// Reloading stages and layers may potentially cause non-threadsafe
    /// mutations to stages held by the cache, which may cause crashes
    /// in other threads that are making use of those caches.
    /// To avoid the need for intrusive and expensive locking around all
    /// code that accesses a stage, reload operations are placed on Houdini's
    /// single-threaded event queue, where it is expected that only a single
    /// thread is accessing the stages while reload operations occur.
    ///
    /// This is the only safe way to reload stages and layers in Houdini.
    /// Users should never directly call UsdStage::Reload() on stages returned
    /// from the cache. Users also must not call SdfLayer::Reload() on any
    /// layers returned by SdfLayer::FindOrOpen(), since stages internally
    /// use such shared layers, and may be affected.

    /// Mark a set of stages for reload on the event queue.
    static void ReloadStages(const UT_Set<UsdStagePtr>& stages);

    /// Mark a set of layers for reload on the event queue.
    static void ReloadLayers(const UT_Set<SdfLayerHandle>& layers);


public://TEMP
    class _Impl;
    _Impl* const    _impl;

    friend class GusdStageCacheReader;
    friend class GusdStageCacheWriter;
};


/// Helper for reading from a GusdStageCache.
/// Cache readers can both infd existing stages on the cache,
/// as well as cause additional stages to be inserted into the cache.
/// Cache readers cannot clear out any existing stages or mutate
/// auxiliary data caches.
///
/// Example usage:
/// @code
///     GusdStageCacheReader cache;
///
///     // Pull a stage from the cache.
///     UsdStageRefPtr stage = cache.FindOrOpen(stagePath);
///
///     // Access a prim on the cache.
///     UsdPrim prim = cache.GetPrim(stagePath, primPath).first;
///
///     // Access a prim with a variant selection.
///     SdfPath primPath("/foo{variant=sel}bar");
///     UsdPrim prim = cache.GetPrim(stagePath, primPath);
/// @endcode
class GUSD_API GusdStageCacheReader
{
public:
    using PrimStagePair = std::pair<UsdPrim,UsdStageRefPtr>;

    /// Construct a reader for the cache singleton.
    GusdStageCacheReader(GusdStageCache& cache=GusdStageCache::GetInstance())
        : GusdStageCacheReader(cache, false) {}

    ~GusdStageCacheReader();

    /// Find an existing stage on the cache.
    UsdStageRefPtr
    Find(const UT_StringRef& path,
         const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
         const GusdStageEditPtr& edit=nullptr);
    
    /// Return a stage from the cache, if one exists.
    /// If not, attempt to open the stage and add it to the cache.
    UsdStageRefPtr
    FindOrOpen(const UT_StringRef& assetPath,
               const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
               const GusdStageEditPtr& edit=nullptr,
               GusdUT_ErrorContext* err=nullptr);

    /// Get a micro node for a stage.
    /// Micro nodes are created on demand, and are dirtied both for
    /// stage reloading and cache evictions.
    DEP_MicroNode*
    GetStageMicroNode(const UsdStagePtr& stage);

    /// \section Prim Accessors
    ///
    /// These helpers return prims on masked stages, where only the
    /// parts of the stage required to produce a given prim are composed.
    /// This helps support workflows such as setting variants on packed prims,
    /// where either many stage mutations may be made that conflict with each
    /// other, or in isolation, such that different mutations can't be made
    /// to share stages without intrusive locking.
    /// In all cases, if a full stage which satisfies the stage options and
    /// edits has already been loaded on the cache, the prim will fetched from
    /// that stage instead.
    ///
    /// Because primitives are masked to include a subset of a stage,
    /// there is an expectation that the caller follows encapsulation rules:
    /// Ancestor prims are guanteed to be loaded, and all descendant prims
    /// of the returned prims will also be loaded. There is no guarantee,
    /// through, that either siblings or prims elsewhere in the scene
    /// graph will be composed on the returned stage.
    /// Any dependencies on unrelated prims (not descendants or ancestors)
    /// must be defined by the presence of a relationships or attribute
    /// connections (not supported yet).
    /// Any attempt to reach implicit dependencies -- for example, a schema
    /// that accesses other non-descendant prims using naming conventions --
    /// may either fail or introduce non-deterministic behavior.

    /// Get a prim from the cache, on a maksed stage.
    PrimStagePair
    GetPrim(const UT_StringRef& path,
            const SdfPath& primPath,
            const GusdStageEditPtr& stageEdit=GusdStageEditPtr(),
            const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
            GusdUT_ErrorContext* err=nullptr);

    /// Get multiple prims from the cache (in parallel).
    /// If the configured error severity is less than UT_ERROR_ABORT,
    /// prim loading will continue even after load errors have occurred.
    bool
    GetPrims(const GusdDefaultArray<UT_StringHolder>& filePaths,
             const UT_Array<SdfPath>& primPaths,
             const GusdDefaultArray<GusdStageEditPtr>& edits,
             UsdPrim* prims,
             const GusdStageOpts opts=GusdStageOpts::LoadAll(),
             GusdUT_ErrorContext* err=nullptr);

    /// Get a prim from the cache, given a prim path that may contain
    /// variant selections. This is a convenience for the common case
    /// of accessing a prim given parameters for just a file path and
    /// prim path.
    /// @{
    PrimStagePair
    GetPrimWithVariants(const UT_StringRef& path,
                        const SdfPath& primPath,
                        const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
                        GusdUT_ErrorContext* err=nullptr);

    PrimStagePair
    GetPrimWithVariants(const UT_StringRef& path,
                        const UT_StringRef& primPath,
                        const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
                        GusdUT_ErrorContext* err=nullptr);

    /// Different variations of the above the variants are stored separately.
    PrimStagePair
    GetPrimWithVariants(const UT_StringRef& path,
                        const SdfPath& primPath,
                        const SdfPath& variants,
                        const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
                        GusdUT_ErrorContext* err=nullptr);

    PrimStagePair
    GetPrimWithVariants(const UT_StringRef& path,
                        const UT_StringRef& primPath,
                        const UT_StringRef& varaints,
                        const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
                        GusdUT_ErrorContext* err=nullptr);
    /// @}

protected:
    GusdStageCacheReader(GusdStageCache& cache, bool writer=false);

protected:
    GusdStageCache& _cache;
    const bool      _writer;
};


/// Write accessor for a stage cache.
/// Write accessors have all of the capabilities of readers,
/// and can also remove elements from the cache and manipulate
/// child data caches.
/// Writers gain exclusive locks to the cache, and should be used sparingly.
class GUSD_API GusdStageCacheWriter : public GusdStageCacheReader
{
public:
    GusdStageCacheWriter(GusdStageCache& cache=GusdStageCache::GetInstance());

    /// Clear out all cached items.
    /// If micro nodes have been instantiated for any stages, they will
    /// be dirtied when the main event queue execs.
    void    Clear();

    /// Clear out all caches corresponding to a set of stage paths.
    /// Note that layers are owned by a different cache, and may stay
    /// active beyond this point.
    void    Clear(const UT_StringSet& paths);

    /// Find all stages on the cache matching the given paths.
    /// Multiple stages may be found for each path.
    void    FindStages(const UT_StringSet& paths,
                       UT_Set<UsdStageRefPtr>& stages);

    /// Reload all stages matching the given paths.
    /// Actual reloading operations are deferred to the main event
    /// queue (\see GusdStageCache_Reloading).
    void    ReloadStages(const UT_StringSet& paths);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif /*_GUSD_STAGECACHE_H_*/
