//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/usd/usd/primRange.h"

#include "pxr/usdImaging/usdImaging/adapterManager.h"
#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourceStage.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"

#include "pxr/base/tf/denseHashSet.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

using APISchemaEntry = UsdImaging_AdapterManager::APISchemaEntry;
using APISchemaAdapters = UsdImaging_AdapterManager::APISchemaAdapters;

bool
_Contains(const TfTokenVector &vec, const TfToken &t)
{
    return std::find(vec.begin(), vec.end(), t) != vec.end();
}

TfTokenVector
_GetImagingSubprims(
    UsdPrim const& prim,
    const APISchemaAdapters &adapters)
{
    switch (adapters.size())
    {
    case 0:
        {
            // If this prim isn't handled by any adapters, make sure we
            // include the trivial subprim "".
            static const TfTokenVector s_default = { TfToken() };
            return s_default;
        }
    case 1:
        {
            TfTokenVector subprims = adapters[0].first->GetImagingSubprims(
                prim, adapters[0].second);
            // Enforce that the trivial subprim "" always exists, to pick up
            // inherited attributes and for traversal purposes.
            if (!_Contains(subprims, TfToken())) {
                subprims.push_back(TfToken());
            }
            return subprims;
        }
    default:
        {
            // We always add the empty token here and skip it in the loop below.
            // This ensures that we pick up the prim for inherited attributes
            // and for traversal purposes.
            TfTokenVector subprims = { TfToken() };
            TfDenseHashSet<TfToken, TfHash> subprimsSet;

            for (const APISchemaEntry &entry : adapters) {
                UsdImagingAPISchemaAdapterSharedPtr const &apiAdapter =
                    entry.first;
                
                if (!apiAdapter) {
                    continue;
                }
                const TfToken &instanceName = entry.second;
                for (const TfToken &subprim :
                        apiAdapter->GetImagingSubprims(prim, instanceName)) {
                    if ( !subprim.IsEmpty()
                         && subprimsSet.find(subprim) == subprimsSet.end()) {
                        subprims.push_back(subprim);
                        subprimsSet.insert(subprim);
                    }
                }
            }

            return subprims;
        }
    }
}

TfToken
_GetImagingSubprimType(
        const APISchemaAdapters &adapters,
        UsdPrim const& prim,
        const TfToken &subprim)
{
    // strongest non-empty opinion wins
    for (const APISchemaEntry &entry : adapters) {
        TfToken result =
            entry.first->GetImagingSubprimType(prim, subprim, entry.second);

        if (!result.IsEmpty()) {
            return result;
        }
    }

    return TfToken();
}

HdContainerDataSourceHandle
_GetImagingSubprimData(
        const APISchemaAdapters &adapters,
        UsdPrim const& prim,
        const TfToken &subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (adapters.empty()) {
        return nullptr;
    }

    if (adapters.size() == 1) {
        return adapters[0].first->GetImagingSubprimData(
            prim, subprim, adapters[0].second, stageGlobals);
    }

    TfSmallVector<HdContainerDataSourceHandle, 8> containers;
    containers.reserve(adapters.size());

    for (const APISchemaEntry &entry : adapters) {
        if (HdContainerDataSourceHandle ds =
                entry.first->GetImagingSubprimData(
                    prim, subprim, entry.second, stageGlobals)) {
            containers.push_back(ds);
        }
    }

    if (containers.empty()) {
        return nullptr;
    }

    if (containers.size() == 1) {
        return containers[0];
    }

    return HdOverlayContainerDataSource::New(
        containers.size(), containers.data());
}

HdDataSourceLocatorSet
_InvalidateImagingSubprim(
        const APISchemaAdapters &adapters,
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties)
{
    if (adapters.empty()) {
        return HdDataSourceLocatorSet();
    }

    if (adapters.size() == 1) {
        return adapters[0].first->InvalidateImagingSubprim(
            prim, subprim, adapters[0].second, properties);
    }

    HdDataSourceLocatorSet result;

    for (const APISchemaEntry &entry : adapters) {
        result.insert(entry.first->InvalidateImagingSubprim(
                    prim, subprim, entry.second, properties));
    }

    return result;
}

}

