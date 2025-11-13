//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/flattenedDataSourceProvider.h"
#include "pxr/imaging/hd/invalidatableContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/utils.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

namespace HdFlatteningSceneIndex_Impl
{

/// A cache for HdContainerDataSourceHandle that has suitable semantics
/// for flattened data sources which are stateful and can be invalidated
/// rather than dropped.
///
/// In particular, the scene index should return the same instance of the
/// flattened data source when queried for the same prim and locator multiple
/// times. If we were to return different instances for different queries, we would need
/// to invalidate all of those instances since a client can potentially hold on
/// to any of them.
///    
class _ContainerDataSourceCache
{
public:
    _ContainerDataSourceCache() { };
    
    _ContainerDataSourceCache(HdContainerDataSourceHandle const &ds)
     : _ds(_ToNonNull(ds))
    {
    }

    /// Returns cached result or nullopt if not cached.
    std::optional<HdContainerDataSourceHandle> Get()
    {
        if (auto const ds = HdDataSourceBase::AtomicLoad(_ds)) {
            return HdContainerDataSource::Cast(ds);
        } else {
            return {};
        }
    }

    /// If called concurrently, only one call will set the cache and the data
    /// source passed to all other calls will be ignored. All calls to Cache
    /// return the same result (if Invalidate was not called inbetween).
    ///
    HdContainerDataSourceHandle Cache(HdContainerDataSourceHandle const &ds)
    {
        HdDataSourceBaseHandle const newDs = _ToNonNull(ds);
        
        HdDataSourceBaseAtomicHandle existingDs;
        if (HdDataSourceBase::AtomicCompareExchange(_ds, existingDs, newDs)) {
            return HdContainerDataSource::Cast(newDs);
        } else {
            return HdContainerDataSource::Cast(existingDs);
        }
    }

    void Invalidate()
    {
        HdDataSourceBase::AtomicStore(_ds, nullptr);
    }
    
private:
    /// Turn null ptr to non-container data source handle to distinguish
    /// between the case where we have not cached the container data source yet
    /// and the case where we have cached it but it is null.
    static
    HdDataSourceBaseHandle _ToNonNull(
        HdContainerDataSourceHandle const &ds)
    {
        if (ds) {
            return ds;
        } else {
            return HdRetainedTypedSampledDataSource<bool>::New(false);
        }
    }
    
    HdDataSourceBaseAtomicHandle _ds;    
};
    
/// wraps the input scene's prim-level data sources in order to deliver
/// overriden value
class _PrimLevelWrappingDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimLevelWrappingDataSource);
    
    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    // Invalidate data sources for prim.
    //
    // The dirtied locators are given by going along
    // HdFlatteningSceneIndex::GetFlattenedDataSourceNames() and
    // relativeDirtyLocators in parallel and prepending the
    // name to the locators in the set.
    //
    // Recall that this prim stores a flattened data source for
    // each name in HdFlatteningSceneIndex::GetFlattenedDataSourceNames().
    //
    // If the corresponding set in relativeDirtyLocators is empty,
    // that flattened data source is untouched.
    // If it is the universal set, the flattened data source gets
    // dropped.
    // If the flattened data source supports invalidation, invalidation
    // is applied. Otherwise, the data source gets dropped.
    //
    // Returns true if any flattened data source was dropped or
    // invalidated.
    bool PrimDirtied(
        const _DataSourceLocatorSetVector &relativeDirtyLocators);

    void PrimContainerDirtied();

private:
    _PrimLevelWrappingDataSource(
        const HdFlatteningSceneIndex &flatteningSceneIndex,
        const SdfPath &primPath,
        const HdSceneIndexPrim &inputPrim)
      : _flatteningSceneIndex(flatteningSceneIndex)
      , _primPath(primPath)
      , _primType(inputPrim.primType)
      , _primSourceCache(inputPrim.dataSource)
      , _computedDataSources(
          _flatteningSceneIndex.GetFlattenedDataSourceNames().size())
    {
    }

    HdContainerDataSourceHandle _GetPrimSource();

    const HdFlatteningSceneIndex &_flatteningSceneIndex;
    const SdfPath _primPath;
    const TfToken _primType;
    _ContainerDataSourceCache _primSourceCache;

    // Parallel to HdFlatteningSceneIndex::GetFlattenedDataSourceNames()
    TfSmallVector<_ContainerDataSourceCache, _smallVectorSize>
                                _computedDataSources;
};
    
HD_DECLARE_DATASOURCE_HANDLES(_PrimLevelWrappingDataSource);

