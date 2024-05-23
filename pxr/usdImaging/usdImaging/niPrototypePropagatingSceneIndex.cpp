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
#include "pxr/usdImaging/usdImaging/niPrototypePropagatingSceneIndex.h"

#include "pxr/usdImaging/usdImaging/flattenedDataSourceProviders.h"
#include "pxr/usdImaging/usdImaging/niInstanceAggregationSceneIndex.h"
#include "pxr/usdImaging/usdImaging/niPrototypePruningSceneIndex.h"
#include "pxr/usdImaging/usdImaging/niPrototypeSceneIndex.h"
#include "pxr/usdImaging/usdImaging/rerootingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/dataSourceHash.h"
#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(USDIMAGING_SHOW_NATIVE_PROTOTYPE_SCENE_INDICES, false,
                      "If true, the native prototype propagating scene index "
                      "will list as input scene indices all intermediate scene "
                      "indices for all prototypes.");

/// Caches scene indices for each USD prototype.
///
/// Stores weak ptr's to the scene indices.
///
class UsdImagingNiPrototypePropagatingSceneIndex::_SceneIndexCache
{
public:
    _SceneIndexCache(HdSceneIndexBaseRefPtr const &inputSceneIndex,
                     const TfTokenVector &instanceDataSourceNames,
                     const SceneIndexAppendCallback &sceneIndexAppendCallback)
      : _inputSceneIndex(inputSceneIndex)
      , _instanceDataSourceNames(instanceDataSourceNames)
      , _sceneIndexAppendCallback(sceneIndexAppendCallback)
    {
    }

    /// Input scene Index from UsdImagingNiPrototypePropagatingSceneIndex
    /// (constructed for scene root).
    HdSceneIndexBaseRefPtr const &GetInputSceneIndex() const {
        return _inputSceneIndex;
    }

    struct SceneIndices {
        /// UsdImaging_NiPrototypeSceneIndex for given prototype.
        HdSceneIndexBaseRefPtr prototypeSceneIndex;
        /// UsdImaging_NiInstanceAggregationSceneIndex for given
        /// prototype.
        HdSceneIndexBaseRefPtr instanceAggregationSceneIndex;
    };

    // Get scene indices to propagate the USD prototype with
    // given name.
    //
    // We will also overlay the prototype root with the given
    // data source. We need the hash of the given data source
    // for caching the result by the pair (prototype name, hash).
    SceneIndices
    GetSceneIndicesForPrototype(
        const TfToken &prototypeName,
        const size_t prototypeRootOverlayDsHash,
        HdContainerDataSourceHandle const &prototypeRootOverlayDs)
    {
        TRACE_FUNCTION();

        SceneIndices result;

        _SceneIndices2 &sceneIndices2 =
            _prototypeToBindingHashToSceneIndices[prototypeName];
        _SceneIndices1 &sceneIndices1 =
            sceneIndices2.hashToSceneIndices[prototypeRootOverlayDsHash];

        // Check whether weak ptr references valid scene index.
        HdSceneIndexBaseRefPtr isolatingSceneIndex =
            sceneIndices2.isolatingSceneIndex;
        if (!isolatingSceneIndex) {
            // Allocate new scene index if not.
            isolatingSceneIndex =
                _ComputeIsolatingSceneIndex(
                    prototypeName);
            // Store weak ptr so that it can be re-used in the future.
            sceneIndices2.isolatingSceneIndex =
                isolatingSceneIndex;
        }

        // Are we instantiating, e.g., the instance aggregation scene index
        // to aggregate instances inside a prototype or for everything outside
        // any USD prototype?
        const bool forPrototype = !prototypeName.IsEmpty();

        result.prototypeSceneIndex = sceneIndices1.prototypeSceneIndex;
        if (!result.prototypeSceneIndex) {
            result.prototypeSceneIndex =
                _ComputePrototypeSceneIndex(
                    isolatingSceneIndex,
                    forPrototype,
                    prototypeRootOverlayDs);
            sceneIndices1.prototypeSceneIndex =
                result.prototypeSceneIndex;
        }

        result.instanceAggregationSceneIndex =
            sceneIndices1.instanceAggregationSceneIndex;
        if (!result.instanceAggregationSceneIndex) {
            result.instanceAggregationSceneIndex =
                _ComputeInstanceAggregationSceneIndex(
                    result.prototypeSceneIndex,
                    forPrototype);
            sceneIndices1.instanceAggregationSceneIndex =
                result.instanceAggregationSceneIndex;
        }

        return result;
    }