// ---------------------------------------------------------------------------

UsdImagingStageSceneIndex::UsdImagingStageSceneIndex()
  : _adapterManager(std::make_unique<UsdImaging_AdapterManager>())
{
}

UsdImagingStageSceneIndex::~UsdImagingStageSceneIndex()
{
    SetStage(nullptr);
}

// ---------------------------------------------------------------------------

HdSceneIndexPrim
UsdImagingStageSceneIndex::GetPrim(const SdfPath &path) const
{
    TRACE_FUNCTION();

    static const HdSceneIndexPrim s_emptyPrim = {TfToken(), nullptr};

    if (!_stage) {
        return s_emptyPrim;
    }

    if (path.IsAbsoluteRootPath()) {
        return { TfToken(), UsdImagingDataSourceStage::New(_stage) };
    }

    const SdfPath primPath = path.GetPrimPath();

    const UsdPrim prim = _stage->GetPrimAtPath(primPath);
    if (!prim) {
        return s_emptyPrim;
    }
    if (prim.IsInstanceProxy()) {
        return s_emptyPrim;
    }

    const TfToken subprim =
        path.IsPropertyPath() ? path.GetNameToken() : TfToken();

    const APISchemaAdapters adapters =
        _adapterManager->AdapterSetLookup(prim);

    return {
        _GetImagingSubprimType(adapters, prim, subprim),
        _GetImagingSubprimData(adapters, prim, subprim, _stageGlobals)
    };
}

// ---------------------------------------------------------------------------

SdfPathVector
UsdImagingStageSceneIndex::GetChildPrimPaths(
        const SdfPath &path) const
{
    TRACE_FUNCTION();

    if (!_stage) {
        return {};
    }

    // If this is a subprim path, treat it as a leaf.
    if (!path.IsAbsoluteRootOrPrimPath()) {
        return {};
    }

    UsdPrim prim = _stage->GetPrimAtPath(path);
    if (!prim) {
        return {};
    }

    SdfPathVector result;

    // This function needs to match Populate() in traversal rules.  Namely:
    // 1.) Unless prim adapter represents descendent prims, all children of
    //     prim (modulo predicate) are traversed, although some of them may
    //     have null type.
    // 2.) If prim has imaging behaviors and defines subprims other than
    //     TfToken(), those need to be considered as well.

    UsdImagingPrimAdapterSharedPtr primAdapter;
    const APISchemaAdapters adapters =
        _adapterManager->AdapterSetLookup(prim, &primAdapter);

    if (!primAdapter ||
            primAdapter->GetPopulationMode() !=
                UsdImagingPrimAdapter::RepresentsSelfAndDescendents) {
        UsdPrimSiblingRange range =
            prim.GetFilteredChildren(_GetTraversalPredicate());
        for (const UsdPrim &child: range) {
            result.push_back(child.GetPath());
        }
    }

    const SdfPath primPath = prim.GetPath();
    for (const TfToken &subprim : _GetImagingSubprims(prim, adapters)) {
        if (!subprim.IsEmpty()) {
            result.push_back(primPath.AppendProperty(subprim));
        }
    }

    if (path.IsAbsoluteRootPath()) {
        for (const UsdPrim &prim : _stage->GetPrototypes()) {
            result.push_back(prim.GetPath());
        }
    }

    return result;
}

// ---------------------------------------------------------------------------

void UsdImagingStageSceneIndex::SetTime(UsdTimeCode time)
{
    TRACE_FUNCTION();

    if (_stageGlobals.GetTime() == time) {
        return;
    }

    HdSceneIndexObserver::DirtiedPrimEntries dirtied;
    _stageGlobals.SetTime(time, &dirtied);
    if (!dirtied.empty()) {
        _SendPrimsDirtied(dirtied);
    }
}

