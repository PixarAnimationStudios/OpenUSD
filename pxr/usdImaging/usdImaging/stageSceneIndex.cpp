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

#include "pxr/usd/usdLux/lightAPI.h"

#include "pxr/usdImaging/usdImaging/adapterRegistry.h"
#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/imaging/hd/overlayContainerDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// Because auto-applied schemas have weaker opinions than type-based prim
// adapters, it interweaves the opinion strength of prim and API schemas.
// In order to present that to all consumers as a single ordered list of
// potential contributors, this class satisfies UsdImagingAPISchemaAdapter
// by ignoring appliedInstanceName (which will always be empty as built) and
// calling through to equivalent methods on a UsdImagingPrimAdapter
class _PrimAdapterAPISchemaAdapter : public UsdImagingAPISchemaAdapter
{
public:
    _PrimAdapterAPISchemaAdapter(
            const UsdImagingPrimAdapterSharedPtr &primAdapter)
    : _primAdapter(primAdapter)
    {}

    TfTokenVector GetImagingSubprims(
            TfToken const& appliedInstanceName) override {
        return _primAdapter->GetImagingSubprims();
    }

    TfToken GetImagingSubprimType(
            TfToken const& subprim,
            TfToken const& appliedInstanceName) override {

        return _primAdapter->GetImagingSubprimType(subprim);
    }

    HdContainerDataSourceHandle GetImagingSubprimData(
            TfToken const& subprim,
            UsdPrim const& prim,
            TfToken const& appliedInstanceName,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override {
        return _primAdapter->GetImagingSubprimData(subprim, prim, stageGlobals);
    }

    HdDataSourceLocatorSet InvalidateImagingSubprim(
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            TfTokenVector const& properties) override {

        return _primAdapter->InvalidateImagingSubprim(subprim, properties);
    }

private:
    UsdImagingPrimAdapterSharedPtr _primAdapter;
};


// If no prim type adapter is present, this will call use
// UsdImagingDataSourcePrim
class _BasePrimAdapterAPISchemaAdapter : public UsdImagingAPISchemaAdapter
{
public:

    _BasePrimAdapterAPISchemaAdapter()
    {}