    void
    GarbageCollect(const TfToken &prototypeName,
                   const size_t prototypeRootOverlayDsHash)
    {
        auto it = _prototypeToBindingHashToSceneIndices.find(prototypeName);
        if (it == _prototypeToBindingHashToSceneIndices.end()) {
            return;
        }
        _GarbageCollect(&it->second.hashToSceneIndices, prototypeRootOverlayDsHash);
        if (!it->second.hashToSceneIndices.empty()) {
            return;
        }
        if (it->second.isolatingSceneIndex) {
            return;
        }

        _prototypeToBindingHashToSceneIndices.erase(it);
    }

private:
    // Scene indices that can only be created if we have both
    // the prototype name and the overlay data source.
    struct _SceneIndices1
    {
        HdSceneIndexBasePtr instanceAggregationSceneIndex;
        HdSceneIndexBasePtr prototypeSceneIndex;
    };

    // Scene indices that can be created if we have only the
    // prototype name.
    struct _SceneIndices2
    {
        HdSceneIndexBasePtr isolatingSceneIndex;
        std::map<size_t, _SceneIndices1> hashToSceneIndices;
    };

    HdSceneIndexBaseRefPtr
    _ComputeIsolatingSceneIndex(
        const TfToken &prototypeName) const
    {
        if (prototypeName.IsEmpty()) {
            return UsdImaging_NiPrototypePruningSceneIndex::New(
                _inputSceneIndex);
        } else {
            // Isolate prototype from UsdImagingStageSceneIndex and
            // move it under the instancer.
            return UsdImagingRerootingSceneIndex::New(
                _inputSceneIndex,
                // Path of prototype on UsdImagingStageSceneIndex
                SdfPath::AbsoluteRootPath().AppendChild(prototypeName),
                UsdImaging_NiPrototypeSceneIndex::GetPrototypePath());
        }
    }

    HdSceneIndexBaseRefPtr
    _ComputePrototypeSceneIndex(
        HdSceneIndexBaseRefPtr const &isolatingSceneIndex,
        const bool forPrototype,
        HdContainerDataSourceHandle const &prototypeRootOverlayDs)
    {
        HdSceneIndexBaseRefPtr sceneIndex = isolatingSceneIndex;

        sceneIndex = 
            UsdImaging_NiPrototypeSceneIndex::New(
                sceneIndex,
                forPrototype,
                prototypeRootOverlayDs);
        // We insert the flattening scene index at every recursion level of
        // native instancing.
        //
        // Thus, if we have a nested instance with opinions inside a prototype
        // instanced by a nested instance with opinions, we will flatten them
        // correctly.
        sceneIndex =
            HdFlatteningSceneIndex::New(
                sceneIndex,
                UsdImagingFlattenedDataSourceProviders());
        if (_sceneIndexAppendCallback) {
            // Typically adds the UsdImagingDrawModeSceneIndex.
            sceneIndex =
                _sceneIndexAppendCallback(sceneIndex);
        }

        return sceneIndex;
    }

    HdSceneIndexBaseRefPtr
    _ComputeInstanceAggregationSceneIndex(
        HdSceneIndexBaseRefPtr const &prototypeSceneIndex,
        const bool forPrototype)
    {
        return
            UsdImaging_NiInstanceAggregationSceneIndex::New(
                prototypeSceneIndex,
                forPrototype,
                _instanceDataSourceNames);
    }

    static
    void
    _GarbageCollect(std::map<size_t, _SceneIndices1> * hashToSceneIndices,
                    const size_t prototypeRootOverlayDsHash)
    {
        auto it = hashToSceneIndices->find(prototypeRootOverlayDsHash);
        if (it == hashToSceneIndices->end()) {
            return;
        }
        if (it->second.instanceAggregationSceneIndex) {
            return;
        }
        if (it->second.prototypeSceneIndex) {
            return;
        }

        hashToSceneIndices->erase(it);
    }

    HdSceneIndexBaseRefPtr const _inputSceneIndex;
    const TfTokenVector _instanceDataSourceNames;
    const SceneIndexAppendCallback _sceneIndexAppendCallback;

    // Nested map.
    std::map<TfToken, _SceneIndices2> _prototypeToBindingHashToSceneIndices;
};

// An RAII helper that inserts the given scene index followed by a
// re-rooting scene index into the given merging scene index upon
// constructions and removes it from the merging scene index on
// destruction.
class
UsdImagingNiPrototypePropagatingSceneIndex::_MergingSceneIndexEntry
{
public:
    _MergingSceneIndexEntry(
        const SdfPath &prefix,
        HdSceneIndexBaseRefPtr const &sceneIndex,
        HdMergingSceneIndexRefPtr const &mergingSceneIndex)
      : _rerootingSceneIndex(
          UsdImagingRerootingSceneIndex::New(
              sceneIndex,
              // Re-root, but only prims under the instancer,
              // i.e., the instancer and the prototype.
              // This way paths inside the prototype pointing to
              // stuff outside the prototype will not be changed.
              UsdImaging_NiPrototypeSceneIndex::GetInstancerPath(), prefix))
      , _mergingSceneIndex(mergingSceneIndex)
    {
        _mergingSceneIndex->AddInputScene(_rerootingSceneIndex, prefix);
    }

    ~_MergingSceneIndexEntry()
    {
        _mergingSceneIndex->RemoveInputScene(_rerootingSceneIndex);
    }

private:
    HdSceneIndexBaseRefPtr const _rerootingSceneIndex;
    HdMergingSceneIndexRefPtr const _mergingSceneIndex;
};