UsdTimeCode UsdImagingStageSceneIndex::GetTime() const
{
    return _stageGlobals.GetTime();
}

void UsdImagingStageSceneIndex::SetStage(UsdStageRefPtr stage)
{
    if (_stage == stage) {
        return;
    }

    TRACE_FUNCTION();

    if (_stage) {
        TF_DEBUG(USDIMAGING_POPULATION).Msg("[Population] Removing </>\n");
        _SendPrimsRemoved({SdfPath::AbsoluteRootPath()});
        _stageGlobals.Clear();
        TfNotice::Revoke(_objectsChangedNoticeKey);
        _adapterManager->Reset();
    }

    _stage = stage;

    if (_stage) {
        _objectsChangedNoticeKey =
            TfNotice::Register(TfCreateWeakPtr(this),
                &UsdImagingStageSceneIndex::_OnUsdObjectsChanged, _stage);
    }

    _Populate();
}

void UsdImagingStageSceneIndex::_Populate()
{
    if (!_stage) {
        return;
    }

    _PopulateSubtree(_stage->GetPseudoRoot());

    for (const UsdPrim &prim : _stage->GetPrototypes()) {
        _PopulateSubtree(prim);
    }

}

void UsdImagingStageSceneIndex::_PopulateSubtree(UsdPrim subtreeRoot)
{
    TRACE_FUNCTION();
    if (!subtreeRoot) {
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries addedPrims;
    size_t lastEnd = 0;

    UsdPrimRange range(subtreeRoot, _GetTraversalPredicate());

    for (auto it = range.begin(); it != range.end(); ++it) {
        const UsdPrim &prim = *it;
        if (prim.IsPseudoRoot()) {
            // XXX for now, we have to make sure the prim at the absolute root
            // path is "added"
            addedPrims.emplace_back(SdfPath::AbsoluteRootPath(), TfToken());
            continue;
        }

        UsdImagingPrimAdapterSharedPtr primAdapter;
        const APISchemaAdapters adapters =
            _adapterManager->AdapterSetLookup(prim, &primAdapter);

        if (primAdapter && primAdapter->GetPopulationMode() ==
                 UsdImagingPrimAdapter::RepresentsSelfAndDescendents) {
            it.PruneChildren();
        }

        // Enumerate the imaging sub-prims.
        const SdfPath primPath = prim.GetPath();
        const TfTokenVector subprims =
            _GetImagingSubprims(prim, adapters);

        for (TfToken const& subprim : subprims) {
            const SdfPath subpath =
                subprim.IsEmpty() ? primPath : primPath.AppendProperty(subprim);

            addedPrims.emplace_back(subpath,
                _GetImagingSubprimType(adapters, prim, subprim));
        }

        if (TfDebug::IsEnabled(USDIMAGING_POPULATION)) {
            TF_DEBUG(USDIMAGING_POPULATION).Msg(
                "[Population] Populating <%s> (type = %s) ->\n",
                primPath.GetText(),
                prim.GetPrimTypeInfo().GetSchemaTypeName().GetText());
            for (size_t i = lastEnd; i < addedPrims.size(); ++i) {
                TF_DEBUG(USDIMAGING_POPULATION).Msg("\t<%s> (type = %s)\n",
                    addedPrims[i].primPath.GetText(),
                    addedPrims[i].primType.GetText());
            }
            lastEnd = addedPrims.size();
        }
    }

    _SendPrimsAdded(addedPrims);
}

Usd_PrimFlagsConjunction
UsdImagingStageSceneIndex::_GetTraversalPredicate() const
{
    // Note that it differs from the UsdPrimDefaultPredicate by not requiring
    // UsdPrimIsDefined. This way, we pick up instance and over's and their
    // namespace descendants which might include prototypes instanced by a
    // point instancer.
    //
    // Over's and their namespace descendants are made unrenderable by changing
    // their prim type to empty by UsdImaging_PiPrototypeSceneIndex.
    //
    // The UsdImaging_NiPrototypeSceneIndex is doing something similar for
    // native instances.
    //
    return UsdPrimIsActive && UsdPrimIsLoaded && !UsdPrimIsAbstract;
}

// ---------------------------------------------------------------------------

void
UsdImagingStageSceneIndex::_OnUsdObjectsChanged(
    UsdNotice::ObjectsChanged const& notice,
    UsdStageWeakPtr const& sender)
{
    if (!sender || !TF_VERIFY(sender == _stage)) {
        return;
    }

    TRACE_FUNCTION();

    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Objects Changed] Notice received "
            "from stage with root layer @%s@\n",
            sender->GetRootLayer()->GetIdentifier().c_str());

    // These paths represent objects which have been modified in a structural
    // way, for example changing type or composition topology. These paths may
    // be paths to prims or properties. Prim resyncs trigger a repopulation of
    // the subtree rooted at the prim path. Property resyncs are promoted to
    // hydra property invalidations.
    const UsdNotice::ObjectsChanged::PathRange pathsToResync =
        notice.GetResyncedPaths();
    for (auto it = pathsToResync.begin(); it != pathsToResync.end(); ++it) {
        if (it->IsPrimPath()) {
            _usdPrimsToResync.push_back(*it);
            TF_DEBUG(USDIMAGING_CHANGES).Msg(" - Resync queued: %s\n",
                    it->GetText());
        } else if (it->IsPropertyPath()) {
            _usdPropertiesToUpdate[it->GetPrimPath()]
                .push_back(it->GetNameToken());
            TF_DEBUG(USDIMAGING_CHANGES).Msg(
                    " - Property update due to property resync queued: %s\n",
                    it->GetText());
        }
    }

    // These paths represent objects which have been modified in a 
    // non-structural way, for example setting a value. These paths may be paths
    // to prims or properties. Property invalidations flow into hydra as dirty
    // locators. Prim invalidations are promoted to resyncs or ignored.
    const UsdNotice::ObjectsChanged::PathRange pathsToUpdate =
        notice.GetChangedInfoOnlyPaths();
    const SdfSchema& schema = SdfSchema::GetInstance();

    for (auto it = pathsToUpdate.begin(); it != pathsToUpdate.end(); ++it) {
        if (it->IsPrimPath()) {
            // By default, resync the prim if there are any changes to plugin
            // fields and ignore changes to built-in fields. Schemas typically
            // register their own plugin metadata fields instead of relying on
            // built-in fields.
            const TfTokenVector changedFields = it.GetChangedFields();
            for (const TfToken &field : changedFields) {
                const SdfSchema::FieldDefinition *fieldDef =
                    schema.GetFieldDefinition(field);
                if (fieldDef && fieldDef->IsPlugin()) {
                    _usdPrimsToResync.push_back(*it);
                    TF_DEBUG(USDIMAGING_CHANGES).Msg(
                            " - Resync due to prim update queued: %s\n",
                            it->GetText());
                    break;
                }
            }
        } else if (it->IsPropertyPath()) {
            _usdPropertiesToUpdate[it->GetPrimPath()]
                .push_back(it->GetNameToken());
            TF_DEBUG(USDIMAGING_CHANGES).Msg(" - Property update queued: %s\n",
                    it->GetText());
        }
    }
}

