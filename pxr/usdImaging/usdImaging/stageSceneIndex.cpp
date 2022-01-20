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
#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include <queue>

PXR_NAMESPACE_OPEN_SCOPE

// ---------------------------------------------------------------------------
// Adapter delegation

TfTokenVector
UsdImagingStageSceneIndex::_GetImagingSubprims(UsdPrim prim) const
{
    UsdImagingPrimAdapterSharedPtr adapter = _AdapterLookup(prim);
    if (adapter) {
        TfTokenVector subprims;
        subprims = adapter->GetImagingSubprims();

        // Enforce that the trivial subprim "" always exists, to pick up
        // inherited attributes and for traversal purposes.
        if (std::find(subprims.begin(), subprims.end(), TfToken())
                == subprims.end()) {
            subprims.push_back(TfToken());
        }
    }

    static TfTokenVector s_default = { TfToken() };
    return s_default;
}

TfToken
UsdImagingStageSceneIndex::_GetImagingSubprimType(
        UsdPrim prim, TfToken const& subprim) const
{
    UsdImagingPrimAdapterSharedPtr adapter = _AdapterLookup(prim);
    if (adapter) {
        return adapter->GetImagingSubprimType(subprim);
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingStageSceneIndex::_GetImagingSubprimData(
        UsdPrim prim, TfToken const& subprim) const
{
    UsdImagingPrimAdapterSharedPtr adapter = _AdapterLookup(prim);
    if (adapter) {
        HdContainerDataSourceHandle ds =
            adapter->GetImagingSubprimData(subprim, prim, _stageGlobals);
        if (ds) {
            return ds;
        }
    }

    // If there's no adapter/the adapter didn't handle the subprim, and the
    // subprim is the trivial one "", create a dataSourcePrim to pick up
    // inherited attributes.
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourcePrim::New(
                prim.GetPath(), prim, _stageGlobals);
    }

    return nullptr;
}

void
UsdImagingStageSceneIndex::_PopulateAdapterMap()
{
    TRACE_FUNCTION();

    UsdImagingAdapterRegistry& reg = UsdImagingAdapterRegistry::GetInstance();
    const TfTokenVector& adapterKeys = reg.GetAdapterKeys();

    for (TfToken const& adapterKey : adapterKeys) {
        _adapterMap.insert({adapterKey, reg.ConstructAdapter(adapterKey)});
    }
}

UsdImagingPrimAdapterSharedPtr
UsdImagingStageSceneIndex::_AdapterLookup(UsdPrim prim) const
{
    // Minus the draw mode & instance stuff, this is designed to match
    // UsdImagingDelegate::_AdapterLookup. In the future we might want to do
    // imaging behavior composition based on typeInfo.GetAppliedAPISchemas()
    // or something, instead of hardcoding the LightAPI reference...

    const UsdPrimTypeInfo &typeInfo = prim.GetPrimTypeInfo();

    _AdapterMap::const_iterator it =
        _adapterMap.find(typeInfo.GetSchemaTypeName());
    if(it != _adapterMap.end() && TF_VERIFY(it->second)) {
        return it->second;
    }

    // XXX: Note that we're hardcoding handling for LightAPI here to match
    // UsdImagingDelegate, but the hope is to more generally support imaging
    // behaviors for API classes in the future.
    if (prim.HasAPI<UsdLuxLightAPI>()) {
        it = _adapterMap.find(TfToken("LightAPI"));
        if (it != _adapterMap.end() && TF_VERIFY(it->second)) {
            return it->second;
        }
    }

    return nullptr;
}

// ---------------------------------------------------------------------------

UsdImagingStageSceneIndex::UsdImagingStageSceneIndex()
{
}

// ---------------------------------------------------------------------------

HdSceneIndexPrim
UsdImagingStageSceneIndex::GetPrim(const SdfPath &path) const
{
    static const HdSceneIndexPrim s_emptyPrim = {TfToken(), nullptr};

    if (!_stage) {
        return s_emptyPrim;
    }

    SdfPath primPath = path.GetPrimPath();

    UsdPrim prim = _stage->GetPrimAtPath(primPath);
    if (!prim) {
        return s_emptyPrim;
    }

    TfToken subprim = path.IsPropertyPath() ? path.GetNameToken() : TfToken();

    TfToken imagingType = _GetImagingSubprimType(prim, subprim);
    HdContainerDataSourceHandle dataSource =
        _GetImagingSubprimData(prim, subprim);

    return {imagingType, dataSource};
}

// ---------------------------------------------------------------------------

SdfPathVector
UsdImagingStageSceneIndex::GetChildPrimPaths(
        const SdfPath &path) const
{
    if (!_stage) {
        return {};
    }

    // If this is a subprim path, treat it as a leaf.
    if (!path.IsPrimPath()) {
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

    SdfPath const& primPath = prim.GetPath();
    TfTokenVector subprims = _GetImagingSubprims(prim);
    for (TfToken const& subprim : subprims) {
        if (!subprim.IsEmpty()) {
            result.push_back(primPath.AppendChild(subprim));
        }
    }

    return result;
}

// ---------------------------------------------------------------------------

void UsdImagingStageSceneIndex::SetTime(UsdTimeCode time)
{
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
    if (_stage) {
        _SendPrimsRemoved({SdfPath::AbsoluteRootPath()});
        _stageGlobals.Clear();
        _adapterMap.clear();
    }
    _stage = stage;
    _PopulateAdapterMap();
}

void UsdImagingStageSceneIndex::Populate()
{
    if (!_stage) {
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries addedPrims;

    std::queue<UsdPrim> roots;
    roots.push(_stage->GetPseudoRoot());
    while (!roots.empty()) {
        UsdPrim root = roots.front();
        roots.pop();

        UsdPrimRange range(root, _GetTraversalPredicate());
        for (UsdPrim prim : range) {
            if (prim.IsPseudoRoot()) {
                continue;
            }

            if (prim.IsInstance()) {
                // XXX(USD-7119): Add native instancing support...
                continue;
            }

            // Enumerate the imaging sub-prims.
            SdfPath const& primPath = prim.GetPath();
            const TfTokenVector subprims = _GetImagingSubprims(prim);
            for (TfToken const& subprim : subprims) {
                SdfPath const subpath = subprim.IsEmpty()
                    ? primPath : primPath.AppendChild(subprim);
                addedPrims.emplace_back(subpath,
                        _GetImagingSubprimType(prim, subprim));
            }
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
