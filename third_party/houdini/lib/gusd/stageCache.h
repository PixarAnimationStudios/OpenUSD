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
#include <UT/UT_Error.h>
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

    GusdStageCache(const GusdStageCache&) = delete;

    GusdStageCache& operator=(const GusdStageCache&) = delete;

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
    /// Stages and layers may be reloaded during an active sessions, but it's
    /// important to understand the full implications of doing so.
    /// When a layer is reloaded, change notifications are sent to any stages
    /// referncing that layer, causing those stages to recompose, if necessary.
    /// This operation is not thread-safe, and may result in a crash if another
    /// thread is attempting to read from an affected stage at the same time.
    /// Further, it must be noted that simply loading stages within separate
    /// GusdStageCache instances also does not mean that that change
    /// propopagation will be isolated only to stages of the stage cache
    /// instance: Although it is possible to isolate the effect of changes
    /// on the root layers of stages to some extent, secondary layers -- such
    /// as sublayers and reference arcs -- are shared on a global cache.
    /// The effect of reloading layers is _global_ and _immediate_.
    ///
    /// Rather than attempting to solve this problem with intrusive and
    /// expensive locking -- which would only solve the problem for stages
    /// held internally in a GusdStageCache, not for stages referenced from
    /// other caches -- we prefer to address the problem by requiring that
    /// reloading only be performed at certain points of Houdini's main event
    /// loop, where it known to be safe.
    /// An example of a 'safe' way to exec stage reloads is via a callback
    /// triggered by a button in a node's GUI.
    /// Users should never attempt to reload stages or layers during node
    /// cook methods.

    /// Mark a set of stages for reload on the event queue.
    static void ReloadStages(const UT_Set<UsdStagePtr>& stages);

    /// Mark a set of layers for reload on the event queue.
    static void ReloadLayers(const UT_Set<SdfLayerHandle>& layers);


private:
    class _MaskedStageCache;

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

    GusdStageCacheReader(const GusdStageCacheReader&) = delete;

    GusdStageCacheReader& operator=(const GusdStageCacheReader&) = delete;

    ~GusdStageCacheReader();

    /// Find an existing stage on the cache.
    UsdStageRefPtr
    Find(const UT_StringRef& path,
         const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
         const GusdStageEditPtr& edit=nullptr) const;
    
    /// Return a stage from the cache, if one exists.
    /// If not, attempt to open the stage and add it to the cache.
    /// If \p path is a non-empty path and stage opening fails, errors
    /// are reporting to the currently scoped error manager at a severity
    /// of \p sev.
    UsdStageRefPtr
    FindOrOpen(const UT_StringRef& path,
               const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
               const GusdStageEditPtr& edit=nullptr,
               UT_ErrorSeverity sev=UT_ERROR_ABORT);

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
    /// This use of masking may be disabled by way of the GUSD_STAGEMASK_ENABLE
    /// environment variable, but beware that doing so may significantly degrade
    /// performance for certain access patterns, such as if many separate prims
    /// are being queried from the cache with different stage edits.
    ///
    /// \subection Primitive Encapsulation
    ///
    /// Because primitives are masked to include a subset of a stage,
    /// there is an expectation that the caller follows _encapsulation_ rules.
    /// When we read in a prim, we consider that prim to be encapsulated,
    /// which means that if any other primitives from the stage are required
    /// to process an encapsulated primitive, they are expected to either
    /// be descendants or ancestors of the encapsulated prim, or the dependency
    /// to that external prim must be discoverable using either relationships
    /// or attribute connections.
    /// Following those encapsulation rules, neither _siblings_ of the prim
    /// being requested, or other prims in separate branches of the stage
    /// are guaranteed to be loaded. Any attempt to reach other prims that
    /// can't be discovered using the above rules for discovering dependencies
    /// may either fail or introduce non-deterministic behavior.

    /// Get a prim from the cache, on a masked stage.
    /// If \p path and \p primPath are both valid, and either a stage load
    /// error occurs or no prim can be found, errors are reported on the
    /// currently scoped error manager at a severity of \p sev.
    PrimStagePair
    GetPrim(const UT_StringRef& path,
            const SdfPath& primPath,
            const GusdStageEditPtr& stageEdit=GusdStageEditPtr(),
            const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
            UT_ErrorSeverity sev=UT_ERROR_ABORT);

    /// Get multiple prims from the cache (in parallel).
    /// If the configured error severity is less than UT_ERROR_ABORT,
    /// prim loading will continue even after load errors have occurred.
    /// If any stage load errors occur, or if any prims cannot be found, errors
    /// are reported on the currently scoped error manager with a severity of
    /// \p sev. If \p sev is less than UT_ERROR_ABORT, prim loading will
    /// continue even when errors occur for some prims. Otherwise, loading
    /// aborts upon the first error.
    bool
    GetPrims(const GusdDefaultArray<UT_StringHolder>& filePaths,
             const UT_Array<SdfPath>& primPaths,
             const GusdDefaultArray<GusdStageEditPtr>& edits,
             UsdPrim* prims,
             const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
             UT_ErrorSeverity sev=UT_ERROR_ABORT);

    /// Get a prim from the cache, given a prim path that may contain
    /// variant selections. This is a convenience for the common case
    /// of accessing a prim given parameters for just a file path and
    /// prim path.
    /// @{
    PrimStagePair
    GetPrimWithVariants(const UT_StringRef& path,
                        const SdfPath& primPath,
                        const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
                        UT_ErrorSeverity sev=UT_ERROR_ABORT);

    PrimStagePair
    GetPrimWithVariants(const UT_StringRef& path,
                        const UT_StringRef& primPath,
                        const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
                        UT_ErrorSeverity sev=UT_ERROR_ABORT);

    /// Different variations of the above the variants are stored separately.
    PrimStagePair
    GetPrimWithVariants(const UT_StringRef& path,
                        const SdfPath& primPath,
                        const SdfPath& variants,
                        const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
                        UT_ErrorSeverity sev=UT_ERROR_ABORT);

    PrimStagePair
    GetPrimWithVariants(const UT_StringRef& path,
                        const UT_StringRef& primPath,
                        const UT_StringRef& varaints,
                        const GusdStageOpts& opts=GusdStageOpts::LoadAll(),
                        UT_ErrorSeverity sev=UT_ERROR_ABORT);
    /// @}