UsdImagingStageSceneIndex::_PrimAdapterPair
UsdImagingStageSceneIndex::_FindResponsibleAncestor(const UsdPrim &prim)
{
    UsdPrim parentPrim = prim.GetParent();
    while (parentPrim) {

        UsdImagingPrimAdapterSharedPtr primAdapter;
        _adapterManager->AdapterSetLookup(parentPrim, &primAdapter);

        if (primAdapter && primAdapter->GetPopulationMode() ==
                UsdImagingPrimAdapter::RepresentsSelfAndDescendents) {

            return {parentPrim, primAdapter};
        }

        parentPrim = parentPrim.GetParent();
    }

    return {UsdPrim(), nullptr};
}



void
UsdImagingStageSceneIndex::_ApplyPendingResyncs()
{
    if (!_stage || _usdPrimsToResync.empty()) {
        return;
    }

    std::sort(_usdPrimsToResync.begin(), _usdPrimsToResync.end());
    size_t lastResynced = 0;
    for (size_t i = 0; i < _usdPrimsToResync.size(); ++i) {
        // Coalesce paths with a common prefix, so as not to resync /A and /A/B,
        // since due to their hierarchical nature the latter is redundant.
        // Thanks to the sort, all suffixes of path[i] are in a contiguous block
        // to the right of i.  We skip all resync paths until we find one that's
        // not a suffix of path[i], which marks the start of a new (possibly
        // 1-element) contiguous block of suffixes of some path.
        if (i > 0 && _usdPrimsToResync[i].HasPrefix(
                _usdPrimsToResync[lastResynced])) {
            continue;
        }
        lastResynced = i;

        UsdPrim prim =
            _stage->GetPrimAtPath(_usdPrimsToResync[i]);


        // For prims represented by an ancestor, we don't want to repopulate
        // (as they wouldn't have been populated in the first place) but instead
        // convert to an empty property name dirtying to be handled in
        // ApplyPendingUpdates. Do not worry about redundant property
        // invalidation in that case.
        UsdImagingPrimAdapterSharedPtr primAdapter;
        _adapterManager->AdapterSetLookup(prim, &primAdapter);
        if (primAdapter &&
                primAdapter->GetPopulationMode() ==
                    UsdImagingPrimAdapter::RepresentedByAncestor) {
            _PrimAdapterPair ancestor = _FindResponsibleAncestor(prim);
            if (ancestor.second) {
                TF_DEBUG(USDIMAGING_CHANGES).Msg(
                    "Invalidating <%s> due to resync of descendant <%s>\n",
                        ancestor.first.GetPrimPath().GetText(),
                        _usdPrimsToResync[i].GetText());
                _usdPropertiesToUpdate[_usdPrimsToResync[i]] = {TfToken()};
                continue;
            }
        }

        TF_DEBUG(USDIMAGING_CHANGES).Msg("[Population] Repopulating <%s>\n",
                _usdPrimsToResync[i].GetText());
        _SendPrimsRemoved({_usdPrimsToResync[i]});
        _PopulateSubtree(prim);

        // Prune property updates of resynced prims, which are redundant.
        auto start = _usdPropertiesToUpdate.lower_bound(_usdPrimsToResync[i]);
        auto end = start;
        while (end != _usdPropertiesToUpdate.end() &&
               end->first.HasPrefix(_usdPrimsToResync[i])) {
            ++end;
        }
        if (start != end) {
            _usdPropertiesToUpdate.erase(start, end);
        }
    }

    _usdPrimsToResync.clear();
}

