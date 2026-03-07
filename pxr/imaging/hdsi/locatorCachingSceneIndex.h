//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_LOCATOR_CACHING_SCENE_INDEX
#define PXR_IMAGING_HDSI_LOCATOR_CACHING_SCENE_INDEX

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/usd/sdf/pathTable.h"
#include "pxr/base/tf/denseHashMap.h"
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdsiLocatorCachingSceneIndex);

///
/// \class HdsiLocatorCachingSceneIndex
///
/// A scene index that caches data sources for specified locators
/// on specified prim types.  This avoids redundant computation
/// when scene indexes query the same locator on the same prims
/// multiple times.
///
/// \section cachingStrategies Caching Scenarios in Hydra Scene Indexes
///
/// The basic purpose of Hydra scene indexes is to transmit
/// scene information to a renderer.  In the simplest idealized
/// form this can be thought of as a for() loop over the scene
/// contents.  In this single-traversal scenario, there is no need
/// for scene indexes to internally cache data that would be visisted
/// only once.  In practice, however, several situations motivate
/// some degree of caching.
///
/// The first is when scene indexes access data in other prims.
/// A common example is resolving material bindings, which reads
/// information from the material bound to each geometry prim.
/// If the same material is bound to multiple prims it will be read
/// multiple times.  This pattern occurs in any binding-like concept.
///
/// Another inter-prim dependency situation is flattening.  Hydra scene
/// indexes present flat data to the renderer, without further implied
/// hierarchical transformation of data.  Since many input scene
/// descriptions heavily rely on hierarchical structure of data,
/// such as for xforms, a flattening scene index is used.  In this
/// situation, sibling prims access a shared parent multiple times.
///
/// A third case is repeated access to the same data source by
/// distinct scene indexes in sequence.  For example, there might
/// be a series of scene indexes that analyze and modify a material
/// network.  In such cases, it may be preferable to cache intermediate
/// state(s) of the material network to avoid re-running a growing
/// series of transformations as downstream scene indexes each
/// traverse the incoming material network.
///
/// A fourth case is interactive use when locator-based invalidation
/// granularity is insufficient to avoid repeated computation.
/// For example, some consumers of a scene index may not be
/// able to retain full granularity of individual primvar updates,
/// and instead collapse all primvar invalidation into a single
/// "dirty bit".  Observers of the dirty bit will have no choice
/// but to re-query all primvars, widening the computation.
/// Caching can avoid incurring undue extra cost.
///
/// In each of these situations, the appropriate scope of cache
/// is distinct.  This is why Hydra does not provide built-in
/// "one size fits all" caching and instead encourages specific
/// approaches based on the need.
///
///
/// \section cachingSceneIndex Cache Implementation with HdsiLocatorCachingSceneIndex
///
/// HdsiLocatorCachingSceneIndex provides a response to these needs.
/// It is a configurable, targeted cache to support specific cases
/// where memoizing multiple-access to the same data is appropriate.
///
/// Scene index plugins should insert HdsiLocatorCachingSceneIndex instances,
/// configured appropriately, based on the known structure of their
/// computations: either caching input dependencies or their outputs.
///
/// For computations that pull on upstream prims from scene-defined paths,
/// such as material bindings, it may be appropriate to insert a caching
/// index immedately before the resolving scene index, configured
/// according to the prim types and locators it depends upon.
///
/// In other cases it may be appropriate to append a caching index
/// directly after the scene index producing an expensive result,
/// if that result is expected to be accessed broadly or repeatedly.
///
/// Flattening is a special case where the scene index wants to
/// compute a value on a prim, cache it, and then re-use that in
/// comptutations on child prims.  HdFlatteningSceneIndex, rather than
/// HdsiLocatorCachingSceneIndex, is appropriate for such cases.
///
///
/// \section cacheInvalidation Cache Invalidation
///
/// HdsiLocatorCachingSceneIndex relies on PrimsDirtied
/// notification to invalidate internal cache state correctly.
///
/// There are two approaches to dependency invalidation in Hydra.
///
/// The first is that a scene index may directly propagate
/// invalidation to dependent prims and locators inside its
/// PrimsDirtied notification handler.
///
/// The second is that a scene index can overlay the
/// HdDependencySchema, and express its dependencies in
/// that form.  This approach relies on a downsteam
/// HdDependencyForwardingSceneIndex to observe those dependnecies,
/// and propagate invalidation to dependent prims and locators
/// based on the provided dependency data.
///
/// Since HdsiLocatorCachingSceneIndex has no way of knowing
/// what approach is used by upstream scene indexes, a
/// straightforward way to achieve correct cache invalidation is
/// by inserting a HdDependencyForwardingSceneIndex directly before
/// the HdsiLocatorCachingSceneIndex in the scene index chain.
/// The method AddDependencyForwardingAndLocatorCache() does this.
///
/// Note that this does rely on upstream scene indexes properly
/// implementing one or the other of the two approaches above.
///
///
/// \section internalState Internal Runtime State Caching
///
/// The discussion above pertains to caching the scene index data
/// produced by a scene index and seen by observers.  Some scene
/// index implementations contain internal runtime state (as C++
/// member variables or thread-safe/protected global state).
///
/// Maintaining integrity of internal runtime cache state is the
/// responsibility of the scene index, and this will require handling
/// inside change notification methods.  Such state is not addressed
/// by the dependency schema or HdDependencyForwardingSceneIndex.
///
class HdsiLocatorCachingSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// Creates a new HdDependencyForwardingSceneIndex followed by a
    /// HdsiLocatorCachingSceneIndex.
    ///
    /// \see New() for parameter documentation
    HD_API
    static HdSceneIndexBaseRefPtr
    AddDependencyForwardingAndCache(
        HdSceneIndexBaseRefPtr const& inputScene,
        HdDataSourceLocator const& locatorToCache,
        TfToken const& primTypeToCache);

    /// Creates a new HdsiLocatorCachingSceneIndex.
    ///
    /// \note AddDependencyForwardingAndLocatorCache() is recommended
    /// for most uses.  See class notes regarding cache invalidation.
    ///
    /// \param inputScene The upstream input scene index to cache
    /// \param locatorToCache The data source locator to cache;
    /// for example, HdMaterialSchema::GetDefaultLocator().
    /// Must not be empty.
    /// \param primTypeToCache If specified, caching only applies
    /// to prims of this data type; if empty token, caching applies
    /// to all prim types
    HD_API
    static HdsiLocatorCachingSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const& inputScene,
        HdDataSourceLocator const& locatorToCache,
        TfToken const& primTypeToCache);

    HD_API
    ~HdsiLocatorCachingSceneIndex() override;

    HD_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HD_API 
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HD_API
    HdsiLocatorCachingSceneIndex(
        HdSceneIndexBaseRefPtr const& inputScene,
        HdDataSourceLocator const& locatorToCache,
        TfToken const& primTypeToCache);

    void _PrimsAdded(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsRemoved(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    void _PrimsDirtied(
            const HdSceneIndexBase &sender,
            const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    // A data source that delegates back to HdsiLocatorCachingSceneIndex, allowing
    // it to manage invalidation.
    class _DataSource : public HdContainerDataSource
    {
    public:
        HD_DECLARE_DATASOURCE(_DataSource);

        TfTokenVector GetNames() override;
        HdDataSourceBaseHandle Get(const TfToken &name) override;

    private:
        _DataSource(
            HdsiLocatorCachingSceneIndex const* cachingSceneIndex,
            SdfPath const& primPath,
            HdDataSourceLocator const& locator,
            HdContainerDataSourceHandle const& inputDataSource);

        const HdsiLocatorCachingSceneIndex& _cachingSceneIndex;
        const SdfPath _primPath;
        const HdDataSourceLocator _locator;
        const HdContainerDataSourceHandle _inputDataSource;
    };

    // Retrieve data for the requested locator, populating and
    // re-using cached data sources as appropriate.
    HdDataSourceBaseHandle
    _GetWithCache(
        SdfPath const& primPath,
        HdDataSourceLocator const& locator,
        HdContainerDataSourceHandle const& inputDataSource) const;

private: // data

    // Cache of data sources within a prim, by locator
    using _LocatorDataSourceMap =
        TfDenseHashMap<HdDataSourceLocator, HdDataSourceBaseHandle, TfHash>;

    // Cache of data sources, by prim
    using _PrimCache = SdfPathTable<_LocatorDataSourceMap>;

    // Cache strategy parameters.
    const HdDataSourceLocator _locatorToCache;
    const TfToken _primTypeToCache;

    // Cache contents.
    mutable _PrimCache _primDsCache;
    mutable std::mutex _cacheMutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