    HdContainerDataSourceHandle GetImagingSubprimData(
            TfToken const& subprim,
            UsdPrim const& prim,
            TfToken const& appliedInstanceName,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override {

        if (subprim.IsEmpty()) {
            return UsdImagingDataSourcePrim::New(
                prim.GetPath(), prim, stageGlobals);
        }
        return nullptr;
    }

    HdDataSourceLocatorSet InvalidateImagingSubprim(
            TfToken const& subprim,
            TfToken const& appliedInstanceName,
            TfTokenVector const& properties) override {

        return UsdImagingDataSourcePrim::Invalidate(subprim, properties);
    }
};




} //anonymous namespace



// ---------------------------------------------------------------------------
// Adapter delegation

TfTokenVector
UsdImagingStageSceneIndex::_GetImagingSubprims(
        const _APISchemaAdapters &adapters) const
{
    TfTokenVector subprims;

    switch (adapters.size())
    {
    case 0:
        break;
    case 1:
        subprims = adapters[0].first->GetImagingSubprims(adapters[0].second);
        break;

    default:
        {
            TfDenseHashSet<TfToken, TfHash> subPrimNames;

            for (const _APISchemaEntry &entry : adapters) {
                UsdImagingAPISchemaAdapterSharedPtr const &apiAdapter =
                    entry.first;
                
                if (!apiAdapter) {
                    continue;
                }
                const TfToken &instanceName = entry.second;
                for (const TfToken &subPrimName :
                        apiAdapter->GetImagingSubprims(instanceName)) {
                    if (!subPrimName.IsEmpty()
                            && subPrimNames.find(subPrimName)
                                == subPrimNames.end()) {
                        subprims.push_back(subPrimName);
                        subPrimNames.insert(subPrimName);
                    }
                }

                if (subPrimNames.find(TfToken()) == subPrimNames.end()) {
                    subprims.push_back(TfToken());
                }
            }

            return subprims;
        }
    }

    if (subprims.empty()) {
        // If this prim isn't handled by any adapters, make sure we
        // include the trivial subprim "".
        static const TfTokenVector s_default = { TfToken() };
        return s_default;
    } else {
        // Enforce that the trivial subprim "" always exists, to pick up
        // inherited attributes and for traversal purposes.
        if (std::find(subprims.begin(), subprims.end(), TfToken())
                == subprims.end()) {
            subprims.push_back(TfToken());
        }
        return subprims;
    }
}

TfToken
UsdImagingStageSceneIndex::_GetImagingSubprimType(
        const _APISchemaAdapters &adapters,
        const TfToken &subprim) const
{
    // strongest non-empty opinion wins
    for (const _APISchemaEntry &entry : adapters) {
        TfToken result =
            entry.first->GetImagingSubprimType(subprim, entry.second);

        if (!result.IsEmpty()) {
            return result;
        }
    }

    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingStageSceneIndex::_GetImagingSubprimData(
        const _APISchemaAdapters &adapters,
        UsdPrim prim, const TfToken &subprim) const
{
    if (adapters.empty()) {
        return nullptr;
    }

    if (adapters.size() == 1) {
        return adapters[0].first->GetImagingSubprimData(
            subprim, prim, adapters[0].second, _stageGlobals);
    }

    TfSmallVector<HdContainerDataSourceHandle, 8> containers;
    containers.reserve(adapters.size());

    for (const _APISchemaEntry &entry : adapters) {
        if (HdContainerDataSourceHandle ds =
                entry.first->GetImagingSubprimData(
                    subprim, prim, entry.second, _stageGlobals)) {
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
UsdImagingStageSceneIndex::_InvalidateImagingSubprim(
        const _APISchemaAdapters &adapters,
        TfToken const& subprim, TfTokenVector const& properties) const
{
    if (adapters.empty()) {
        return HdDataSourceLocatorSet();
    }

    if (adapters.size() == 1) {
        return adapters[0].first->InvalidateImagingSubprim(
            subprim, adapters[0].second, properties);
    }

    HdDataSourceLocatorSet result;

    for (const _APISchemaEntry &entry : adapters) {
        result.insert(entry.first->InvalidateImagingSubprim(
                    subprim, entry.second, properties));
    }

    return result;
}

UsdImagingStageSceneIndex::_APISchemaAdapters
UsdImagingStageSceneIndex::_AdapterSetLookup(UsdPrim prim) const
{
    const UsdPrimTypeInfo &typeInfo = prim.GetPrimTypeInfo();

    // check for previously cached value of full array
    _AdapterSetMap::const_iterator it = _adapterSetMap.find(&typeInfo);
    if (it != _adapterSetMap.end()) {
        return it->second;
    }

    _APISchemaAdapters result;

    // contains both auto-applied and manually applied schemas
    TfTokenVector allAppliedSchemas = prim.GetAppliedSchemas();
    // contains only the manually applied API schemas
    TfTokenVector appliedAPISchemas = typeInfo.GetAppliedAPISchemas();

    result.reserve(allAppliedSchemas.size() + 1);

    // first add the manually applied API schemas as they have the strongest
    // opinion
    for (const TfToken &schemaToken : appliedAPISchemas) {
        std::pair<TfToken, TfToken> tokenPair =
            UsdSchemaRegistry::GetTypeNameAndInstance(schemaToken);
        if (UsdImagingAPISchemaAdapterSharedPtr a =
                _APIAdapterLookup(tokenPair.first)) {
            result.emplace_back(a, tokenPair.second);
        }
    }

    // then any prim-type schema
    const TfToken adapterKey = typeInfo.GetSchemaTypeName();
    // If there is an adapter for the type name, include it.
    if (UsdImagingPrimAdapterSharedPtr adapter =
            _PrimAdapterLookup(adapterKey)) {
        // wrap and cache the prim adapter in an API schema interface
        UsdImagingAPISchemaAdapterSharedPtr adapterAdapter;

        const auto it = _apiAdapterMap.find(adapterKey);
        if (it == _apiAdapterMap.end()) {
            adapterAdapter = std::make_shared<
                _PrimAdapterAPISchemaAdapter>(adapter);
            _apiAdapterMap[adapterKey] = adapterAdapter;
        } else {
            adapterAdapter = it->second;
        }

        result.emplace_back(adapterAdapter, TfToken());
    } else {
        // use a fallback adapter which calls directly to
        // UsdImagingDataSourcePrim where appropriate
        static const UsdImagingAPISchemaAdapterSharedPtr basePrimAdapter =
             std::make_shared<_BasePrimAdapterAPISchemaAdapter>();

        result.emplace_back(basePrimAdapter, TfToken());
    }

    // then the auto-applied/built-in schemas which will start after the entries
    // which are (also) found (in isolation) within allAppliedSchemas
    for (size_t i = appliedAPISchemas.size(); i < allAppliedSchemas.size();
            ++i) {
        
        const TfToken &schemaToken = allAppliedSchemas[i];
        std::pair<TfToken, TfToken> tokenPair =
            UsdSchemaRegistry::GetTypeNameAndInstance(schemaToken);
            
        if (UsdImagingAPISchemaAdapterSharedPtr a =
                _APIAdapterLookup(tokenPair.first)) {
            result.emplace_back(a, tokenPair.second);
        }
    }

    _adapterSetMap.insert({&typeInfo, result});
    return result;
}

UsdImagingPrimAdapterSharedPtr
UsdImagingStageSceneIndex::_PrimAdapterLookup(const TfToken &adapterKey) const
{
    // Look-up adapter in cache.
    _PrimAdapterMap::const_iterator const it = _primAdapterMap.find(adapterKey);
    if (it != _primAdapterMap.end()) {
        return it->second;
    }

    // Construct and store in cache if not in cache yet.
    UsdImagingAdapterRegistry &reg = UsdImagingAdapterRegistry::GetInstance();
    UsdImagingPrimAdapterSharedPtr adapter = reg.ConstructAdapter(adapterKey);
    _primAdapterMap[adapterKey] = adapter;
    return adapter;
}

UsdImagingAPISchemaAdapterSharedPtr
UsdImagingStageSceneIndex::_APIAdapterLookup(
    const TfToken &adapterKey) const
{
    _ApiAdapterMap::const_iterator const it = _apiAdapterMap.find(adapterKey);
    if (it != _apiAdapterMap.end()) {
        return it->second;
    }

    // Construct and store in cache if not in cache yet.
    UsdImagingAdapterRegistry &reg = UsdImagingAdapterRegistry::GetInstance();
    UsdImagingAPISchemaAdapterSharedPtr adapter =
        reg.ConstructAPISchemaAdapter(adapterKey);
    _apiAdapterMap[adapterKey] = adapter;
    return adapter;
}


// ---------------------------------------------------------------------------

UsdImagingStageSceneIndex::UsdImagingStageSceneIndex()
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

    const SdfPath primPath = path.GetPrimPath();

    UsdPrim prim = _stage->GetPrimAtPath(primPath);
    if (!prim) {
        return s_emptyPrim;
    }

    const TfToken subprim =
        path.IsPropertyPath() ? path.GetNameToken() : TfToken();

    _APISchemaAdapters adapters = _AdapterSetLookup(prim);

    const TfToken imagingType =
        _GetImagingSubprimType(adapters, subprim);

    return {imagingType, _GetImagingSubprimData(adapters, prim, subprim)};
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
    // 1.) All children of prim (modulo predicate) are traversed, although
    //     some of them may have null type.
    // 2.) If prim has imaging behaviors and defines subprims other than
    //     TfToken(), those need to be considered as well.

    UsdPrimSiblingRange range =
        prim.GetFilteredChildren(_GetTraversalPredicate());
    for (UsdPrim child: range) {
        if (child.IsInstance()) {
            // XXX(USD-7119): Add native instancing support...
            continue;
        }
        result.push_back(child.GetPath());
    }

    const SdfPath primPath = prim.GetPath();
    for (const TfToken &subprim : _GetImagingSubprims(_AdapterSetLookup(prim))){
        if (!subprim.IsEmpty()) {
            result.push_back(primPath.AppendChild(subprim));
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
    if (dirtied.size() > 0) {
        _SendPrimsDirtied(dirtied);
    }
}

UsdTimeCode UsdImagingStageSceneIndex::GetTime() const
{
    return _stageGlobals.GetTime();
}

void UsdImagingStageSceneIndex::SetStage(UsdStageRefPtr stage)
{
    TRACE_FUNCTION();

    if (_stage) {
        TF_DEBUG(USDIMAGING_POPULATION).Msg("[Population] Removing </>\n");
        _SendPrimsRemoved({SdfPath::AbsoluteRootPath()});
        _stageGlobals.Clear();
        TfNotice::Revoke(_objectsChangedNoticeKey);
        _primAdapterMap.clear();
        _apiAdapterMap.clear();
        _adapterSetMap.clear();
    }

    _stage = stage;

    if (_stage) {
        _objectsChangedNoticeKey =
            TfNotice::Register(TfCreateWeakPtr(this),
                &UsdImagingStageSceneIndex::_OnUsdObjectsChanged, _stage);
    }
}

void UsdImagingStageSceneIndex::Populate()
{
    if (!_stage) {
        return;
    }

    _Populate(_stage->GetPseudoRoot());
}

void UsdImagingStageSceneIndex::_Populate(UsdPrim subtreeRoot)
{
    TRACE_FUNCTION();
    if (!subtreeRoot) {
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries addedPrims;
    size_t lastEnd = 0;

    UsdPrimRange range(subtreeRoot, _GetTraversalPredicate());
    for (UsdPrim prim : range) {
        if (prim.IsPseudoRoot()) {
            continue;
        }

        if (prim.IsInstance()) {
            // XXX(USD-7119): Add native instancing support...
            continue;
        }

        _APISchemaAdapters adapters = _AdapterSetLookup(prim);

        // Enumerate the imaging sub-prims.
        const SdfPath primPath = prim.GetPath();
        const TfTokenVector subprims =
            _GetImagingSubprims(adapters);

        for (TfToken const& subprim : subprims) {
            const SdfPath subpath =
                subprim.IsEmpty() ? primPath : primPath.AppendChild(subprim);

            addedPrims.emplace_back(subpath,
                _GetImagingSubprimType(adapters, subprim));
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
    // XXX:(USD-7120) UsdImagingDelegate does a lot of really weird things
    // with traversal; traversal rules are different for point instancer
    // prototypes, or for display-cards-as-unloaded.  We prune non-imageable
    // typed prims, which prunes all materials in the scene, but then add
    // the materials back by following relationships.
    //
    // Ideally, going forward we can parse these features out so that the
    // UsdPrimRange traversal isn't impossible to follow.  For now we'll go
    // with the default predicate, and resolve special cases as they come up.
    return UsdPrimDefaultPredicate;
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

void
UsdImagingStageSceneIndex::ApplyPendingUpdates()
{
    if (!_stage ||
        (_usdPrimsToResync.empty() && _usdPropertiesToUpdate.empty())) {
        return;
    }

    TRACE_FUNCTION();

    // Resync first...
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

        TF_DEBUG(USDIMAGING_POPULATION).Msg("[Population] Removing <%s>\n",
                _usdPrimsToResync[i].GetText());
        _SendPrimsRemoved({_usdPrimsToResync[i]});
        _Populate(_stage->GetPrimAtPath(_usdPrimsToResync[i]));

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

    // Changed properties...
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedPrims;
    for (auto const& pair : _usdPropertiesToUpdate) {
        const SdfPath &primPath = pair.first;
        const TfTokenVector &properties = pair.second;
        // XXX: We could sort/unique the properties here...
        
        const UsdPrim prim = _stage->GetPrimAtPath(primPath);

        _APISchemaAdapters adapters = _AdapterSetLookup(prim);
        const TfTokenVector subprims = _GetImagingSubprims(adapters);

        for (TfToken const& subprim : subprims) {
            HdDataSourceLocatorSet dirtyLocators;

            for (const _APISchemaEntry &entry : adapters) {
                dirtyLocators.insert(entry.first->InvalidateImagingSubprim(
                    subprim, entry.second, properties));
            }

            if (!dirtyLocators.IsEmpty()) {
                SdfPath const subpath = subprim.IsEmpty()
                    ? primPath : primPath.AppendChild(subprim);
                dirtiedPrims.emplace_back(subpath, dirtyLocators);
            }
        }
    }

    _usdPrimsToResync.clear();
    _usdPropertiesToUpdate.clear();

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
