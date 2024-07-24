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
#include "pxr/base/work/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdFlatteningSceneIndex_Impl
{

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
    
private:
    _PrimLevelWrappingDataSource(
        const HdFlatteningSceneIndex &flatteningSceneIndex,
        const SdfPath &primPath,
        const HdContainerDataSourceHandle &inputDataSource)
      : _flatteningSceneIndex(flatteningSceneIndex)
      , _primPath(primPath)
      , _inputDataSource(inputDataSource)
      , _computedDataSources(
          _flatteningSceneIndex.GetFlattenedDataSourceNames().size())
    {
    }
    
    const HdFlatteningSceneIndex &_flatteningSceneIndex;
    const SdfPath _primPath;
    HdContainerDataSourceHandle const _inputDataSource;

    // Parallel to HdFlatteningSceneIndex::GetFlattenedDataSourceNames()
    TfSmallVector<HdDataSourceBaseAtomicHandle, _smallVectorSize>
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

        HdDataSourceBaseAtomicHandle &dsAtomicHandle = _computedDataSources[i];
        HdDataSourceBaseHandle const ds =
            HdDataSourceBase::AtomicLoad(dsAtomicHandle);
        if (!ds) {
            continue;
        }
        if (!relativeDirtyLocators[i].Contains(
                HdDataSourceLocator::EmptyLocator())) {
            if (HdInvalidatableContainerDataSourceHandle const
                    invalidatableDs =
                        HdInvalidatableContainerDataSource::Cast(ds)) {
                anyDirtied |=
                    invalidatableDs->Invalidate(relativeDirtyLocators[i]);
                continue;
            }
        }

        HdDataSourceBase::AtomicStore(dsAtomicHandle, nullptr);
        anyDirtied = true;
    }

    return anyDirtied;
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
    if (!_inputDataSource) {
        return _flatteningSceneIndex.GetFlattenedDataSourceNames();
    }

    TfTokenVector result = _inputDataSource->GetNames();
    _Insert(_flatteningSceneIndex.GetFlattenedDataSourceNames(), &result);
    return result;
};        

HdDataSourceBaseHandle
_PrimLevelWrappingDataSource::Get(
        const TfToken &name)
{
    const TfTokenVector &dataSourceNames =
        _flatteningSceneIndex.GetFlattenedDataSourceNames();
    const HdFlattenedDataSourceProviderSharedPtrVector &providers =
        _flatteningSceneIndex.GetFlattenedDataSourceProviders();

    for (size_t i = 0; i < dataSourceNames.size(); i++) {
        if (name != dataSourceNames[i]) {
            continue;
        }

        HdDataSourceBaseAtomicHandle &dsAtomicHandle =
            _computedDataSources[i];
        if (HdDataSourceBaseHandle const computedDs =
                HdDataSourceBase::AtomicLoad(dsAtomicHandle)) {
            return HdContainerDataSource::Cast(computedDs);
        }
        const HdFlattenedDataSourceProvider::Context ctx(
            _flatteningSceneIndex,
            _primPath,
            name,
            _inputDataSource);
        HdDataSourceBaseHandle flattenedDs = 
            providers[i]->GetFlattenedDataSource(ctx);
        if (!flattenedDs) {
            // A nullptr means cache miss. To distinguish a cache miss from
            // the flattened data source being null, we store a bool
            // data source.
            flattenedDs = HdRetainedTypedSampledDataSource<bool>::New(false);
        }

        HdDataSourceBaseAtomicHandle existingDs;
        // Make sure that we only set the flattened data source only once.
        // Flattened data source can cache state and need to be invalidated.
        //
        // It would be bad if we return different flattened data sources
        // on different calls and only invalidate the last one that was
        // returned.
        if (HdDataSourceBase::AtomicCompareExchange(
                dsAtomicHandle, existingDs, flattenedDs)) {
            return HdContainerDataSource::Cast(flattenedDs);
        } else {
            return HdContainerDataSource::Cast(existingDs);
        }
    }

    if (_inputDataSource) {
        return _inputDataSource->Get(name);
    }
    return nullptr;
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
        *this, primPath, prim.dataSource);

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

    // Check the hierarchy for cached prims to dirty
    HdSceneIndexObserver::DirtiedPrimEntries dirtyEntries;
    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        _DirtyHierarchy(
            entry.primPath,
            _relativeDataSourceLocators,
            _dataSourceLocatorSet,
            &dirtyEntries);
    }

    // Clear out any cached dataSources for prims that have been re-added.
    // They will get updated dataSources in the next call to GetPrim().
    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        const _PrimTable::iterator i = _prims.find(entry.primPath);
        if (i != _prims.end()) {
            WorkSwapDestroyAsync(i->second.dataSource);
        }
    }

    _SendPrimsAdded(entries);
    if (!dirtyEntries.empty()) {
        _SendPrimsDirtied(dirtyEntries);
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
    HdSceneIndexObserver::DirtiedPrimEntries * const dirtyEntries)
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
        _DirtyHierarchy(
            entry.primPath, relativeDirtyLocators, dirtyLocators, dirtyEntries);
    }

    // Empty locator indicates that we need to pull the input data source
    // again - which we achieve by destroying the data source wrapping the
    // input data source.
    // Note that we destroy it after calling _DirtyHierarchy to not prevent
    // _DirtyHierarchy propagating the invalidation to the descendants.
    if (entry.dirtyLocators.Contains(HdDataSourceLocator::EmptyLocator())) {
        const _PrimTable::iterator it = _prims.find(entry.primPath);
        if (it != _prims.end() && it->second.dataSource) {
            WorkSwapDestroyAsync(it->second.dataSource);
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

    HdSceneIndexObserver::DirtiedPrimEntries dirtyEntries;
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        _PrimDirtied(entry, &dirtyEntries);
    }

    _SendPrimsDirtied(entries);
    if (!dirtyEntries.empty()) {
        _SendPrimsDirtied(dirtyEntries);
    }
}

void
HdFlatteningSceneIndex::_ConsolidateRecentPrims()
{
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
    HdSceneIndexObserver::DirtiedPrimEntries * const dirtyEntries)
{
    // XXX: here and elsewhere, if a parent xform is dirtied and the child has
    // resetXformStack, we could skip dirtying the child...

    auto startEndIt = _prims.FindSubtreeRange(primPath);
    for (auto it = startEndIt.first; it != startEndIt.second; ) {
        HdSceneIndexPrim &prim = it->second;

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
