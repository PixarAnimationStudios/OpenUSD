//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/legacyGeomSubsetSceneIndex.h"

#include "pxr/imaging/hd/basisCurvesTopology.h"
#include "pxr/imaging/hd/basisCurvesTopologySchema.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLegacyPrim.h"
#include "pxr/imaging/hd/dataSourceLocator.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/geomSubset.h"
#include "pxr/imaging/hd/geomSubsetSchema.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/meshTopologySchema.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/visibilitySchema.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"

#include "pxr/pxr.h"

#include <algorithm>
#include <iterator>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((invisiblePoints, "__invisiblePoints"))
    ((invisibleCurves, "__invisibleCurves"))
    ((invisibleFaces,  "__invisibleFaces" )));

namespace {

HdSceneDelegate*
_GetSceneDelegate(const HdContainerDataSourceHandle& primSource)
{
    if (primSource) {
        if (const auto ds = HdTypedSampledDataSource<HdSceneDelegate*>::Cast(
            primSource->Get(HdSceneIndexEmulationTokens->sceneDelegate))) {
            return ds->GetTypedValue(0.0f);
        }
    }
    return nullptr;
}

// added     = set_difference(after, before) (after - before)
// removed   = set_difference(before, after) (before - after)
// unchanged = set_intersection(before, after)
// Note: inputs must be sorted!
template<class InputIt1, class InputIt2, class OutputIt>
void set_differences(
    InputIt1 beforeIt, InputIt1 beforeEndIt,
    InputIt2 afterIt, InputIt2 afterItEnd,
    OutputIt addedIt, OutputIt removedIt, OutputIt unchangedIt)
{
    while (beforeIt != beforeEndIt && afterIt != afterItEnd) {
        if (*beforeIt < *afterIt) {
            *removedIt++ = *beforeIt++;
        } else {
            if (*afterIt < *beforeIt) {
                *addedIt++ = *afterIt++;
            } else {
                *unchangedIt++ = *beforeIt++;
                ++afterIt;
            }
        }
    }
    while (beforeIt != beforeEndIt) {
        *removedIt++ = *beforeIt++;
    }
    while (afterIt != afterItEnd) {
        *addedIt++ = *afterIt++;
    }
}

// XXX: This class derives from HdDataSourceLegacyPrim primarily for asthetic
// reasons. It relies on the base class only for access to the scene delegate.
class _HdDataSourceLegacyGeomSubset
  : public HdDataSourceLegacyPrim
{
public:
    HD_DECLARE_DATASOURCE(_HdDataSourceLegacyGeomSubset);
    
    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken& name) override;
    
private:
    _HdDataSourceLegacyGeomSubset(
        const SdfPath& id,
        const SdfPath& parentId,
        const TfToken& parentType,
        HdSceneDelegate* sceneDelegate);
    
    struct _Subset
    {
        TfToken type;
        VtIntArray indices;
        bool visibility = false;
        SdfPath materialBinding;
        
        operator bool() const {
            return (!type.IsEmpty() && !indices.empty());
        }
    };
    
    _Subset
    _FindSubset() const;
    
    SdfPath _parentId;
    TfToken _parentType;
};

HD_DECLARE_DATASOURCE_HANDLES(_HdDataSourceLegacyGeomSubset);

_HdDataSourceLegacyGeomSubset::_HdDataSourceLegacyGeomSubset(
    const SdfPath& id, const SdfPath& parentId, const TfToken& parentType,
    HdSceneDelegate* delegate)
  : HdDataSourceLegacyPrim(id, HdPrimTypeTokens->geomSubset, delegate)
  , _parentId(parentId)
  , _parentType(parentType)
{
    TF_VERIFY(HdPrimTypeSupportsGeomSubsets(_parentType));
}

TfTokenVector
_HdDataSourceLegacyGeomSubset::GetNames()
{
    TfTokenVector names;
    names.push_back(HdGeomSubsetSchema::GetSchemaToken());
    names.push_back(HdVisibilitySchema::GetSchemaToken());
    names.push_back(HdMaterialBindingsSchema::GetSchemaToken());
    names.push_back(HdPrimvarsSchema::GetSchemaToken());
    names.push_back(HdSceneIndexEmulationTokens->sceneDelegate);
    return names;
}

