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
#include "pxr/imaging/hd/instancedBySceneIndex.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/lazyContainerDataSource.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/xformSchema.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdInstancedBySceneIndexTokens,
                        HD_INSTANCED_BY_SCENE_INDEX_TOKENS);

namespace {

// Given a prim, extracts prototype paths from instancer topology
// schema.
VtArray<SdfPath>
_PrototypesForInstancer(const HdSceneIndexPrim &prim)
{
    HdInstancerTopologySchema instancerTopology =
        HdInstancerTopologySchema::GetFromParent(prim.dataSource);
    if (HdPathArrayDataSourceHandle const ds = 
                    instancerTopology.GetPrototypes()) {
        return ds->GetTypedValue(0.0f);
    }
    return { };
}

// VtArray has no insert
void
_Insert(VtArray<SdfPath> &paths,
        const VtArray<SdfPath>::const_iterator &it,
        const SdfPath &path)
{
    // Iterators are invalidated by mutating functions on VtArray.
    const size_t i = it - paths.begin();
    paths.resize(paths.size() + 1);
    std::move_backward(paths.begin() + i, paths.end() - 1, paths.end());
    paths[i] = path;
}

// Maps a prim to all the instancers pointing at it.
//
class _PrototypeToInstancerMapping {
public:
    // Returns instancers that point to this prim in lexicographic
    // order.
    //
    // Returns empty array if no instancer is pointing at this prim
    // (and thus the prim is not a prototype).
    const VtArray<SdfPath> &
    GetInstancersForPrim(const SdfPath &primPath) const
    {
        TRACE_FUNCTION();

        const auto it = _prototypeToInstancerMap.find(primPath);
        if (it == _prototypeToInstancerMap.end()) {
            static const VtArray<SdfPath> empty;
            return empty;
        }
        return it->second;
    }

    // Adds entries prototype -> instancer to map.
    void AddPrototypes(
        const SdfPath &instancer,
        const VtArray<SdfPath> &prototypes)
   {
        for (const SdfPath &prototype : prototypes) {
            VtArray<SdfPath> &instancers = _prototypeToInstancerMap[prototype];
            const auto it = std::lower_bound(
                instancers.cbegin(), instancers.cend(), instancer);
            if (it != instancers.end() && (*it) == instancer) {
                continue;
            }
            _Insert(instancers, it, instancer);
        }
   }        

    // Remove entries prototype -> instancer to map.
    void RemovePrototypes(
        const SdfPath &instancer,
        const VtArray<SdfPath> &prototypes)
   {
       TRACE_FUNCTION();

       for (const SdfPath &prototype : prototypes) {
           const auto it = _prototypeToInstancerMap.find(prototype);
           if (it == _prototypeToInstancerMap.end()) {
               continue;
           }
           VtArray<SdfPath> &instancers = it->second;
           const auto instancerIt = std::lower_bound(
               instancers.cbegin(), instancers.cend(), instancer);
           if (instancerIt == instancers.cend()) {
               continue;
           }
           instancers.erase(instancerIt);
           if (instancers.empty()) {
               _prototypeToInstancerMap.erase(it);
           }
       }
   }

private:
    using _PathToPathsMap = std::map<SdfPath, VtArray<SdfPath>>;
    _PathToPathsMap _prototypeToInstancerMap;
};

void
_Append(const VtArray<SdfPath> &newPaths, SdfPathSet * const paths)
{
    if (paths) {
        paths->insert(newPaths.begin(), newPaths.end());
    }
}

}

class HdInstancedBySceneIndex::InstancerMapping
{
public:
    // Returns instancers for given primPath.
    //
    const VtArray<SdfPath> &
    GetInstancersForPrim(const SdfPath &primPath) const {
        return _prototypeToInstancerMapping.GetInstancersForPrim(primPath);
    }

