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
#include "pxr/usdImaging/usdImaging/piPrototypePropagatingSceneIndex.h"

#include "pxr/usdImaging/usdImaging/piPrototypeSceneIndex.h"
#include "pxr/usdImaging/usdImaging/rerootingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/sceneIndexPrimView.h"

#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(USDIMAGING_SHOW_POINT_PROTOTYPE_SCENE_INDICES, false,
                      "If true, the prototype propagating scene index will "
                      "list as input scene indices all intermediate scene "
                      "indices for all prototypes.");

namespace UsdImagingPiPrototypePropagatingSceneIndex_Impl
{

struct _Context
{
    _Context(
        HdSceneIndexBaseRefPtr const &inputSceneIndex)
      : inputSceneIndex(inputSceneIndex)
      , instancerSceneIndex(HdRetainedSceneIndex::New())
      , mergingSceneIndex(HdMergingSceneIndex::New())
    {
        mergingSceneIndex->AddInputScene(
            instancerSceneIndex, SdfPath::AbsoluteRootPath());
    }

    HdSceneIndexBaseRefPtr const inputSceneIndex;
    /// Scene index used to override the instancerTopology::prototypes
    /// data sources of instancers to account for the re-rooting.
    HdRetainedSceneIndexRefPtr const instancerSceneIndex;
    /// Our "output" scene index.
    HdMergingSceneIndexRefPtr const mergingSceneIndex;
};

/// \class _InstancerObserver
///
/// A scene index observer that adds the root of a scene or re-rooted
/// prototypes to the mergingSceneIndex.
///
/// It observes the scene to detect instancers within the prototype.
///
/// It querries the instancer for its prototypes to then add (recursively)
/// _InstancerObservers to add re-rooted copies of the prototype and update the
/// instancers with the re-rooted paths by authoring stronger opinions
/// in the instancerSceneIndex.
///
class _InstancerObserver final : public HdSceneIndexObserver
{
public:
    _InstancerObserver(
        _ContextSharedPtr const &context);

    /// Adds the prims under prototypeRoot at rerootedPrototypeRoot
    /// and sets the instancedBy:paths data source of those prims
    /// to instancer.
    _InstancerObserver(
        _ContextSharedPtr const &context,
        const SdfPath &instancer,
        const SdfPath &prototypeRoot,
        const SdfPath &rerootedPrototypeRoot);

    ~_InstancerObserver();

    void PrimsAdded(
        const HdSceneIndexBase &sender,
        const AddedPrimEntries &entries) override;

    void PrimsDirtied(
        const HdSceneIndexBase &sender,
        const DirtiedPrimEntries &entries) override;

    void PrimsRemoved(
        const HdSceneIndexBase &sender,
        const RemovedPrimEntries &entries) override;

private:
    // For point instancers nested within this prototype, we store the map
    // instancer -> prototype -> _InstancerObserver.
    using _Map0 = std::map<SdfPath, _InstancerObserverUniquePtr>;
    using _Map1 = std::map<SdfPath, _Map0>;

    SdfPath _RerootedPath(const SdfPath &instancer) const;
    // Create a unique name for re-rooted prototypes for instancers within
    // this prototype.
    TfToken _InstancerHash(const SdfPath &instancer) const;

    // Create or destory instancerObservers for an instancer within this
    // prototype.
    void _UpdateInstancerPrototypes(
        _Map0 * prototypeToInstancerObserver,
        const SdfPath &instancer,
        const VtArray<SdfPath> &prototypes);

    // Convenience methods for _UpdateInstancerPrototypes.
    void _UpdateInstancer(
        _Map0 * prototypeToInstancerObserver, const SdfPath &path);
    void _UpdateInstancer(
        const SdfPath &path, const HdSceneIndexPrim &prim);
    void _UpdateInstancer(
        const SdfPath &path);

    void _Populate();

    _ContextSharedPtr const _context;
    
    const SdfPath _prototypeRoot;
    const SdfPath _rerootedPrototypeRoot;

    HdSceneIndexBaseRefPtr const _prototypeSceneIndex;
    HdSceneIndexBaseRefPtr const _rerootingSceneIndex;