HdDataSourceBaseHandle
_HdDataSourceLegacyGeomSubset::Get(const TfToken& name)
{
    if (name == HdGeomSubsetSchema::GetSchemaToken()) {
        if (const _Subset& subset = _FindSubset()) {
            return HdGeomSubsetSchema::Builder()
                .SetType(HdRetainedTypedSampledDataSource<TfToken>::New(
                    subset.type))
                .SetIndices(HdRetainedTypedSampledDataSource<VtIntArray>::New(
                    subset.indices))
                .Build();
        }
    }
    
    // We must intercept visibility and materialBindings because the
    // base class does not know how to compute these for geom subsets.
    if (name == HdVisibilitySchema::GetSchemaToken()) {
        if (const _Subset& subset = _FindSubset()) {
            return HdVisibilitySchema::Builder()
                .SetVisibility(HdRetainedTypedSampledDataSource<bool>::New(
                    subset.visibility))
                .Build();
        }
    }
    if (name == HdMaterialBindingsSchema::GetSchemaToken()) {
        if (const _Subset& subset = _FindSubset()) {
            if (!subset.materialBinding.IsEmpty()) {
                static const TfTokenVector names {
                    HdMaterialBindingsSchemaTokens->allPurpose };
                const std::vector<HdDataSourceBaseHandle> sources {
                    HdMaterialBindingSchema::Builder()
                        .SetPath(HdRetainedTypedSampledDataSource<SdfPath>::New(
                            subset.materialBinding))
                        .Build() };
                return HdMaterialBindingsSchema::BuildRetained(
                    names.size(), names.data(), sources.data());
            }
        }
    }

    // We must intercept primvars, otherwise the base class will try to look
    // them up via the scene delegate. We return empty primvars instead of
    // just a nullptr because a lot of stuff downstream expects everything
    // to have primvars. See above at HdDataSourceLegacyPrim::GetNames().
    if (name == HdPrimvarsSchema::GetSchemaToken()) {
        static const HdContainerDataSourceHandle emptyPrimvarsDs =
            HdPrimvarsSchema::BuildRetained(0, nullptr, nullptr);
        return emptyPrimvarsDs;
    }
    
    // To block everything else, and so prevent calling something on the
    // sceneDelegate for a geom subset path about which it knows nothing, we
    // only defer to base class for sceneDelegate.
    if (name == HdSceneIndexEmulationTokens->sceneDelegate) {
        return HdDataSourceLegacyPrim::Get(name);
    }
    
    return nullptr;
}

_HdDataSourceLegacyGeomSubset::_Subset
_HdDataSourceLegacyGeomSubset::_FindSubset() const
{
    const TfToken& name = _id.GetNameToken();
    if (_parentType == HdPrimTypeTokens->basisCurves) {
        const HdBasisCurvesTopology& topo =
            _sceneDelegate->GetBasisCurvesTopology(_parentId);
        if (name == _tokens->invisibleCurves) {
            return { HdGeomSubsetSchemaTokens->typeCurveSet,
                topo.GetInvisibleCurves() };
        }
        if (name == _tokens->invisiblePoints) {
            return { HdGeomSubsetSchemaTokens->typePointSet,
                topo.GetInvisiblePoints() };
        }
    } else if (_parentType == HdPrimTypeTokens->mesh) {
        const HdMeshTopology& topo = _sceneDelegate->GetMeshTopology(_parentId);
        if (name == _tokens->invisibleFaces) {
            return { HdGeomSubsetSchemaTokens->typeFaceSet,
                topo.GetInvisibleFaces() };
        }
        if (name == _tokens->invisiblePoints) {
            return { HdGeomSubsetSchemaTokens->typePointSet,
                topo.GetInvisiblePoints() };
        }
        for (const HdGeomSubset& subset : topo.GetGeomSubsets()) {
            if (subset.id.GetNameToken() == name) {
                return { HdGeomSubsetSchemaTokens->typeFaceSet,
                    subset.indices, true, subset.materialId };
            }
        }
    } else {
        // XXX: At construction, we checked _parentType with
        // HdPrimTypeSupportsGeomSubsets(), so this should not happen.
        TF_CODING_ERROR("Unsupported geomSubset parent type: `%s`",
            _parentType.GetText());
    }
    return _Subset();
}

} // anonymous namespace

HdLegacyGeomSubsetSceneIndexRefPtr
HdLegacyGeomSubsetSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(
        new HdLegacyGeomSubsetSceneIndex(inputSceneIndex));
}