    // Updates the mapping by clearing the old prototypes for the instancer
    // and setting the new ones.
    //
    // Optionally, gives the set of prims for which "instancedBy/paths"
    // has changed.
    //
    void SetPrototypesForInstancer(
             const SdfPath &instancer,
             const VtArray<SdfPath> &newPrototypes,
             SdfPathSet * const dirtiedPrims)
    {
        TRACE_FUNCTION();

        VtArray<SdfPath> &prototypes = _instancerToPrototypeMap[instancer];
        _prototypeToInstancerMapping.RemovePrototypes(instancer, prototypes);
        _Append(prototypes, dirtiedPrims);
        prototypes = newPrototypes;
        _prototypeToInstancerMapping.AddPrototypes(instancer, prototypes);
        _Append(prototypes, dirtiedPrims);
    }

    // Updates the map by removing all instancers with prefix primPath.
    //
    // Optionally, gives a set of prims like SetPrototypesForInstancer.
    //
    void RemoveInstancersUnderPrim(
            const SdfPath &primPath,
            SdfPathSet * const dirtiedPrims)
    {
        TRACE_FUNCTION();

        auto it = _instancerToPrototypeMap.lower_bound(primPath);
        while (it != _instancerToPrototypeMap.end() &&
               it->first.HasPrefix(primPath)) {
            _prototypeToInstancerMapping.RemovePrototypes(
                it->first, it->second);
            _Append(it->second, dirtiedPrims);
            it = _instancerToPrototypeMap.erase(it);
        }
    }

private:

    std::map<SdfPath, VtArray<SdfPath>> _instancerToPrototypeMap;
    _PrototypeToInstancerMapping _prototypeToInstancerMapping;
};

// ----------------------------------------------------------------------------

static
HdContainerDataSourceHandle
_XformDataSource(
    const SdfPath &primPath,
    HdInstancedBySceneIndex::InstancerMappingSharedPtr const &mapping)
{
    if (mapping->GetInstancersForPrim(primPath).empty()) {
        return nullptr;
    } else {
        static HdContainerDataSourceHandle const ds =
            HdXformSchema::Builder()
                .SetResetXformStack(
                    HdRetainedTypedSampledDataSource<bool>::New(true))
                .Build();
        return ds;
    }
}

static
HdContainerDataSourceHandle
_InstancedByPathsDataSource(
    const SdfPath &primPath,
    HdInstancedBySceneIndex::InstancerMappingSharedPtr const &mapping)
{
    using PathsDataSource = HdRetainedTypedSampledDataSource<VtArray<SdfPath>>;

    return 
        HdInstancedBySchema::Builder()
            .SetPaths(
                PathsDataSource::New(
                    mapping->GetInstancersForPrim(primPath)))
            .Build();
}

static
bool
_GetBool(HdContainerDataSourceHandle const &inputArgs,
         const TfToken &name)
{
    if (!inputArgs) {
        return false;
    }
    HdBoolDataSourceHandle const ds =
        HdBoolDataSource::Cast(
            inputArgs->Get(name));
    if (!ds) {
        return false;
    }
    return ds->GetTypedValue(0.0f);
}

HdInstancedBySceneIndex::HdInstancedBySceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene,
        HdContainerDataSourceHandle const &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputScene)
  , _resetXformStackForPrototypes(
      _GetBool(
          inputArgs,
          HdInstancedBySceneIndexTokens->resetXformStackForPrototypes))
  , _instancerMapping(std::make_shared<InstancerMapping>())
{
    _FillInstancerMapRecursively(SdfPath::AbsoluteRootPath());
}

HdInstancedBySceneIndex::~HdInstancedBySceneIndex() = default;

void
HdInstancedBySceneIndex::_FillInstancerMapRecursively(const SdfPath &primPath)
{
    HdSceneIndexPrim const prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.primType == HdPrimTypeTokens->instancer) {
        _instancerMapping->SetPrototypesForInstancer(
            primPath,
            _PrototypesForInstancer(prim),
            nullptr);
    }

    for (const SdfPath &childPath :
             _GetInputSceneIndex()->GetChildPrimPaths(primPath)) {
        _FillInstancerMapRecursively(childPath);
    }
}