    // instancer -> prototype -> instancerObserver
    _Map1 _subinstancerObservers;
};

HdSceneIndexBaseRefPtr
_RerootingSceneIndex(HdSceneIndexBaseRefPtr const &sceneIndex,
                     const SdfPath &srcPrefix,
                     const SdfPath &dstPrefix)
{
    if (srcPrefix.IsAbsoluteRootPath() && dstPrefix.IsAbsoluteRootPath()) {
        return sceneIndex;
    } else {
        return UsdImagingRerootingSceneIndex::New(
            sceneIndex, srcPrefix, dstPrefix);
    }
}

_InstancerObserver::_InstancerObserver(_ContextSharedPtr const &context)
  : _InstancerObserver(
      context,
      /* instancer = */ SdfPath(),
      /* prototypeRoot = */ SdfPath::AbsoluteRootPath(),
      /* rerootedPrototypeRoot = */ SdfPath::AbsoluteRootPath())
{
}

_InstancerObserver::_InstancerObserver(
        _ContextSharedPtr const &context,
        const SdfPath &instancer,
        const SdfPath &prototypeRoot,
        const SdfPath &rerootedPrototypeRoot)
  : _context(context)
  , _prototypeRoot(prototypeRoot)
  , _rerootedPrototypeRoot(rerootedPrototypeRoot)
  , _prototypeSceneIndex(
      UsdImaging_PiPrototypeSceneIndex::New(
          // Isolate the prototype
          _RerootingSceneIndex(
              context->inputSceneIndex,
              prototypeRoot, prototypeRoot),
          instancer,
          prototypeRoot))
  , _rerootingSceneIndex(
      _RerootingSceneIndex(
          _prototypeSceneIndex,
          prototypeRoot, rerootedPrototypeRoot))
{    
    _context->mergingSceneIndex->AddInputScene(
        _rerootingSceneIndex,
        rerootedPrototypeRoot);

    _prototypeSceneIndex->AddObserver(HdSceneIndexObserverPtr(this));

    _Populate();
}

_InstancerObserver::~_InstancerObserver()
{
    // _InstancerObserver is RAII.
    // Upon deletion, it removes the scene indices and prims it added
    // to the merging scene index and retained scene index, respectively.

    if (!_subinstancerObservers.empty()) {
        HdSceneIndexObserver::RemovedPrimEntries removedInstancers;
        removedInstancers.reserve(_subinstancerObservers.size());
        
        for (const auto &instancerAndObserver : _subinstancerObservers) {
            const SdfPath &instancer = instancerAndObserver.first;
            removedInstancers.emplace_back(_RerootedPath(instancer));
        }

        _context->instancerSceneIndex->RemovePrims(removedInstancers);

        _subinstancerObservers.clear();
    }
    // We remove the scene indices in the order opposite to how we
    // added them.
    _context->mergingSceneIndex->RemoveInputScene(_rerootingSceneIndex);
}

SdfPath
_InstancerObserver::_RerootedPath(const SdfPath &instancer) const
{
    return instancer.ReplacePrefix(_prototypeRoot, _rerootedPrototypeRoot);
}

TfToken
_InstancerObserver::_InstancerHash(const SdfPath &instancer) const
{
    // Compute name when making a re-rooted copy of the prototype.
    //
    // This name uses the (1) instancer name and (2) the re-rooted path of the
    // prototype inserted by this _InstancerObserver.
    //
    // This is for the following reasons:
    // (1) Two instancers within this prototype could instance the same
    // prototype.
    //
    // (2) This prototype could have been instantiated by two different
    // instancers and we have two _InstancerObservers, each one needs to in
    // turn insert the same prototype under different names. Note that the
    // re-rooted path of this prototype contains the instancer hash, so we
    // actually compute a chain of hashes if we have nested point instancers.
    //

    const size_t h = TfHash::Combine(instancer, _rerootedPrototypeRoot);
    
    return TfToken(TfStringPrintf("ForInstancer%zx", h));
}

HdContainerDataSourceHandle
_InstancerTopology(const VtArray<SdfPath> &prototypes)
{
    return
        HdRetainedContainerDataSource::New(
            HdInstancerTopologySchemaTokens->instancerTopology,
            HdInstancerTopologySchema::Builder()
                .SetPrototypes(
                    HdRetainedTypedSampledDataSource<VtArray<SdfPath>>::New(
                        prototypes))
                .Build());
    
}

void
_InstancerObserver::_UpdateInstancerPrototypes(
    _Map0 * const prototypeToInstancerObserver,
    const SdfPath &instancer,
    const VtArray<SdfPath> &prototypes)
{
    const SdfPath rerootedInstancer = _RerootedPath(instancer);

    {
        // Remove _InstancerObservers for prims that are no longer targeted by the
        // instancer's prototype relationship.
        const SdfPathSet prototypeSet(prototypes.begin(), prototypes.end());
        for (auto it = prototypeToInstancerObserver->begin();
             it != prototypeToInstancerObserver->end(); ) {
            if (prototypeSet.find(it->first) == prototypeSet.end()) {
                it = prototypeToInstancerObserver->erase(it);
            } else {
                ++it;
            }
        }
    }

    // Compute the re-rooted paths for the instancer's prototypes.
    // Add a _InstancerObserver for the re-rooted path if there wasn't a _InstancerObserver
    // already.
    VtArray<SdfPath> rerootedPrototypes;
    rerootedPrototypes.reserve(prototypes.size());
    for (const SdfPath &prototype : prototypes) {
        const SdfPath rerootedPrototype =
            prototype.AppendChild(
                _InstancerHash(instancer));
        rerootedPrototypes.push_back(rerootedPrototype);
        _InstancerObserverUniquePtr &observer = 
            (*prototypeToInstancerObserver)[prototype];
        if (!observer) {
            observer = std::make_unique<_InstancerObserver>(
                _context, rerootedInstancer, prototype, rerootedPrototype);
        }
    }

    // Update the instancer's prototypes to point to the re-rooted prototypes.
    _context->instancerSceneIndex->AddPrims(
        { { rerootedInstancer,
            HdPrimTypeTokens->instancer,
            _InstancerTopology(rerootedPrototypes) } });
}

VtArray<SdfPath>
_GetPrototypes(const HdSceneIndexPrim &instancer)
{
    HdInstancerTopologySchema topologySchema =
        HdInstancerTopologySchema::GetFromParent(instancer.dataSource);
    if (HdPathArrayDataSourceHandle const ds = topologySchema.GetPrototypes()) {
        return ds->GetTypedValue(0.0f);
    } else {
        return {};
    }
}

void
_InstancerObserver::_UpdateInstancer(
    _Map0 * const prototypeToInstancerObserver,
    const SdfPath &path)
{
    _UpdateInstancerPrototypes(
        prototypeToInstancerObserver,
        path,
        _GetPrototypes(_prototypeSceneIndex->GetPrim(path)));
}

void
_InstancerObserver::_UpdateInstancer(
    const SdfPath &path,
    const HdSceneIndexPrim &prim)
{
    _UpdateInstancerPrototypes(
        &_subinstancerObservers[path],
        path,
        _GetPrototypes(prim));
}

void
_InstancerObserver::_UpdateInstancer(
    const SdfPath &path)
{
    _UpdateInstancer(
        &_subinstancerObservers[path],
        path);
}

void
_InstancerObserver::_Populate()
{
    UsdImaging_SceneIndexPrimView view(_prototypeSceneIndex, _prototypeRoot);
    for (auto it = view.begin(); it != view.end(); ++it) {
        const SdfPath &path = *it;
        HdSceneIndexPrim const prim = _prototypeSceneIndex->GetPrim(path);
        if (prim.primType == HdPrimTypeTokens->instancer) {
            _UpdateInstancer(path, prim);
            // Do not visit descendants: if the instancer has another instancer
            // as descendant, then we only want to pick it up if it is within
            // a prototype of this instancer.
            // The _InstancerObserver that _UpdateInstancer inserted will
            // do that.
            it.SkipDescendants();
        }
    }
}

void
_InstancerObserver::PrimsAdded(const HdSceneIndexBase &sender,
                    const AddedPrimEntries &entries)
{
    for (const AddedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;
        if (entry.primType == HdPrimTypeTokens->instancer) {
            _UpdateInstancer(path);
        } else {
            // If prim was re-synced and is no longer an instancer,
            // delete it from the necessary places.
            if (_subinstancerObservers.erase(path) > 0) {
                _context->instancerSceneIndex->RemovePrims(
                    { { _RerootedPath(path) } });
            }
        }
    }
}

void
_InstancerObserver::PrimsDirtied(const HdSceneIndexBase &sender,
                                 const DirtiedPrimEntries &entries)
{
    static const HdDataSourceLocator locator =
        HdInstancerTopologySchema::GetDefaultLocator().Append(
            HdInstancerTopologySchemaTokens->prototypes);

    for (const DirtiedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;
        if (!entry.dirtyLocators.Contains(locator)) {
            continue;
        }
        auto it = _subinstancerObservers.find(path);
        if (it == _subinstancerObservers.end()) {
            continue;
        }
        _UpdateInstancer(&it->second, path);
    }
}

void
_InstancerObserver::PrimsRemoved(const HdSceneIndexBase &sender,
                                 const RemovedPrimEntries &entries)
{
    HdSceneIndexObserver::RemovedPrimEntries removedInstancers;

    for (const RemovedPrimEntry &entry : entries) {
        const SdfPath &path = entry.primPath;
        // Find all instancers that are namespace descendants of
        // the removed prim to delete them.
        auto it = _subinstancerObservers.lower_bound(path);
        while (it != _subinstancerObservers.end() &&
               it->first.HasPrefix(path)) {
            removedInstancers.emplace_back(_RerootedPath(it->first));
            it = _subinstancerObservers.erase(it);
        }
    }

    if (!removedInstancers.empty()) {
        _context->instancerSceneIndex->RemovePrims(removedInstancers);
    }
}

}