HdLegacyGeomSubsetSceneIndex::HdLegacyGeomSubsetSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{ }

HdLegacyGeomSubsetSceneIndex::~HdLegacyGeomSubsetSceneIndex() = default;

HdSceneIndexPrim
HdLegacyGeomSubsetSceneIndex::GetPrim(const SdfPath& primPath) const
{
    const HdSceneIndexPrim& prim = _GetInputSceneIndex()->GetPrim(primPath);
    // If primPath is for a legacy subset, the input scene index will
    // not know anything about it. We can bail early if dataSorce is null.
    if (!prim.dataSource) {
        const SdfPath& parentPath = primPath.GetParentPath();
        if (_parentPrims.find(parentPath) != _parentPrims.end()) {
            const HdSceneIndexPrim& parent =
                _GetInputSceneIndex()->GetPrim(parentPath);
            if (HdSceneDelegate* delegate =
                _GetSceneDelegate(parent.dataSource)) {
                return {
                    HdPrimTypeTokens->geomSubset,
                    _HdDataSourceLegacyGeomSubset::New(
                        primPath, parentPath,
                        parent.primType, delegate) };
            }
        }
    }
    return prim;
}

SdfPathVector
HdLegacyGeomSubsetSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    // XXX: To the extent there are authored mesh subsets in here, we must
    // return them in their original authored order. We could get that by
    // pulling the mesh topology from the delegate, but by doing so we would be
    // giving up the ability to do fine-grained invalidation in _PrimsDirtied.
    SdfPathVector paths = _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    const auto it = _parentPrims.find(primPath);
    if (it != _parentPrims.end()) {
        paths.insert(paths.end(), it->second.begin(), it->second.end());
    }
    return paths;
}

void
HdLegacyGeomSubsetSceneIndex::_PrimsAdded(
    const HdSceneIndexBase&  /*sender*/,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{                
    HdSceneIndexObserver::AddedPrimEntries newEntries;
    for (const HdSceneIndexObserver::AddedPrimEntry& entry : entries) {
        if (!HdPrimTypeSupportsGeomSubsets(entry.primType)) {
            continue;
        }
        const HdSceneIndexPrim& prim =
            _GetInputSceneIndex()->GetPrim(entry.primPath);
        const SdfPathVector& paths = _ListDelegateSubsets(entry.primPath, prim);
        if (paths.empty()) {
            continue;
        }
        // Only add prims with subsets to _parentPrims to save on memory.
        _parentPrims.insert({ entry.primPath, paths });        
        for (const SdfPath& path : paths) {
            newEntries.push_back({ path, HdPrimTypeTokens->geomSubset });
        }
    }
    if (!newEntries.empty()) {
        newEntries.insert(newEntries.begin(), entries.cbegin(), entries.cend());
        return _SendPrimsAdded(newEntries);
    }
    _SendPrimsAdded(entries);
}

void
HdLegacyGeomSubsetSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase&  /*sender*/,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    HdSceneIndexObserver::RemovedPrimEntries removedEntries;
    for (const HdSceneIndexObserver::RemovedPrimEntry& entry : entries) {
        auto it = _parentPrims.find(entry.primPath);
        if (it != _parentPrims.end()) {
            for (const SdfPath& path : it->second) {
                removedEntries.push_back({ path });
            }
            _parentPrims.erase(it);
        }
    }
    if (!removedEntries.empty()) {
        removedEntries.insert(
            removedEntries.begin(), entries.cbegin(), entries.cend());
        return _SendPrimsRemoved(removedEntries);
    }
    _SendPrimsRemoved(entries);
}