UsdImagingNiPrototypePropagatingSceneIndexRefPtr
UsdImagingNiPrototypePropagatingSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    const TfTokenVector &instanceDataSourceNames,
    const SceneIndexAppendCallback &sceneIndexAppendCallback)
{
    return _New(/* prototypeName = */ TfToken(),
                /* protoypeRootDs =*/ nullptr,
                std::make_shared<_SceneIndexCache>(
                    inputSceneIndex,
                    instanceDataSourceNames,
                    sceneIndexAppendCallback));
}

UsdImagingNiPrototypePropagatingSceneIndexRefPtr
UsdImagingNiPrototypePropagatingSceneIndex::_New(
    const TfToken &prototypeName,
    HdContainerDataSourceHandle const &prototypeRootOverlayDs,
    _SceneIndexCacheSharedPtr const &cache)
{
    return TfCreateRefPtr(
        new UsdImagingNiPrototypePropagatingSceneIndex(
            prototypeName, prototypeRootOverlayDs, cache));
}

UsdImagingNiPrototypePropagatingSceneIndex::
UsdImagingNiPrototypePropagatingSceneIndex(
        const TfToken &prototypeName,
        HdContainerDataSourceHandle const &prototypeRootOverlayDs,
        _SceneIndexCacheSharedPtr const &cache)
  : _prototypeName(prototypeName)
  , _prototypeRootOverlayDsHash(
      HdDataSourceHash(prototypeRootOverlayDs, 0.0f, 0.0f))
  , _cache(cache)
  , _mergingSceneIndex(HdMergingSceneIndex::New())
  , _instanceAggregationSceneIndexObserver(this)
  , _mergingSceneIndexObserver(this)
{
    TRACE_FUNCTION();

    const _SceneIndexCache::SceneIndices sceneIndices =
        _cache->GetSceneIndicesForPrototype(
            prototypeName, _prototypeRootOverlayDsHash, prototypeRootOverlayDs);

    _instanceAggregationSceneIndex = sceneIndices.instanceAggregationSceneIndex;

    _mergingSceneIndex->AddInputScene(
        sceneIndices.prototypeSceneIndex,
        SdfPath::AbsoluteRootPath());
    _mergingSceneIndex->AddInputScene(
        sceneIndices.instanceAggregationSceneIndex,
        SdfPath::AbsoluteRootPath());

    sceneIndices.instanceAggregationSceneIndex->AddObserver(
        HdSceneIndexObserverPtr(&_instanceAggregationSceneIndexObserver));
    _mergingSceneIndex->AddObserver(
        HdSceneIndexObserverPtr(&_mergingSceneIndexObserver));

    _Populate(sceneIndices.instanceAggregationSceneIndex);
}

UsdImagingNiPrototypePropagatingSceneIndex::
~UsdImagingNiPrototypePropagatingSceneIndex()
{
    // We need to release all references we have to the scene indices...
    _instancersToMergingSceneIndexEntry.clear();
    _instanceAggregationSceneIndex = nullptr;
    _mergingSceneIndex = nullptr;
    
    // ... before we can garbage collect.
    _cache->GarbageCollect(_prototypeName, _prototypeRootOverlayDsHash);
}
    

void
UsdImagingNiPrototypePropagatingSceneIndex::_Populate(
    HdSceneIndexBaseRefPtr const &instanceAggregationSceneIndex)
{
    TRACE_FUNCTION();

    for (const SdfPath &primPath
             : HdSceneIndexPrimView(instanceAggregationSceneIndex,
                                    SdfPath::AbsoluteRootPath())) {
        _AddPrim(primPath);
    }
}

static
HdContainerDataSourceHandle
_GetBindingScopeDataSource(HdSceneIndexBaseRefPtr const &sceneIndex,
                           const SdfPath &primPath)
{
    const SdfPath bindingScope =
        UsdImaging_NiInstanceAggregationSceneIndex::
        GetBindingScopeFromInstancerPath(primPath);

    return sceneIndex->GetPrim(bindingScope).dataSource;
}