using namespace UsdImagingPiPrototypePropagatingSceneIndex_Impl;

UsdImagingPiPrototypePropagatingSceneIndexRefPtr
UsdImagingPiPrototypePropagatingSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return TfCreateRefPtr(
        new UsdImagingPiPrototypePropagatingSceneIndex(
            inputSceneIndex));
}

UsdImagingPiPrototypePropagatingSceneIndex::
UsdImagingPiPrototypePropagatingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
  : _context(std::make_shared<_Context>(inputSceneIndex))
  , _mergingSceneIndexObserver(this)
  , _instancerObserver(std::make_unique<_InstancerObserver>(_context))
{
}

std::vector<HdSceneIndexBaseRefPtr>
UsdImagingPiPrototypePropagatingSceneIndex::GetInputScenes() const
{
    if (TfGetEnvSetting(USDIMAGING_SHOW_POINT_PROTOTYPE_SCENE_INDICES)) {
        return { _context->mergingSceneIndex->GetInputScenes() };
    } else {
        return { _context->inputSceneIndex };
    }
}

HdSceneIndexPrim
UsdImagingPiPrototypePropagatingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    return _context->mergingSceneIndex->GetPrim(primPath);
}

SdfPathVector
UsdImagingPiPrototypePropagatingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _context->mergingSceneIndex->GetChildPrimPaths(primPath);
}

UsdImagingPiPrototypePropagatingSceneIndex::
_MergingSceneIndexObserver::_MergingSceneIndexObserver(
    UsdImagingPiPrototypePropagatingSceneIndex * const owner)
  : _owner(owner)
{
    HdSceneIndexObserverPtr const self(this);
    _owner->_context->mergingSceneIndex->AddObserver(self);
}


void
UsdImagingPiPrototypePropagatingSceneIndex::
_MergingSceneIndexObserver::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    _owner->_SendPrimsAdded(entries);
}

void
UsdImagingPiPrototypePropagatingSceneIndex::
_MergingSceneIndexObserver::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    _owner->_SendPrimsDirtied(entries);
}

void
UsdImagingPiPrototypePropagatingSceneIndex::
_MergingSceneIndexObserver::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    _owner->_SendPrimsRemoved(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