bool
_PrimLevelWrappingDataSource::PrimDirtied(
    const _DataSourceLocatorSetVector &relativeDirtyLocators)
{
    bool anyDirtied = false;

    for (size_t i = 0; i < relativeDirtyLocators.size(); i++) {
        if (relativeDirtyLocators[i].IsEmpty()) {
            continue;
        }

        _ContainerDataSourceCache &dsCache = _computedDataSources[i];
        const std::optional<HdContainerDataSourceHandle> ds = dsCache.Get();
        if (!ds) {
            continue;
        }
        if (!relativeDirtyLocators[i].Contains(
                HdDataSourceLocator::EmptyLocator())) {
            if (HdInvalidatableContainerDataSourceHandle const
                    invalidatableDs =
                        HdInvalidatableContainerDataSource::Cast(*ds)) {
                anyDirtied |=
                    invalidatableDs->Invalidate(relativeDirtyLocators[i]);
                continue;
            }
        }

        dsCache.Invalidate();
        anyDirtied = true;
    }

    return anyDirtied;
}

void
_PrimLevelWrappingDataSource::PrimContainerDirtied()
{
    _primSourceCache.Invalidate();
}
   
void
_Insert(const TfTokenVector &vec,
        TfTokenVector * const result)
{
    if (vec.size() > 31) {
        std::unordered_set<TfToken, TfHash> s(vec.begin(), vec.end());
        for (const TfToken &t : *result) {
            s.erase(t);
        }
        for (const TfToken &t : vec) {
            if (s.find(t) != s.end()) {
                result->push_back(t);
            }
        }
    } else {
        uint32_t mask = (1 << vec.size()) - 1;
        for (const TfToken &t : *result) {
            for (size_t i = 0; i < vec.size(); i++) {
                if (vec[i] == t) {
                    mask &= ~(1 << i);
                }
            }
            if (!mask) {
                return;
            }
        }
        for (size_t i = 0; i < vec.size(); i++) {
            if (mask & 1 << i) {
                result->push_back(vec[i]);
            }
        }
    }
}

TfTokenVector
_PrimLevelWrappingDataSource::GetNames()
{
    TRACE_FUNCTION();
    
    HdContainerDataSourceHandle const primSource = _GetPrimSource();

    if (!primSource) {
        return _flatteningSceneIndex.GetFlattenedDataSourceNames();
    }

    TfTokenVector result = primSource->GetNames();
    _Insert(_flatteningSceneIndex.GetFlattenedDataSourceNames(), &result);
    return result;
};        

HdDataSourceBaseHandle
_PrimLevelWrappingDataSource::Get(
        const TfToken &name)
{
    TRACE_FUNCTION();

    const TfTokenVector &dataSourceNames =
        _flatteningSceneIndex.GetFlattenedDataSourceNames();
    const HdFlattenedDataSourceProviderSharedPtrVector &providers =
        _flatteningSceneIndex.GetFlattenedDataSourceProviders();

    for (size_t i = 0; i < dataSourceNames.size(); i++) {
        if (name != dataSourceNames[i]) {
            continue;
        }

        _ContainerDataSourceCache &dsCache = _computedDataSources[i];
        if (const std::optional<HdContainerDataSourceHandle> ds =
                                        dsCache.Get()) {
            return *ds;
        }

        const HdFlattenedDataSourceProvider::Context ctx(
            _flatteningSceneIndex,
            _primPath,
            name,
            HdSceneIndexPrim{_primType, _GetPrimSource()});
        return dsCache.Cache(providers[i]->GetFlattenedDataSource(ctx));
    }

    if (HdContainerDataSourceHandle const primSource = _GetPrimSource()) {
        return primSource->Get(name);
    }
    return nullptr;
}

HdContainerDataSourceHandle
_PrimLevelWrappingDataSource::_GetPrimSource()
{
    TRACE_FUNCTION();

    if (std::optional<HdContainerDataSourceHandle> const primSource =
            _primSourceCache.Get()) {
        return *primSource;
    }

    const HdSceneIndexPrim prim =
        _flatteningSceneIndex._GetInputSceneIndex()->GetPrim(_primPath);
    return _primSourceCache.Cache(prim.dataSource);
}

}

using namespace HdFlatteningSceneIndex_Impl;

HdFlatteningSceneIndex::HdFlatteningSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene,
        HdContainerDataSourceHandle const &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputScene)
{
    if (!inputArgs) {
        return;
    }
    for (const TfToken &name : inputArgs->GetNames()) {
        using DataSource =
            HdTypedSampledDataSource<HdFlattenedDataSourceProviderSharedPtr>;
        DataSource::Handle const ds =
            DataSource::Cast(inputArgs->Get(name));
        if (!ds) {
            continue;
        }
        HdFlattenedDataSourceProviderSharedPtr const provider =
            ds->GetTypedValue(0.0f);
        if (!provider) {
            continue;
        }
        _dataSourceNames.push_back(name);
        _dataSourceProviders.push_back(provider);
        _dataSourceLocatorSet.insert(
            HdDataSourceLocator(name));
        _relativeDataSourceLocators.push_back(
            HdDataSourceLocatorSet::UniversalSet());
    }
}