void
UsdImagingStageSceneIndex::ApplyPendingUpdates()
{
    if (!_stage ||
        (_usdPrimsToResync.empty() && _usdPropertiesToUpdate.empty())) {
        return;
    }

    TRACE_FUNCTION();

    _ApplyPendingResyncs();

    // Changed properties...
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrims;
    for (auto const& pair : _usdPropertiesToUpdate) {
        const SdfPath &primPath = pair.first;
        const TfTokenVector &properties = pair.second;
        // XXX: We could sort/unique the properties here...
        
        const UsdPrim prim = _stage->GetPrimAtPath(primPath);

        UsdImagingPrimAdapterSharedPtr primAdapter;
        APISchemaAdapters
            adapters = _adapterManager->AdapterSetLookup(prim, &primAdapter);

        if (primAdapter && primAdapter->GetPopulationMode()
                == UsdImagingPrimAdapter::RepresentedByAncestor) {

            _PrimAdapterPair ancestor = _FindResponsibleAncestor(prim);
            if (ancestor.second) {
                UsdPrim &parentPrim = ancestor.first;
                UsdImagingPrimAdapterSharedPtr &parentAdapter = ancestor.second;

                // Give the parent adapter an opportunity to invalidate
                // each of the subprims it declares itself. API schema
                // adapters do not participate.
                for (const TfToken &subprim :
                        parentAdapter->GetImagingSubprims(parentPrim)) {

                     HdDataSourceLocatorSet dirtyLocators = 
                        parentAdapter->
                            InvalidateImagingSubprimFromDescendent(
                                parentPrim, prim, subprim, properties);

                    if (!dirtyLocators.IsEmpty()) {
                        const SdfPath path = subprim.IsEmpty()
                            ? parentPrim.GetPrimPath()
                            : parentPrim.GetPrimPath().AppendProperty(
                                subprim);
                        dirtiedPrims.emplace_back(path, dirtyLocators);
                    }
                }

                // we were handled by an ancestor prim and need not do the
                // below invalidation on our own.
                continue;
            }

            // If a responsible ancestor wasn't found, we've likely been
            // populated and should at least get a chance to handle it
            // ourself below.
        }

        const TfTokenVector subprims = _GetImagingSubprims(prim, adapters);

        for (TfToken const& subprim : subprims) {
            const HdDataSourceLocatorSet dirtyLocators =
                _InvalidateImagingSubprim(
                    adapters, prim, subprim, properties);

            if (!dirtyLocators.IsEmpty()) {

                const static HdDataSourceLocator repopulateLocator(
                    UsdImagingTokens->stageSceneIndexRepopulate);

                if (dirtyLocators.Contains(repopulateLocator)) {
                    _usdPrimsToResync.push_back(primPath);
                } else {
                    SdfPath const subpath =
                        subprim.IsEmpty()
                            ? primPath
                            : primPath.AppendProperty(subprim);
                    dirtiedPrims.emplace_back(subpath, dirtyLocators);
                }

            }
        }
    }

    _usdPropertiesToUpdate.clear();

    // Resync any prims whose property invalidation indicated repopulation
    // was necessary
    if (!_usdPrimsToResync.empty()) {
        _ApplyPendingResyncs();
    }

    if (dirtiedPrims.size() > 0) {
        _SendPrimsDirtied(dirtiedPrims);
    }
}

// ---------------------------------------------------------------------------

void UsdImagingStageSceneIndex::_StageGlobals::FlagAsTimeVarying(
        const SdfPath & primPath,
        const HdDataSourceLocator & locator) const
{
    _VariabilityMap::accessor accessor;
    _timeVaryingLocators.insert(accessor, primPath);
    accessor->second.insert(locator);
}

UsdTimeCode UsdImagingStageSceneIndex::_StageGlobals::GetTime() const
{
    return _time;
}

void UsdImagingStageSceneIndex::_StageGlobals::SetTime(UsdTimeCode time,
        HdSceneIndexObserver::DirtiedPrimEntries *dirtied)
{
    TRACE_FUNCTION();

    _time = time;
    if (dirtied && !_timeVaryingLocators.empty()) {
        dirtied->reserve(_timeVaryingLocators.size());
        for (const auto & entryPair : _timeVaryingLocators) {
            dirtied->emplace_back(entryPair.first, entryPair.second);
        }
    }
}

void UsdImagingStageSceneIndex::_StageGlobals::Clear()
{
    _timeVaryingLocators.clear();
    _time = UsdTimeCode::EarliestTime();
}

PXR_NAMESPACE_CLOSE_SCOPE
