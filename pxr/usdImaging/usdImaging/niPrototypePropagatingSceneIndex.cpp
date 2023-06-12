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

#include "pxr/usdImaging/usdImaging/niInstanceAggregationSceneIndex.h"
#include "pxr/usdImaging/usdImaging/niPrototypePruningSceneIndex.h"
#include "pxr/usdImaging/usdImaging/niPrototypeSceneIndex.h"
#include "pxr/usdImaging/usdImaging/rerootingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"

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
    _SceneIndexCache(HdSceneIndexBaseRefPtr const &inputSceneIndex)
      : _inputSceneIndex(inputSceneIndex)
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

    SceneIndices
    GetSceneIndicesForPrototype(const TfToken &prototypeName)
    {
        SceneIndices result;

        _SceneIndices &sceneIndices = _prototypeToSceneIndices[prototypeName];

        // Are we instantiating, e.g., the instance aggregation scene index
        // to aggregate instances inside a prototype or for everything outside
        // any USD prototype?
        const bool forPrototype = !prototypeName.IsEmpty();

        // Check whether weak ptr references valid scene index.
        HdSceneIndexBaseRefPtr isolatingSceneIndex =
            sceneIndices.isolatingSceneIndex;
        if (!isolatingSceneIndex) {
            // Allocate new scene index if not.
            isolatingSceneIndex =
                _ComputeIsolatingSceneIndex(
                    prototypeName);
            // Store weak ptr so that it can be re-used in the future.
            sceneIndices.isolatingSceneIndex =
                isolatingSceneIndex;
        }

        result.prototypeSceneIndex = sceneIndices.prototypeSceneIndex;
        if (!result.prototypeSceneIndex) {
            result.prototypeSceneIndex =
                UsdImaging_NiPrototypeSceneIndex::New(
                    isolatingSceneIndex, forPrototype);
            sceneIndices.prototypeSceneIndex =
                result.prototypeSceneIndex;
        }

        result.instanceAggregationSceneIndex =
            sceneIndices.instanceAggregationSceneIndex;
        if (!result.instanceAggregationSceneIndex) {
            result.instanceAggregationSceneIndex =
                UsdImaging_NiInstanceAggregationSceneIndex::New(
                    isolatingSceneIndex, forPrototype);
            sceneIndices.instanceAggregationSceneIndex =
                result.instanceAggregationSceneIndex;
        }

        return result;
    }

private:
    HdSceneIndexBaseRefPtr
    _ComputeIsolatingSceneIndex(const TfToken &prototypeName) const
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

    HdSceneIndexBaseRefPtr const _inputSceneIndex;

    struct _SceneIndices
    {
        HdSceneIndexBasePtr isolatingSceneIndex;
        HdSceneIndexBasePtr instanceAggregationSceneIndex;
        HdSceneIndexBasePtr prototypeSceneIndex;
    };

    // We don't clear this map - but the keys will always be
    // __Prototype_1, __Prototype_2,
    // and there won't be too many of those.
    std::map<TfToken, _SceneIndices> _prototypeToSceneIndices;
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
    HdSceneIndexBaseRefPtr const &inputSceneIndex)
{
    return _New(/* prototypeName = */ TfToken(),
                std::make_shared<_SceneIndexCache>(inputSceneIndex));
}

UsdImagingNiPrototypePropagatingSceneIndexRefPtr
UsdImagingNiPrototypePropagatingSceneIndex::_New(
    const TfToken &prototypeName,
    _SceneIndexCacheSharedPtr const &cache)
{
    return TfCreateRefPtr(
        new UsdImagingNiPrototypePropagatingSceneIndex(prototypeName, cache));
}

UsdImagingNiPrototypePropagatingSceneIndex::
UsdImagingNiPrototypePropagatingSceneIndex(
        const TfToken &prototypeName,
        _SceneIndexCacheSharedPtr const &cache)
  : _prototypeName(prototypeName)
  , _cache(cache)
  , _mergingSceneIndex(HdMergingSceneIndex::New())
  , _instanceAggregationSceneIndexObserver(this)
  , _mergingSceneIndexObserver(this)
{
    const _SceneIndexCache::SceneIndices sceneIndices =
        _cache->GetSceneIndicesForPrototype(prototypeName);

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

void
UsdImagingNiPrototypePropagatingSceneIndex::_Populate(
    HdSceneIndexBaseRefPtr const &instanceAggregationSceneIndex)
{
    for (const SdfPath &primPath
             : HdSceneIndexPrimView(instanceAggregationSceneIndex,
                                    SdfPath::AbsoluteRootPath())) {
        _AddPrim(primPath);
    }
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
        UsdImagingNiPrototypePropagatingSceneIndex::_New(prototypeName, _cache),
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

HdSceneIndexPrim
UsdImagingNiPrototypePropagatingSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    return _mergingSceneIndex->GetPrim(primPath);
}

SdfPathVector
UsdImagingNiPrototypePropagatingSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
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