void
HdLegacyGeomSubsetSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& /*sender*/,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    // XXX: We cache each parent prim's subset paths so we can tell when
    // dirty topology means one or more subsets were added or removed.
    // Otherwise, we would have to remove and add every subset every time the
    // topology was dirty, even when the list of subsets does not change.
    // We always send an add, remove, or dirty signal for every subset though.
    // Downstream consumers may be able to handle a dirty subset more
    // efficiently than a destroyed and recreated one.
    static const HdDataSourceLocatorSet topologyLocators {
        HdBasisCurvesTopologySchema::GetDefaultLocator(),
        HdMeshTopologySchema::GetDefaultLocator() };
    static const HdDataSourceLocatorSet emptyLocatorSet {
        HdDataSourceLocator::EmptyLocator() };
        
    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    HdSceneIndexObserver::RemovedPrimEntries removedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;
    
    for (const HdSceneIndexObserver::DirtiedPrimEntry& entry : entries) {
        if (!entry.dirtyLocators.Intersects(topologyLocators)) {
            // If the change didn't affect topology, we can continue. This is
            // either not a mesh/basisCurves or the subsets didn't change.
            continue;
        }
        const HdSceneIndexPrim& prim =
            _GetInputSceneIndex()->GetPrim(entry.primPath);
        // Immediately fetch the new list of subsets
        SdfPathVector after  = _ListDelegateSubsets(entry.primPath, prim);
        auto it = _parentPrims.find(entry.primPath);
        if (it == _parentPrims.end()) {
            // This mesh/basisCurves did not previously have subsets
            // but some may have been added. If none have been added,
            // we can continue.
            if (after.empty()) {
                continue;
            }
            // There are new subsets for this mesh/basisCurves;
            // add an entry to _parentPrims.
            it = _parentPrims.insert({entry.primPath, {}}).first;
        }
        SdfPathVector before = it->second; // a copy!
        SdfPathVector added, removed, dirtied;
        // update cached child paths
        it->second.clear();
        it->second.insert(it->second.end(), after.begin(), after.end());
        if (before.empty()) {
            added = after;
        } else if (after.empty()) {
            removed = before;
        } else {
            // set_differences requires before and after to be sorted
            std::sort(before.begin(), before.end());
            std::sort(after.begin(), after.end());
            set_differences(
                before.begin(), before.end(),
                after.begin(), after.end(),
                std::back_inserter(added),
                std::back_inserter(removed),
                std::back_inserter(dirtied));
        }
        for (const SdfPath& path : added) {
            addedEntries.push_back({ path, HdPrimTypeTokens->geomSubset });
        }
        for (const SdfPath& path : removed) {
            removedEntries.push_back({ path });
        }
        for (const SdfPath& path : dirtied) {
            dirtiedEntries.push_back({ path, emptyLocatorSet });
        }
    }
    if (!removedEntries.empty()) {
        _SendPrimsRemoved(removedEntries);
    }
    if (!dirtiedEntries.empty()) {
        dirtiedEntries.insert(
            dirtiedEntries.begin(), entries.cbegin(), entries.cend());
        _SendPrimsDirtied(dirtiedEntries);
    } else {
        _SendPrimsDirtied(entries);
    }
    if (!addedEntries.empty()) {
        _SendPrimsAdded(addedEntries);
    }
}

SdfPathVector
HdLegacyGeomSubsetSceneIndex::_ListDelegateSubsets(
    const SdfPath& parentPath,
    const HdSceneIndexPrim& parentPrim) 
{
    SdfPathVector paths;
    if (!parentPrim.dataSource ||
        !HdPrimTypeSupportsGeomSubsets(parentPrim.primType)) {
        return paths;
    }
    if (HdSceneDelegate* delegate = _GetSceneDelegate(parentPrim.dataSource)) {
        if (parentPrim.primType == HdPrimTypeTokens->basisCurves) {
            const HdBasisCurvesTopology& topo =
                delegate->GetBasisCurvesTopology(parentPath);
            if (!topo.GetInvisibleCurves().empty()) {
                paths.push_back(parentPath.AppendChild(
                    _tokens->invisibleCurves));
            }
            if (!topo.GetInvisiblePoints().empty()) {
                paths.push_back(parentPath.AppendChild(
                    _tokens->invisiblePoints));
            }
        } else if (parentPrim.primType == HdPrimTypeTokens->mesh) {
            const HdMeshTopology& topo =
                delegate->GetMeshTopology(parentPath);
            for (const HdGeomSubset& subset : topo.GetGeomSubsets()) {
                paths.push_back(parentPath.AppendChild(
                    subset.id.GetNameToken()));
            }
            if (!topo.GetInvisibleFaces().empty()) {
                paths.push_back(parentPath.AppendChild(
                    _tokens->invisibleFaces));
            }
            if (!topo.GetInvisiblePoints().empty()) {
                paths.push_back(parentPath.AppendChild(
                    _tokens->invisiblePoints));
            }
            
        }
    }
    return paths;
}

PXR_NAMESPACE_CLOSE_SCOPE