HdFlatteningSceneIndex::~HdFlatteningSceneIndex() = default;

HdSceneIndexPrim
HdFlatteningSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();
    
    // Check the hierarchy cache
    const _PrimTable::const_iterator i = _prims.find(primPath);
    // SdfPathTable will default-construct entries for ancestors
    // as needed to represent hierarchy, so double-check the
    // dataSource to confirm presence of a cached prim
    if (i != _prims.end() && i->second.dataSource) {
        return i->second;
    }

    // Check the recent prims cache
    {
        // Use a scope to minimize lifetime of tbb accessor
        // for maximum concurrency
        _RecentPrimTable::const_accessor accessor;
        if (_recentPrims.find(accessor, primPath)) {
            return accessor->second;
        }
    }

    // No cache entry found; query input scene
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // If the input scene does not provide a data source, and there
    // are no descendant prims either (as implied by the lack of a
    // SdfPathTable entry in _prims), do not return anything.
    if (!prim.dataSource && i == _prims.end()) {
        return prim;
    }

    // Wrap the input datasource even when null, to support dirtying
    // down non-contiguous hierarchy
    prim.dataSource = _PrimLevelWrappingDataSource::New(
        *this, primPath, prim);

    // Store in the recent prims cache
    if (!_recentPrims.insert(std::make_pair(primPath, prim))) {
        // Another thread inserted this entry.  Since dataSources
        // are stateful, return that one.
        _RecentPrimTable::accessor accessor;
        if (TF_VERIFY(_recentPrims.find(accessor, primPath))) {
            prim = accessor->second;
        }
    }
    return prim;
}

SdfPathVector
HdFlatteningSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    // we don't change topology so we can dispatch to input
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdFlatteningSceneIndex::_PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    _ConsolidateRecentPrims();

    _AdditionalDirtiedVector dirtyEntries;
    // Check the hierarchy for cached prims to dirty
    {
        TRACE_SCOPE("Dirty hierarchy");

        WorkParallelForN(entries.size(),
        [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            const HdSceneIndexObserver::AddedPrimEntry &entry = entries[i];
            _DirtyHierarchy(
                entry.primPath,
                _relativeDataSourceLocators,
                _dataSourceLocatorSet,
                &dirtyEntries);
        }});
    }

    // Clear out any cached dataSources for prims that have been re-added.
    // They will get updated dataSources in the next call to GetPrim().
    {
        TRACE_SCOPE("Async destroy prim table entries");
        for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
            const _PrimTable::iterator i = _prims.find(entry.primPath);
            if (i != _prims.end()) {
                WorkSwapDestroyAsync(i->second.dataSource);
            }
        }
    }

    _SendPrimsAdded(entries);
    if (!dirtyEntries.empty()) {
        HdSceneIndexObserver::DirtiedPrimEntries flattened(
            dirtyEntries.begin(), dirtyEntries.end());
        _SendPrimsDirtied(flattened);
    }
}

void
HdFlatteningSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    _ConsolidateRecentPrims();

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        if (entry.primPath.IsAbsoluteRootPath()) {
            // Special case removing the whole scene, since this is a common
            // shutdown operation.
            _prims.ClearInParallel();
            TfReset(_prims);
        } else {
            auto startEndIt = _prims.FindSubtreeRange(entry.primPath);
            for (auto it = startEndIt.first; it != startEndIt.second; ++it) {
                WorkSwapDestroyAsync(it->second.dataSource);
            }
            if (startEndIt.first != startEndIt.second) {
                _prims.erase(startEndIt.first);
            }
        }
    }
    _SendPrimsRemoved(entries);
}

