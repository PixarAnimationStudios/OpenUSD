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

        HdSceneIndexBaseRefPtr isolatingSceneIndex =
            sceneIndices.isolatingSceneIndex;
        if (!isolatingSceneIndex) {
            isolatingSceneIndex =
                _ComputeIsolatingSceneIndex(
                    prototypeName);
        }

        result.prototypeSceneIndex = sceneIndices.prototypeSceneIndex;
        if (!result.prototypeSceneIndex) {
            result.prototypeSceneIndex =
                _ComputePrototypeSceneIndex(
                    prototypeName, isolatingSceneIndex);
            sceneIndices.prototypeSceneIndex = result.prototypeSceneIndex;
        }

        result.instanceAggregationSceneIndex =
            sceneIndices.instanceAggregationSceneIndex;
        if (!result.instanceAggregationSceneIndex) {
            result.instanceAggregationSceneIndex =
                _ComputeInstanceAggregationSceneIndex(
                    prototypeName, isolatingSceneIndex);
            sceneIndices.instanceAggregationSceneIndex = result.instanceAggregationSceneIndex;
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
            const SdfPath prototypePath =
                SdfPath::AbsoluteRootPath().AppendChild(prototypeName);
            return UsdImagingRerootingSceneIndex::New(
                _inputSceneIndex, prototypePath, prototypePath);
        }
    }

    HdSceneIndexBaseRefPtr
    _ComputePrototypeSceneIndex(
        const TfToken &prototypeName,
        HdSceneIndexBaseRefPtr const &commonSceneIndex) const
    {
        const SdfPath prototypeRoot =
            prototypeName.IsEmpty()
            ? SdfPath()
            : SdfPath::AbsoluteRootPath().AppendChild(prototypeName);
        
        return UsdImaging_NiPrototypeSceneIndex::New(
            commonSceneIndex, prototypeRoot);
    }

    HdSceneIndexBaseRefPtr
    _ComputeInstanceAggregationSceneIndex(
        const TfToken &prototypeName,
        HdSceneIndexBaseRefPtr const &commonSceneIndex) const
    {
        const SdfPath prototypeRoot =
            prototypeName.IsEmpty()
            ? SdfPath()
            : SdfPath::AbsoluteRootPath().AppendChild(prototypeName);

        return UsdImaging_NiInstanceAggregationSceneIndex::New(
            commonSceneIndex, prototypeRoot);
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
              sceneIndex, SdfPath::AbsoluteRootPath(), prefix))
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
    // Use the convention from the UsdImaging_NiInstanceAggregationSceneIndex
    // that instancers will have paths such as
    // /Foo/__Usd_Prototypes/Binding435..f52/__Prototype_1
    // find them.
    if (primPath.GetPathElementCount() < 3) {
        return;
    }

    // Path has __Usd_Prototypes at correct place.
    const TfToken parentParentName =
        primPath.GetParentPath().GetParentPath().GetNameToken();
    if (parentParentName != UsdImagingTokens->propagatedPrototypesScope) {
        return;
    }

    const TfToken prototypeName = primPath.GetNameToken();

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

PXR_NAMESPACE_CLOSE_SCOPE