void
UsdImagingNiPrototypePropagatingSceneIndex::_AddPrim(const SdfPath &primPath)
{
    const TfToken prototypeName =
        UsdImaging_NiInstanceAggregationSceneIndex::
        GetPrototypeNameFromInstancerPath(primPath);
    if (prototypeName.IsEmpty()) {
        return;
    }

    _MergingSceneIndexEntryUniquePtr &entry =
        _instancersToMergingSceneIndexEntry[primPath];

    // First erase previous entry.
    entry = nullptr;

    // Insert scene index for given instancer.
    entry = std::make_unique<_MergingSceneIndexEntry>(
        primPath,
        UsdImagingNiPrototypePropagatingSceneIndex::_New(
            prototypeName,
            // Apply the container data source from the binding scope
            // to the prototype root.
            // This data source contains opinions of the
            // aggregated native instances about, e.g., purpose.
            //
            // Note that the flattening scene index will
            // propagate these opinions to the descendants of
            // the prototype root without stronger opinion.
            //
            // The bool data source model:applyDrawMode in the container
            // data source has a special role. It will not be touched
            // by the flattening scene index. However, the draw mode
            // scene index will turn the prototype into a draw mode
            // standin if model:applyDrawMode is true and model:drawMode
            // is non-trivial. The draw mode scene index would be called
            // through the AppendSceneIndexCallback.

            /* prototypeRootOverlayDs = */
            _GetBindingScopeDataSource(
                _instanceAggregationSceneIndex, primPath),
            _cache),
        _mergingSceneIndex);
}

template<typename Container>
static
void
_ErasePrefix(Container * const c, const SdfPath &prefix)
{
    auto it = c->lower_bound(prefix);
    while (it != c->end() && it->first.HasPrefix(prefix)) {
        it = c->erase(it);
    }
}

void
UsdImagingNiPrototypePropagatingSceneIndex::_RemovePrim(const SdfPath &primPath)
{
    TRACE_FUNCTION();

    // Erase all entries from map with given prefix.
    _ErasePrefix(&_instancersToMergingSceneIndexEntry, primPath);
}

std::vector<HdSceneIndexBaseRefPtr>
UsdImagingNiPrototypePropagatingSceneIndex::GetInputScenes() const
{
    if (TfGetEnvSetting(USDIMAGING_SHOW_NATIVE_PROTOTYPE_SCENE_INDICES)) {
        return _mergingSceneIndex->GetInputScenes();
    } else {
        return { _cache->GetInputSceneIndex() };
    }
}

std::vector<HdSceneIndexBaseRefPtr>
UsdImagingNiPrototypePropagatingSceneIndex::GetEncapsulatedScenes() const
{
    return { _mergingSceneIndex };
}

HdSceneIndexPrim
UsdImagingNiPrototypePropagatingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    return _mergingSceneIndex->GetPrim(primPath);
}

SdfPathVector
UsdImagingNiPrototypePropagatingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    return _mergingSceneIndex->GetChildPrimPaths(primPath);
}

UsdImagingNiPrototypePropagatingSceneIndex::
_InstanceAggregationSceneIndexObserver::_InstanceAggregationSceneIndexObserver(
    UsdImagingNiPrototypePropagatingSceneIndex * const owner)
  : _owner(owner)
{
}

void
UsdImagingNiPrototypePropagatingSceneIndex::
_InstanceAggregationSceneIndexObserver::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const AddedPrimEntry &entry : entries) {
        _owner->_AddPrim(entry.primPath);
    }
}

void
UsdImagingNiPrototypePropagatingSceneIndex::
_InstanceAggregationSceneIndexObserver::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    // No need to handle this.
}

void
UsdImagingNiPrototypePropagatingSceneIndex::
_InstanceAggregationSceneIndexObserver::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const RemovedPrimEntry &entry : entries) {
        _owner->_RemovePrim(entry.primPath);
    }
}

void
UsdImagingNiPrototypePropagatingSceneIndex::
_InstanceAggregationSceneIndexObserver::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}

UsdImagingNiPrototypePropagatingSceneIndex::
_MergingSceneIndexObserver::_MergingSceneIndexObserver(
    UsdImagingNiPrototypePropagatingSceneIndex * const owner)
  : _owner(owner)
{
}

void
UsdImagingNiPrototypePropagatingSceneIndex::
_MergingSceneIndexObserver::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    _owner->_SendPrimsAdded(entries);
}

void
UsdImagingNiPrototypePropagatingSceneIndex::
_MergingSceneIndexObserver::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    _owner->_SendPrimsDirtied(entries);
}

void
UsdImagingNiPrototypePropagatingSceneIndex::
_MergingSceneIndexObserver::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    _owner->_SendPrimsRemoved(entries);
}

void
UsdImagingNiPrototypePropagatingSceneIndex::
_MergingSceneIndexObserver::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}

PXR_NAMESPACE_CLOSE_SCOPE