void
HdFlatteningSceneIndex::_PrimDirtied(
    const HdSceneIndexObserver::DirtiedPrimEntry &entry,
    _AdditionalDirtiedVector *additionalDirty) const
{
    // Used to invalidate the data sources stored in the
    // _PrimLevelWrappingDataSource.
    _DataSourceLocatorSetVector relativeDirtyLocators(
        _dataSourceNames.size());

    // Used to send out DirtiedPrimEntry for descendants.
    // Computed from relativeDirtyLocators.
    HdDataSourceLocatorSet dirtyLocators;

    for (size_t i = 0; i < _dataSourceNames.size(); i++) {
        // Check data source at locator in prim data source.
        const HdDataSourceLocator locator(_dataSourceNames[i]);
        if (!entry.dirtyLocators.Intersects(locator)) {
            // Nothing to do.
            continue;
        }

        HdDataSourceLocatorSet &relativeDirtyLocatorSet =
            relativeDirtyLocators[i];
        if (entry.dirtyLocators.Contains(locator)) {
            // Nuke the entire data source at locator.
            relativeDirtyLocatorSet = HdDataSourceLocatorSet::UniversalSet();
            dirtyLocators.insert(locator);
            continue;
        }
        // Make intersection relation to locator.
        for (const HdDataSourceLocator &dirtyLocator :
                 entry.dirtyLocators.Intersection(locator)) {
            relativeDirtyLocatorSet.insert(dirtyLocator.RemoveFirstElement());
        }
        // Let provider expand locators.
        HdFlattenedDataSourceProviderSharedPtr const &provider =
            _dataSourceProviders[i];
        provider->ComputeDirtyLocatorsForDescendants(&relativeDirtyLocatorSet);
        if (relativeDirtyLocatorSet.Contains(
                HdDataSourceLocator::EmptyLocator())) {
            // If provider expandede to the universal set, just
            // nuke entire data source.
            dirtyLocators.insert(locator);
            continue;
        }
        // Make relative data source locators absolute.
        for (const HdDataSourceLocator &relativeDirtyLocator :
                 relativeDirtyLocatorSet) {
            dirtyLocators.insert(
                locator.Append(relativeDirtyLocator));
        }
    }

    if (!dirtyLocators.IsEmpty()) {
        _DirtyHierarchy(entry.primPath, relativeDirtyLocators,
            dirtyLocators, additionalDirty);
    }

    // Mark _PrimLevelWrappingDataSource to refetch input data source again
    // if the dirtyLocators indicate so.
    static const HdDataSourceLocator primLevelContainer(
        HdDataSourceLocatorSentinelTokens->container);
    if (entry.dirtyLocators.Contains(primLevelContainer)) {
        const _PrimTable::const_iterator it = _prims.find(entry.primPath);
        if (it != _prims.end() && it->second.dataSource) {
            if (auto const ds = _PrimLevelWrappingDataSource::Cast(
                    it->second.dataSource)) {
                ds->PrimContainerDirtied();
            }
        }
    }
}

void
HdFlatteningSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    _ConsolidateRecentPrims();

    _AdditionalDirtiedVector dirtyEntries;
    {
        TRACE_SCOPE("Dirty hierarchy");

        WorkParallelForN(entries.size(),
        [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            const HdSceneIndexObserver::DirtiedPrimEntry &entry = entries[i];
            _PrimDirtied(entry, &dirtyEntries);
        }});
    }

    if (dirtyEntries.empty()) {
        _SendPrimsDirtied(entries);
    } else {
        HdSceneIndexObserver::DirtiedPrimEntries flattened(
            entries.begin(), entries.end());
        flattened.insert(flattened.end(), dirtyEntries.begin(),
            dirtyEntries.end());
        _SendPrimsDirtied(flattened);
    }
}

void
HdFlatteningSceneIndex::_ConsolidateRecentPrims()
{
    TRACE_FUNCTION();

    for (auto &entry: _recentPrims) {
        std::swap(_prims[entry.first], entry.second);
    }
    _recentPrims.clear();
}

void
HdFlatteningSceneIndex::_DirtyHierarchy(
    const SdfPath &primPath,
    const _DataSourceLocatorSetVector &relativeDirtyLocators,
    const HdDataSourceLocatorSet &dirtyLocators,
    _AdditionalDirtiedVector *dirtyEntries) const
{
    // XXX: here and elsewhere, if a parent xform is dirtied and the child has
    // resetXformStack, we could skip dirtying the child...

    auto startEndIt = _prims.FindSubtreeRange(primPath);
    for (auto it = startEndIt.first; it != startEndIt.second; ) {
        const HdSceneIndexPrim &prim = it->second;

        if (_PrimLevelWrappingDataSourceHandle dataSource =
                _PrimLevelWrappingDataSource::Cast(prim.dataSource)) {
            if (dataSource->PrimDirtied(relativeDirtyLocators)) {
                // If we invalidated any data for any prim besides "primPath"
                // (which already has a notice), generate a new PrimsDirtied
                // notice.
                if (it->first != primPath) {
                    dirtyEntries->emplace_back(it->first, dirtyLocators);
                }
            } else {
                // If we didn't invalidate any data, we can safely assume that
                // no downstream prims depended on this prim for their
                // flattened result, and skip to the next subtree. This is
                // an important optimization for (e.g.) scene population,
                // where no data is cached yet...
                it = it.GetNextSubtree();
                continue;
            }
        }
        ++it;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
