//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/niPrototypePropagatingSceneIndex.h"

#include "pxr/usdImaging/usdImaging/dataSourceRelocatingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/flattenedDataSourceProviders.h"
#include "pxr/usdImaging/usdImaging/niInstanceAggregationSceneIndex.h"
#include "pxr/usdImaging/usdImaging/niPrototypePruningSceneIndex.h"
#include "pxr/usdImaging/usdImaging/niPrototypeSceneIndex.h"
#include "pxr/usdImaging/usdImaging/rerootingSceneIndex.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/dataSourceHash.h"
#include "pxr/imaging/hd/flatteningSceneIndex.h"
#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/primvarsSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/skinningSettings.h"
#include "pxr/imaging/hd/retainedDataSource.h"

#include "pxr/usd/usdSkel/tokens.h"

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
        TRACE_FUNCTION();
        
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
        if (HdSkinningSettings::IsSkinningDeferred() && !forPrototype) {
            // Relocate skelBinding:animationSource to 
            // primvars:skel:animationSource so the skel instances
            // with different animationSource can be aggregated.
            // This also allows it to be aggregated to the instancer as an
            // instance primvar.
            //
            // We hardcode the skelBinding tokens here so we don't have to 
            // introduce dependency to usdSkelImaging.
            static const HdDataSourceLocator srcSkelAnimLocator(
                TfToken("skelBinding"), TfToken("animationSource"));
            // This instance primvar will later be picked up by 
            // UsdSkelImagingDataSourceXformResolver::GetInstanceAnimationSource()
            static const HdDataSourceLocator dstPrimvarLocator(
                HdPrimvarsSchema::GetSchemaToken(), 
                UsdSkelTokens->skelAnimationSource);

            return UsdImaging_NiInstanceAggregationSceneIndex::New(
                UsdImaging_DataSourceRelocatingSceneIndex::New(
                    prototypeSceneIndex,
                    srcSkelAnimLocator, dstPrimvarLocator,
                    /* forNativeInstance */ true),
                forPrototype, _instanceDataSourceNames);
        }
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

// An RAII-helper that batches operations to HdMergingSceneIndex.
class
UsdImagingNiPrototypePropagatingSceneIndex::_MergingSceneIndexOperations
{
public:
    _MergingSceneIndexOperations(
        HdMergingSceneIndexRefPtr const &mergingSceneIndex)
     : _mergingSceneIndex(mergingSceneIndex)
    { }

    ~_MergingSceneIndexOperations()
    {
        _mergingSceneIndex->RemoveInputScenes(_scenesToRemove);
        _mergingSceneIndex->InsertInputScenes(_scenesToInsert);
    }

    void AddInputScene(
        HdSceneIndexBaseRefPtr const &inputScene,
        const SdfPath &activeInputSceneRoot)
    {
        _scenesToInsert.push_back({inputScene, activeInputSceneRoot});
    }

    /// Do not call with a scene that was given to AddInputScene
    /// earlier - as the scene given to AddInputScene will always
    /// be added.
    void RemoveInputScene(
        HdSceneIndexBaseRefPtr const &inputScene)
    {
        _scenesToRemove.push_back(inputScene);
    }

private:
    HdMergingSceneIndexRefPtr const _mergingSceneIndex;

    std::vector<HdMergingSceneIndex::InputScene> _scenesToInsert;
    std::vector<HdSceneIndexBaseRefPtr> _scenesToRemove;
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

    if (!prototypeName.IsEmpty()) {
        SetDisplayName(
            TfStringPrintf(
                "Propagating native prototype %s",
                prototypeName.GetText()));
    }
}

UsdImagingNiPrototypePropagatingSceneIndex::
~UsdImagingNiPrototypePropagatingSceneIndex()
{
    TRACE_FUNCTION()

    // We need to release all references we have to the scene indices...

    // Note that we are not updating the merging scene index here since we are
    // deleting it anyway.
    _instancersToPropagatedPrototypeSceneIndex.clear();
    _instanceAggregationSceneIndex = nullptr;
    // Note that the Hydra Scene Debugger could potentially delay the
    // deletion of the merging scene index.
    {
        TRACE_SCOPE("Deleting merging scene index");

        _mergingSceneIndex = nullptr;
    }

    // ... before we can garbage collect.
    _cache->GarbageCollect(_prototypeName, _prototypeRootOverlayDsHash);
}


void
UsdImagingNiPrototypePropagatingSceneIndex::_Populate(
    HdSceneIndexBaseRefPtr const &instanceAggregationSceneIndex)
{
    TRACE_FUNCTION();

    _MergingSceneIndexOperations
        mergingSceneIndexOperations(_mergingSceneIndex);

    for (const SdfPath &primPath
             : HdSceneIndexPrimView(instanceAggregationSceneIndex,
                                    SdfPath::AbsoluteRootPath())) {
        _AddPrim(primPath, &mergingSceneIndexOperations);
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
UsdImagingNiPrototypePropagatingSceneIndex::_PrimsAdded(
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    _MergingSceneIndexOperations
        mergingSceneIndexOperations(_mergingSceneIndex);

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        _AddPrim(entry.primPath, &mergingSceneIndexOperations);
    }
}

void
UsdImagingNiPrototypePropagatingSceneIndex::_AddPrim(
    const SdfPath &primPath,
    _MergingSceneIndexOperations * const mergingSceneIndexOperations)
{
    const TfToken prototypeName =
        UsdImaging_NiInstanceAggregationSceneIndex::
        GetPrototypeNameFromInstancerPath(primPath);
    if (prototypeName.IsEmpty()) {
        return;
    }

    HdSceneIndexBaseRefPtr &propagatedPrototypeSceneIndex =
        _instancersToPropagatedPrototypeSceneIndex[primPath];

    // First erase previous entry.

    if (propagatedPrototypeSceneIndex) {
        mergingSceneIndexOperations->RemoveInputScene(
            propagatedPrototypeSceneIndex);
    }

    propagatedPrototypeSceneIndex =
        UsdImagingRerootingSceneIndex::New(
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
            UsdImaging_NiPrototypeSceneIndex::GetInstancerPath(),
            primPath);

    // Insert scene index for given instancer.
    mergingSceneIndexOperations->AddInputScene(
        propagatedPrototypeSceneIndex, primPath);
}

void
UsdImagingNiPrototypePropagatingSceneIndex::_PrimsRemoved(
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _MergingSceneIndexOperations
        mergingSceneIndexOperations(_mergingSceneIndex);

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        _RemovePrim(entry.primPath, &mergingSceneIndexOperations);
    }

}

void
UsdImagingNiPrototypePropagatingSceneIndex::_RemovePrim(
    const SdfPath &primPath,
    _MergingSceneIndexOperations * const mergingSceneIndexOperations)
{
    TRACE_FUNCTION();

    // Erase all entries from map with given prefix.

    auto it = _instancersToPropagatedPrototypeSceneIndex.lower_bound(primPath);
    while (it != _instancersToPropagatedPrototypeSceneIndex.end() &&
           it->first.HasPrefix(primPath)) {
        mergingSceneIndexOperations->RemoveInputScene(it->second);
        it = _instancersToPropagatedPrototypeSceneIndex.erase(it);
    }
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

    _owner->_PrimsAdded(entries);
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
    
    _owner->_PrimsRemoved(entries);
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