HdSceneIndexPrim
HdInstancedBySceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    HdSceneIndexPrim const prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (!prim.dataSource) {
        return prim;
    }

    TfToken names[2];
    HdDataSourceBaseHandle sources[2];

    size_t n = 0;

    {
        names[n]   = HdInstancedBySchemaTokens->instancedBy;
        sources[n] = HdLazyContainerDataSource::New(
            std::bind(_InstancedByPathsDataSource,
                      primPath, _instancerMapping));
        n++;
    }

    if (_resetXformStackForPrototypes) {
        names[n]   = HdXformSchemaTokens->xform;
        sources[n] = HdLazyContainerDataSource::New(
            std::bind(_XformDataSource,
                      primPath, _instancerMapping));
        n++;
    }
    
    return { prim.primType,
             HdOverlayContainerDataSource::New(
                 prim.dataSource,
                 HdRetainedContainerDataSource::New(n, names, sources)) };
}

SdfPathVector
HdInstancedBySceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdInstancedBySceneIndex::_SendLocatorsDirtied(const SdfPathSet &paths)
{
    if (paths.empty()) {
        return;
    }

    static const HdDataSourceLocatorSet instancedByLocators{
        HdInstancedBySchema::GetDefaultLocator()};
    static const HdDataSourceLocatorSet instancedByAndXformLocators {
        HdInstancedBySchema::GetDefaultLocator(),
        HdXformSchema::GetDefaultLocator()};

    const HdDataSourceLocatorSet &locators =
        _resetXformStackForPrototypes
        ? instancedByAndXformLocators
        : instancedByLocators;

    HdSceneIndexObserver::DirtiedPrimEntries dirtyEntries;
    dirtyEntries.reserve(paths.size());
    for (const SdfPath &path : paths) {
        dirtyEntries.emplace_back(path, locators);
    }
    _SendPrimsDirtied(dirtyEntries);
}

void
HdInstancedBySceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    const bool isObserved = _IsObserved();

    SdfPathSet dirtiedPrims;

    // Add new instancers to the instancer mapping table.
    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        if (entry.primType == HdPrimTypeTokens->instancer) {
            HdSceneIndexPrim const prim = _GetInputSceneIndex()->GetPrim(
                entry.primPath);
            _instancerMapping->SetPrototypesForInstancer(
                entry.primPath,
                _PrototypesForInstancer(prim),
                isObserved ? &dirtiedPrims : nullptr);
        }
    }
        
    if (!isObserved) {
        return;
    }

    _SendPrimsAdded(entries);
    _SendLocatorsDirtied(dirtiedPrims);
}

void
HdInstancedBySceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    const bool isObserved = _IsObserved();

    static const HdDataSourceLocator prototypesLocator =
        HdInstancerTopologySchema::GetDefaultLocator().Append(
            HdInstancerTopologySchemaTokens->prototypes);

    SdfPathSet dirtiedPrims;
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        if (entry.dirtyLocators.Contains(prototypesLocator)) {
            HdSceneIndexPrim const prim = _GetInputSceneIndex()->GetPrim(
                entry.primPath);
            _instancerMapping->SetPrototypesForInstancer(
                entry.primPath,
                _PrototypesForInstancer(prim),
                isObserved ? &dirtiedPrims : nullptr);
        }
    }
    if (!isObserved) {
        return;
    }

    // Pass along the dirty notification.
    _SendPrimsDirtied(entries);

    _SendLocatorsDirtied(dirtiedPrims);
}

void
HdInstancedBySceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    const bool isObserved = _IsObserved();

    SdfPathSet dirtiedPrims;
    
    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        _instancerMapping->RemoveInstancersUnderPrim(
            entry.primPath, isObserved ? &dirtiedPrims : nullptr);
    }

    if (!isObserved) {
        return;
    }

    _SendPrimsRemoved(entries);

    if (dirtiedPrims.empty()) {
        return;
    }

    // We do not send out dirtied messages for prims that we just removed.
    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        auto it = dirtiedPrims.lower_bound(entry.primPath);
        while (it != dirtiedPrims.end() && it->HasPrefix(entry.primPath)) {
            it = dirtiedPrims.erase(it);
        }
    }

    _SendLocatorsDirtied(dirtiedPrims);
}

PXR_NAMESPACE_CLOSE_SCOPE