protected:
    GusdStageCacheReader(GusdStageCache& cache, bool writer);

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

    GusdStageCacheWriter(const GusdStageCacheWriter&) = delete;

    GusdStageCacheWriter& operator=(const GusdStageCacheWriter&) = delete;

    /// Find all stages on the cache matching the given paths.
    /// Multiple stages may be found for each path.
    void    FindStages(const UT_StringSet& paths,
                       UT_Set<UsdStageRefPtr>& stages);

    /// \section GusdStageCacheWriter_ReloadAndClear Reloading And Clearing
    ///
    /// During active sessions, the contents of a cache may be refreshed
    /// by either reloading a subset of the stages that it contains, or by
    /// removing stage entries from the cache.
    /// In either case, if a stage is reloaded or evicted from the cache,
    /// and if that stage has a micro node
    /// (see: \ref GusdStageCacheReader::GetMicroNode), then that micro
    /// node, and any OP_Node instances that reference it, are dirtied.
    /// This means that any nodes whose cook is based on data from a cached
    /// stage will properly update in response to Clear/Reload actions.
    ///
    /// \warn Dirty state propagation is not thread safe, and should only be
    /// called at a safe point on the main thread, such as through a callback
    /// triggered by a UI button. Also note that there may be side effects
    /// from reloading stages that affect stages from *other caches*. See
    /// \ref GusdStageCache_Reloading for more information on the caveats of
    /// reloading.
    
    /// Clear out all cached items.
    /// Note that layers are owned by a different cache, and may stay
    /// active beyond this point.
    void    Clear();

    /// Variant of Clear() that causes any stages whose root layer has
    /// an asset path in the \p paths set to be removed from the cache.
    void    Clear(const UT_StringSet& paths);

    /// Reload all stages matching the given paths.
    void    ReloadStages(const UT_StringSet& paths);

};


PXR_NAMESPACE_CLOSE_SCOPE


#endif /*_GUSD_STAGECACHE_H_*/
