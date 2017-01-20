//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hd/renderIndex.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/glslfxShader.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/renderDelegateRegistry.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/surfaceShader.h"
#include "pxr/imaging/hd/texture.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/glslfx.h"

#include "pxr/base/work/arenaDispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/tf/pyLock.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

#include <iostream>
#include <mutex>
#include <unordered_set>

typedef boost::shared_ptr<class GlfGLSLFX> GlfGLSLFXSharedPtr;

HdRenderIndex::HdRenderIndex()
    : _delegateRprimMap()
    , _rprimMap()
    , _rprimIDSet()
    , _rprimPrimIdMap()
    , _shaderMap()
    , _taskMap()
    , _sprimTypeMap()
    , _bprimTypeMap()
    , _tracker()
    , _nextPrimId(1)
    , _instancerMap()
    , _surfaceFallback()
    , _syncQueue()
    , _renderDelegate(nullptr)
    , _ownsDelegateXXX(false)
{
    // Creating the fallback shader
    ReloadFallbackShader();

    // Register well-known collection types (to be deprecated)
    // XXX: for compatibility and smooth transition,
    //      leave geometry collection for a while.
    _tracker.AddCollection(HdTokens->geometry);

}

HdRenderIndex::~HdRenderIndex()
{
    HD_TRACE_FUNCTION();
    Clear();

    {
        // XXX: Transitional Code - to be removed.
        // If the render index created the render delegate, it needs
        // to release it.
        if (_ownsDelegateXXX)
        {
            HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();

            renderRegistry.ReleaseDelegate(_renderDelegate);
            _renderDelegate = nullptr;
        }
    }

}

void
HdRenderIndex::InsertRprim(TfToken const& typeId,
                 HdSceneDelegate* sceneDelegate,
                 SdfPath const& rprimId,
                 SdfPath const& instancerId /*= SdfPath()*/)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

#if 0
    // TODO: enable this after patching.
    if (!id.IsAbsolutePath()) {
        TF_CODING_ERROR("All Rprim IDs must be absolute paths <%s>\n",
                id.GetText());
        return;
    }
#endif

    if (ARCH_UNLIKELY(TfMapLookupPtr(_rprimMap, rprimId)))
        return;

    {
        // XXX: Transitional code.
        // If the application didn't use the new context api to
        // create a render delegate, create one on its behalf.
        //
        // In the future, this would be an error condition as the context api
        // would create the delegate and provide it to the render index..
        if (_renderDelegate == nullptr)
        {
            HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();
            const TfToken &defaultRenderDelegateId =
                                          renderRegistry.GetDefaultDelegateId();
             _renderDelegate =
                      renderRegistry.GetRenderDelegate(defaultRenderDelegateId);
             _ownsDelegateXXX = true;
        }
    }

    HdRprim *rprim = _renderDelegate->CreateRprim(typeId,
                                                  rprimId,
                                                  instancerId);
    if (rprim == nullptr) {
        return;
    }

    SdfPath const &sceneDelegateId = sceneDelegate->GetDelegateID();

    _rprimIDSet.insert(rprimId);
    _tracker.RprimInserted(rprimId, rprim->GetInitialDirtyBitsMask());
    _AllocatePrimId(rprim);

    SdfPathVector* vec = &_delegateRprimMap[sceneDelegateId];
    vec->push_back(rprimId);

    _RprimInfo info = {
      sceneDelegate,
      vec->size()-1,
      rprim
    };
    _rprimMap[rprimId] = std::move(info);

    SdfPath instanceId = rprim->GetInstancerId();

    if (!instanceId.IsEmpty()) {
        _tracker.InstancerRPrimInserted(instanceId, rprimId);
    }
}

void
HdRenderIndex::RemoveRprim(SdfPath const& id)
{
    HD_TRACE_FUNCTION();


    _RprimMap::iterator rit = _rprimMap.find(id);
    if (rit == _rprimMap.end())
        return;

    _RprimInfo &rprimInfo = rit->second;

    SdfPath instancerId = rprimInfo.rprim->GetInstancerId();

    SdfPath const &sceneDelegateId = rprimInfo.sceneDelegate->GetDelegateID();

    _DelegateRprimMap::iterator dit = _delegateRprimMap.find(sceneDelegateId);
    SdfPathVector* vec = &dit->second;
    size_t rem = rprimInfo.childIndex;

    TF_VERIFY(rem < vec->size());
    TF_VERIFY(vec->size() > 0);

    if (rem != vec->size()-1) {
        // Swap the item to remove to the back.
        std::swap(vec->at(rem), vec->back());
        // Update the index of the child we swapped.
        _RprimMap::iterator nit = _rprimMap.find(vec->at(rem));
        if (TF_VERIFY(nit != _rprimMap.end(),
                    "%s\n", vec->at(rem).GetText()))
            nit->second.childIndex = rem;
    }

    // The rprim to remove is now the last element in the path vector.
    // Remove the dead rprim.
    vec->pop_back();

    // If that was the last rprim for the delegate, clear out the delegate map
    // entry as well.
    if (vec->empty()) {
        _delegateRprimMap.erase(dit);
    }

    if (!instancerId.IsEmpty()) {
        _tracker.InstancerRPrimRemoved(instancerId, id);
    }

    _tracker.RprimRemoved(id);

    // Ask delegate to actually delete the rprim
    _renderDelegate->DestroyRprim(rprimInfo.rprim);
    rprimInfo.rprim = nullptr;

    _rprimIDSet.erase(id);
    _rprimMap.erase(rit);
}

void
HdRenderIndex::Clear()
{
    HD_TRACE_FUNCTION();
    TF_FOR_ALL(it, _rprimMap) {
        _tracker.RprimRemoved(it->first);
        _RprimInfo &rprimInfo = it->second;
        _renderDelegate->DestroyRprim(rprimInfo.rprim);
        rprimInfo.rprim = nullptr;
    }
    // Clear Rprims, Rprim IDs, and delegate mappings.
    _rprimIDSet.clear();
    _rprimMap.clear();
    _delegateRprimMap.clear();

    // Clear Sprims
    TF_FOR_ALL(typeIt, _sprimTypeMap) {
        TF_FOR_ALL(primIt, typeIt->second.sprimMap) {
            _tracker.SprimRemoved(primIt->first);
            _SprimInfo &sprimInfo = primIt->second;
            _renderDelegate->DestroySprim(sprimInfo.sprim);
            sprimInfo.sprim = nullptr;
        }
    }
    _sprimTypeMap.clear();

    // Clear Bprims
    TF_FOR_ALL(typeIt, _bprimTypeMap) {
        TF_FOR_ALL(primIt, typeIt->second.bprimMap) {
            _tracker.BprimRemoved(primIt->first);
            _BprimInfo &bprimInfo = primIt->second;
            _renderDelegate->DestroyBprim(bprimInfo.bprim);
            bprimInfo.bprim = nullptr;
        }
    }
    _bprimTypeMap.clear();


    // Clear instancers.
    TF_FOR_ALL(it, _instancerMap) {
        _tracker.InstancerRemoved(it->first);
    }
    _instancerMap.clear();

    // Clear shaders.
    TF_FOR_ALL(it, _shaderMap) {
        _tracker.ShaderRemoved(it->first);
    }
    _shaderMap.clear();

    // Clear tasks.
    TF_FOR_ALL(it, _taskMap) {
        _tracker.TaskRemoved(it->first);
    }
    _taskMap.clear();

    // After clearing the index, all collections must be invalidated to force
    // any dependent state to be updated.
    _tracker.MarkAllCollectionsDirty();
}

// -------------------------------------------------------------------------- //
/// \name Rprim Support
// -------------------------------------------------------------------------- //

void
HdRenderIndex::GetDelegateIDsWithDirtyRprims(int dirtyMask, 
                                             SdfPathVector* IDs) const
{
    IDs->reserve(_delegateRprimMap.size());
    HdChangeTracker const& tracker = GetChangeTracker();

    TF_FOR_ALL(delegateIt, _delegateRprimMap) {
        SdfPathVector const& rprimChildren = delegateIt->second;
        TF_FOR_ALL(rprimIt, rprimChildren) {
            if (dirtyMask == 0 || tracker.GetRprimDirtyBits(*rprimIt) & dirtyMask) {
                SdfPath const& delegateID = delegateIt->first;
                IDs->push_back(delegateID);
                break;
            }
        }
    }
}

SdfPathVector const& 
HdRenderIndex::GetDelegateRprimIDs(SdfPath const& delegateID) const
{
    static SdfPathVector const EMPTY;
    _DelegateRprimMap::const_iterator it = _delegateRprimMap.find(delegateID);
    if (it == _delegateRprimMap.end())
        return EMPTY;

    return it->second;
}

// -------------------------------------------------------------------------- //
/// \name Shader Support
// -------------------------------------------------------------------------- //

void 
HdRenderIndex::_TrackDelegateShader(HdSceneDelegate* sceneDelegate,
                                    SdfPath const& shaderId,
                                    HdSurfaceShaderSharedPtr const& shader)
{
    if (shaderId == SdfPath())
        return;
    _tracker.ShaderInserted(shaderId);

    _ShaderInfo shaderInfo;
    shaderInfo.sceneDelegate = sceneDelegate;
    shaderInfo.shader        = shader;

    _shaderMap.insert(std::make_pair(shaderId, shaderInfo));
}

HdSurfaceShaderSharedPtr const& 
HdRenderIndex::GetShader(SdfPath const& id) const {
    if (id == SdfPath())
        return _surfaceFallback;

    _ShaderMap::const_iterator it = _shaderMap.find(id);
    if (it != _shaderMap.end())
        return it->second.shader;

    return _surfaceFallback;
}

void
HdRenderIndex::RemoveShader(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _ShaderMap::iterator it = _shaderMap.find(id);
    if (it == _shaderMap.end())
        return;

    _tracker.ShaderRemoved(id);
    _shaderMap.erase(it);
}

void
HdRenderIndex::ReloadFallbackShader()
{
    GlfGLSLFXSharedPtr glslfx = GlfGLSLFXSharedPtr(new GlfGLSLFX(
                                             HdPackageFallbackSurfaceShader()));

    _surfaceFallback = HdSurfaceShaderSharedPtr(new HdGLSLFXShader(glslfx));
}


// -------------------------------------------------------------------------- //
/// \name Task Support
// -------------------------------------------------------------------------- //

void 
HdRenderIndex::_TrackDelegateTask(HdSceneDelegate* delegate, 
                                    SdfPath const& taskId,
                                    HdTaskSharedPtr const& task)
{
    if (taskId == SdfPath())
        return;
    _tracker.TaskInserted(taskId);
    _taskMap.insert(std::make_pair(taskId, task));
}

HdTaskSharedPtr const& 
HdRenderIndex::GetTask(SdfPath const& id) const {
    _TaskMap::const_iterator it = _taskMap.find(id);
    if (it != _taskMap.end())
        return it->second;
    static HdTaskSharedPtr EMPTY;
    return EMPTY;
}

void
HdRenderIndex::RemoveTask(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _TaskMap::iterator it = _taskMap.find(id);
    if (it == _taskMap.end())
        return;

    _tracker.TaskRemoved(id);
    _taskMap.erase(it);
}

// -------------------------------------------------------------------------- //
/// \name Sprim Support (scene state prim: light, camera...)
// -------------------------------------------------------------------------- //

void
HdRenderIndex::InsertSprim(TfToken const& typeId,
                           HdSceneDelegate* sceneDelegate,
                           SdfPath const& sprimId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (sprimId.IsEmpty()) {
        return;
    }

    {
        // XXX: Transitional code.
        // If the application didn't use the new context api to
        // create a render delegate, create one on its behalf.
        //
        // In the future, this would be an error condition as the context api
        // would create the delegate and provide it to the render index..
        if (_renderDelegate == nullptr)
        {
            HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();
            const TfToken &defaultRenderDelegateId =
                                          renderRegistry.GetDefaultDelegateId();
             _renderDelegate =
                      renderRegistry.GetRenderDelegate(defaultRenderDelegateId);
             _ownsDelegateXXX = true;
        }
    }

    HdSprim *sprim = _renderDelegate->CreateSprim(typeId,
                                                  sprimId);
    if (sprim == nullptr) {
        return;
    }


    int initialDirtyState = sprim->GetInitialDirtyBitsMask();

    _tracker.SprimInserted(sprimId, initialDirtyState);

    _SprimTypeIndex &typeIndex = _sprimTypeMap[typeId];

    typeIndex.sprimIDSet.insert(sprimId);

    _SprimInfo sprimInfo;
    sprimInfo.sceneDelegate = sceneDelegate;
    sprimInfo.sprim         = sprim;

    typeIndex.sprimMap.insert(std::make_pair(sprimId, sprimInfo));
}

void
HdRenderIndex::RemoveSprim(TfToken const& typeId, SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _SprimTypeMap::iterator typeIt = _sprimTypeMap.find(typeId);
    if (typeIt == _sprimTypeMap.end()) {
        TF_CODING_ERROR("Unknown type %s", typeId.GetText());
        return;
    }

    _SprimTypeIndex &typeIndex = typeIt->second;
    _SprimMap::iterator it = typeIndex.sprimMap.find(id);
    if (it == typeIndex.sprimMap.end()) {
        return;
    }

    _tracker.SprimRemoved(id);
    _SprimInfo &sprimInfo = it->second;

    // Ask delegate to actually delete the sprim
    _renderDelegate->DestroySprim(sprimInfo.sprim);
    sprimInfo.sprim = nullptr;

    typeIndex.sprimMap.erase(it);
    typeIndex.sprimIDSet.erase(id);
}

HdSprim const*
HdRenderIndex::GetSprim(TfToken const& typeId, SdfPath const& id) const
{
    _SprimTypeMap::const_iterator typeIt = _sprimTypeMap.find(typeId);
    if (typeIt == _sprimTypeMap.end()) {
        TF_CODING_ERROR("Unknown type %s", typeId.GetText());
        return nullptr;
    }

    const _SprimTypeIndex &typeIndex = typeIt->second;

    _SprimMap::const_iterator it = typeIndex.sprimMap.find(id);
    if (it != typeIndex.sprimMap.end()) {
        return it->second.sprim;
    }

    return nullptr;
}

SdfPathVector
HdRenderIndex::GetSprimSubtree(TfToken const& typeId,
                               SdfPath const& rootPath) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _SprimTypeMap::const_iterator typeIt = _sprimTypeMap.find(typeId);
    if (typeIt == _sprimTypeMap.end()) {
        // Maybe requested for a type that never got added
        return SdfPathVector();
    }

    const _SprimTypeIndex &typeIndex = typeIt->second;

    // Over-allocate paths, assuming worse-case all paths are going to be
    // returned.
    SdfPathVector paths;
    paths.reserve(typeIndex.sprimIDSet.size());

    _SprimIDSet::const_iterator pathIt =
                                     typeIndex.sprimIDSet.lower_bound(rootPath);
     while ((pathIt != typeIndex.sprimIDSet.end()) &&
            (pathIt->HasPrefix(rootPath))) {
         paths.push_back(*pathIt);
         ++pathIt;
    }

    return paths;
}

// -------------------------------------------------------------------------- //
/// \name Bprim Support (Buffer prim: texture, buffers...)
// -------------------------------------------------------------------------- //

void
HdRenderIndex::InsertBprim(TfToken const& typeId,
                           HdSceneDelegate* sceneDelegate,
                           SdfPath const& bprimId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (bprimId.IsEmpty()) {
        return;
    }

    {
        // XXX: Transitional code.
        // If the application didn't use the new context api to
        // create a render delegate, create one on its behalf.
        //
        // In the future, this would be an error condition as the context api
        // would create the delegate and provide it to the render index..
        if (_renderDelegate == nullptr)
        {
            HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();
            const TfToken &defaultRenderDelegateId =
                                          renderRegistry.GetDefaultDelegateId();
             _renderDelegate =
                      renderRegistry.GetRenderDelegate(defaultRenderDelegateId);
             _ownsDelegateXXX = true;
        }
    }

    HdBprim *bprim = _renderDelegate->CreateBprim(typeId,
                                                  bprimId);
    if (bprim == nullptr) {
        return;
    }


    int initialDirtyState = bprim->GetInitialDirtyBitsMask();

    _tracker.BprimInserted(bprimId, initialDirtyState);

    _BprimTypeIndex &typeIndex = _bprimTypeMap[typeId];

    typeIndex.bprimIDSet.insert(bprimId);

    _BprimInfo bprimInfo;
    bprimInfo.sceneDelegate = sceneDelegate;
    bprimInfo.bprim         = bprim;

    typeIndex.bprimMap.insert(std::make_pair(bprimId, bprimInfo));
}

void
HdRenderIndex::RemoveBprim(TfToken const& typeId, SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _BprimTypeMap::iterator typeIt = _bprimTypeMap.find(typeId);
    if (typeIt == _bprimTypeMap.end()) {
        TF_CODING_ERROR("Unknown type %s", typeId.GetText());
        return;
    }

    _BprimTypeIndex &typeIndex = typeIt->second;
    _BprimMap::iterator it = typeIndex.bprimMap.find(id);
    if (it == typeIndex.bprimMap.end()) {
        return;
    }

    _BprimInfo &bprimInfo = it->second;

    _tracker.BprimRemoved(id);

    // Ask delegate to actually delete the bprim
    _renderDelegate->DestroyBprim(bprimInfo.bprim);
    bprimInfo.bprim = nullptr;

    typeIndex.bprimMap.erase(it);
    typeIndex.bprimIDSet.erase(id);
}

HdBprim const*
HdRenderIndex::GetBprim(TfToken const& typeId, SdfPath const& id) const
{
    _BprimTypeMap::const_iterator typeIt = _bprimTypeMap.find(typeId);
    if (typeIt == _bprimTypeMap.end()) {
        TF_CODING_ERROR("Unknown type %s", typeId.GetText());
        return nullptr;
    }

    const _BprimTypeIndex &typeIndex = typeIt->second;

    _BprimMap::const_iterator it = typeIndex.bprimMap.find(id);
    if (it != typeIndex.bprimMap.end()) {
        return it->second.bprim;
    }

    return nullptr;
}

SdfPathVector
HdRenderIndex::GetBprimSubtree(TfToken const& typeId,
                               SdfPath const& rootPath) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _BprimTypeMap::const_iterator typeIt = _bprimTypeMap.find(typeId);
    if (typeIt == _bprimTypeMap.end()) {
        // Maybe requested for a type that never got added
        return SdfPathVector();
    }

    const _BprimTypeIndex &typeIndex = typeIt->second;

    // Over-allocate paths, assuming worse-case all paths are going to be
    // returned.
    SdfPathVector paths;
    paths.reserve(typeIndex.bprimIDSet.size());

    _BprimIDSet::const_iterator pathIt =
                                     typeIndex.bprimIDSet.lower_bound(rootPath);
     while ((pathIt != typeIndex.bprimIDSet.end()) &&
            (pathIt->HasPrefix(rootPath))) {
         paths.push_back(*pathIt);
         ++pathIt;
     }

      return paths;
}


void
HdRenderIndex::SetRenderDelegate(HdRenderDelegate *renderDelegate)
{
    if (_renderDelegate != nullptr && _renderDelegate != renderDelegate) {
        TF_CODING_ERROR("Render Delegate already set on render index and "
                        "switching render delegates is not supported");
        return;
    }

    _renderDelegate = renderDelegate;
    {
        // Small hack until applications migrate to using new
        // context API.
        // XXX: To be removed

        HdRenderDelegateRegistry &renderRegistry =
                                        HdRenderDelegateRegistry::GetInstance();

        renderRegistry.AddDelegateReference(_renderDelegate);
    }

}

TfToken
HdRenderIndex::GetRenderDelegateType() const
{
    if (_renderDelegate == nullptr) {
        return TfToken();
    }

    TfType const &type = TfType::Find(_renderDelegate);

    const std::string &typeName  = type.GetTypeName();
    return TfToken(typeName);
}

// -------------------------------------------------------------------------- //
/// \name Draw Item Handling 
// -------------------------------------------------------------------------- //

static
void
_AppendDrawItem(HdSceneDelegate* delegate,
                HdRprim* rprim,
                TfToken const& collectionName,
                TfToken const& reprName,
                bool forcedRepr,
                tbb::concurrent_vector<HdDrawItem const*>* result)
{
    if (rprim->IsInCollection(delegate, collectionName)) {
        std::vector<HdDrawItem> *drawItems =
                rprim->GetDrawItems(delegate, reprName, forcedRepr);

        TF_FOR_ALL(drawItemIt, *drawItems) {
            result->push_back(&(*drawItemIt));
        }
    }
}

// XXX: Remove
HdRenderIndex::HdDrawItemView
HdRenderIndex::GetDrawItems(HdRprimCollection const& collection)
{
    HD_TRACE_FUNCTION();

    // 
    // PERFORMANCE: Warning, this function is performance sensitive.
    //

    // Note that root paths are always sorted and will never be an empty vector.
    SdfPathVector const& rootPaths = collection.GetRootPaths();
    SdfPathVector const& excludePaths = collection.GetExcludePaths();
    TfToken const& collectionName = collection.GetName();
    TfToken const& reprName = collection.GetReprName();
    bool forcedRepr = collection.IsForcedRepr();

    // We could hold this vector persistently to avoid
    // reallocating and copying on every request.
    tbb::concurrent_vector<HdDrawItem const*> result;
    result.reserve(_rprimMap.size());

    // Here we are going to leverage a common pattern: often a delegate will be
    // created and its root will be used to filter the items being drawn, as a
    // result we can lookup each root in the delegate map and if there is a
    // match, we can process all the rprims that belong to that delegate without
    // having to filter each path individually.
    //
    // Any paths that are not found in the delegate map will be added to the
    // remaining root paths vector to be processed and filtered against each
    // individual Rprim.
    SdfPathVector remainingRootPaths;

    WorkArenaDispatcher dispatcher;

    for (SdfPathVector::const_iterator rootIt = rootPaths.begin();
            rootIt != rootPaths.end(); rootIt++)
    {
        SdfPath const& rootPath = *rootIt;
        SdfPathVector const* children = 
                                TfMapLookupPtr(_delegateRprimMap, rootPath);

        if (!children) {
            remainingRootPaths.push_back(rootPath);
            continue;
        }

        dispatcher.Run(
          [children, collectionName, reprName, forcedRepr, &result, this](){
            // In the loop below, we process the previous item while fetching
            // the next, this is done to hide the memory latency of accessing
            // each item.
            //
            // As a result, we fetch the first item and process the last item
            // outside the loop. 
            SdfPathVector::const_iterator pathIt = children->begin();
            _RprimInfo const* info = TfMapLookupPtr(_rprimMap, *pathIt);
            pathIt++;


            // Main loop.
            for (; pathIt != children->end(); pathIt++) {
                // Grab the current item.
                HdRprim*         rprim         = info->rprim;
                HdSceneDelegate *sceneDelegate = info->sceneDelegate;

                // Prefetch the next item.
                info = TfMapLookupPtr(_rprimMap, *pathIt);

                // Process the current item.
                _AppendDrawItem(sceneDelegate, rprim, collectionName, reprName,
                                forcedRepr, &result);
            }

            // Process the last item.
            _AppendDrawItem(info->sceneDelegate, info->rprim, collectionName,
                            reprName, forcedRepr, &result);
        });
    }

    
    dispatcher.Wait();

    // If we had all delegate roots in the rootPaths vector, we're done.
    if (remainingRootPaths.empty()) {
        HdRenderIndex::HdDrawItemView finalResult;
        finalResult.reserve(result.size());
        finalResult.insert(finalResult.begin(), result.begin(), result.end());
        return finalResult;
    }

    // 
    // Now do the slower filter of all the remaining root paths.
    //
    SdfPathVector::const_iterator rootIt = remainingRootPaths.begin();
    _RprimIDSet::const_iterator pathIt = _rprimIDSet.lower_bound(*rootIt);
     
    while (pathIt != _rprimIDSet.end()) {
        // Precondition: rootIt is either a prefix of the current prim or we've
        // passed that prefix in the iteration.
        if (!pathIt->HasPrefix(*rootIt)) {
            // continue to next root prefix
            rootIt++;
            if (rootIt == remainingRootPaths.end())
                break;

            // PERFORMANCE: could iterate here instead of calling lower_bound.
            pathIt = _rprimIDSet.lower_bound(*rootIt);
            continue;
        } else {
            // Here we know the path is potentially renderable
            // now we need to check if the path is excluded
            // XXX : We will need something more efficient
            //       specially if the list of excluded paths is big.
            //       Also, since the array of excluded paths is sorted
            //       we could for instance use lower_bound
            bool isExcludedPath = false;
            for (SdfPathVector::const_iterator excludeIt = excludePaths.begin();
                    excludeIt != excludePaths.end(); excludeIt++) {

                SdfPath const& excludePath = *excludeIt;
                if (pathIt->HasPrefix(excludePath)) { 
                    isExcludedPath = true;
                    break;
                }
            }

            if (isExcludedPath) {
                pathIt++;
                continue;
            }
        }

        _RprimInfo const* info = TfMapLookupPtr(_rprimMap, *pathIt);
        _AppendDrawItem(info->sceneDelegate, info->rprim, collectionName,
                        reprName, forcedRepr, &result);
        pathIt++;
    }

    HdRenderIndex::HdDrawItemView finalResult;
    finalResult.reserve(result.size());
    finalResult.insert(finalResult.begin(), result.begin(), result.end());
    return finalResult;
}

bool 
HdRenderIndex::IsInCollection(SdfPath const& id,
                              TfToken const& collectionName) const 
{
    _RprimInfo const* info = TfMapLookupPtr(_rprimMap, id);
    if (info == nullptr) {
        return false;
    }

    return info->rprim->IsInCollection(info->sceneDelegate,
                                       collectionName);
}

SdfPathVector
HdRenderIndex::GetRprimSubtree(SdfPath const& rootPath) const
{
    // PERFORMANCE: This loop can get really hot, ideally we wouldn't iterate
    // over a map, since memory coherency is so bad.
    SdfPathVector paths;
    paths.reserve(1024);
    for (auto p = _rprimIDSet.lower_bound(rootPath); 
            p != _rprimIDSet.end() && p->HasPrefix(rootPath); ++p)
    {
        paths.push_back(*p);
    }
    return paths;
}

namespace {
    struct _RprimSyncRequestVector {
        void PushBack(HdSceneDelegate *sceneDelegate,
                      HdRprim *rprim,
                      size_t reprsMask,
                      int dirtyBits, 
                      int maskedDirtyBits)
        {
            sceneDelegates.push_back(sceneDelegate);
            rprims.push_back(rprim);
            reprsMasks.push_back(reprsMask);
            request.IDs.push_back(rprim->GetId());
            request.allDirtyBits.push_back(dirtyBits);
            request.maskedDirtyBits.push_back(maskedDirtyBits);
        }

        void PushBackShader(SdfPath const& shaderID)
        {
            request.surfaceShaderIDs.push_back(shaderID);
        }

        void PushBackTexture(SdfPath const& textureID)
        {
            request.textureIDs.push_back(textureID);
        }

        std::vector<HdSceneDelegate *> sceneDelegates;
        std::vector<HdRprim *> rprims;
        std::vector<size_t> reprsMasks;
        HdSyncRequestVector request;
    };

    typedef TfHashMap<HdSceneDelegate*, 
                      _RprimSyncRequestVector, TfHash> _RprimSyncRequestMap;

    struct _Worker {
        _RprimSyncRequestMap* _map;
        std::vector<HdSceneDelegate*> _index;
    public:
        _Worker(_RprimSyncRequestMap* map) : _map(map)
        {
            _index.reserve(map->size());
            TF_FOR_ALL(dlgIt, *map) {
                _index.push_back(dlgIt->first);
            }
        }
        
        void Process(size_t begin, size_t end) 
        {
            for (size_t i = begin; i < end; i++) {
                HdSceneDelegate* dlg = _index[i];
                _RprimSyncRequestVector* r = TfMapLookupPtr(*_map, dlg);
                dlg->Sync(&r->request);
            }
        }
    };

    struct _ReprSpec {
        _ReprSpec(TfToken const &repr, bool forced) :
            reprName(repr), forcedRepr(forced) {}
        TfToken reprName;
        bool forcedRepr;

        bool operator < (_ReprSpec const &other) const {
            return reprName < other.reprName ||
                (reprName == other.reprName && (forcedRepr && (!other.forcedRepr)));
        }
        bool operator == (_ReprSpec const &other) const {
            return  (reprName == other.reprName) &&
                    (forcedRepr == other.forcedRepr);
        }
    };

    typedef std::vector<_ReprSpec> _ReprList;

    struct _SyncRPrims {
        _RprimSyncRequestVector &_r;
        _ReprList const &_reprs;
        HdChangeTracker &_tracker;
    public:
        _SyncRPrims( _RprimSyncRequestVector& r,
                     _ReprList const &reprs,
                     HdChangeTracker &tracker)
         : _r(r)
         , _reprs(reprs)
         , _tracker(tracker)
        {
        }

        void Sync(size_t begin, size_t end)
        {
            for (size_t i = begin; i < end; ++i)
            {
                HdSceneDelegate *sceneDelegate = _r.sceneDelegates[i];
                HdRprim &rprim = *_r.rprims[i];
                size_t reprsMask = _r.reprsMasks[i];

                int dirtyBits = _r.request.allDirtyBits[i];

                TF_FOR_ALL(it, _reprs) {
                    if (reprsMask & 1) {
                        rprim.Sync(sceneDelegate,
                                    it->reprName,
                                    it->forcedRepr,
                                    &dirtyBits);
                    }
                    reprsMask >>= 1;
                }

                _tracker.MarkRprimClean(rprim.GetId(), dirtyBits);
            }
        }
    };
};

void
HdRenderIndex::Sync(HdDirtyListSharedPtr const &dirtyList)
{
    _syncQueue.push_back(dirtyList);
}

void
HdRenderIndex::SyncAll()
{
    HD_TRACE_FUNCTION();

    // Merge IDs using the slow SdfPath less-than so that all delegate IDs group
    // together. Unfortunately, FastLessThan makes the optimization below less
    // effective, however the time to build the std::map dominates when using
    // the lexicographic less than.
    std::map<SdfPath, /*reprMask=*/size_t, SdfPath::FastLessThan> dirtyIds;
    _ReprList reprs;
    {
        TRACE_SCOPE("Merge Dirty Lists");
        // If dirty list prims are all sorted, we could do something more
        // efficient here.
        for (auto const& hdDirtyList : _syncQueue) {
            HdRprimCollection const& collection = hdDirtyList->GetCollection();

            _ReprSpec reprSpec(collection.GetReprName(),
                               collection.IsForcedRepr());

            // find reprIndex and append it if not exists.
            int reprIndex = (int)reprs.size();
            for (size_t i = 0; i < reprs.size(); ++i) {
                if (reprs[i] == reprSpec) {
                    reprIndex = i;
                    break;
                }
            }
            if (reprIndex == (int)reprs.size()) {
                reprs.push_back(reprSpec);
            }

            // up-to 64 (collection's) reprs can be synced at once.
            // Note that per-prim repr is not limited here, so this
            // is a fair assumption.
            //
            // XXX: WBN to iterate SyncAll if there are more than 64 reprs
            //      in the extreme case.
            //
            if (!TF_VERIFY(reprIndex < 64)) {
                break;
            }

            // PERFORMANCE: this loop can be expensive.
            for (auto const& sdfPath : hdDirtyList->GetDirtyRprims()) {
                dirtyIds[sdfPath] |= (1 << reprIndex);
            }
        }
    }

    _RprimSyncRequestMap syncMap;
    bool resetVaryingState = false;
    const _BprimMap &textureMap =
                              _bprimTypeMap[HdPrimTypeTokens->texture].bprimMap;

    {
        TRACE_SCOPE("Build Sync Map: Textures");
        // Collect dirty texture IDs.
        // We could/should try to be more sparse here, however finding the
        // intersection of textures used by prims in this RenderPass is currently
        // more expensive than just updating all textures.

        TF_FOR_ALL(it, textureMap) {
            if (HdChangeTracker::IsClean(_tracker.GetBprimDirtyBits(it->first)))
                continue;
            syncMap[it->second.sceneDelegate].PushBackTexture(it->first);
        }
    }

    {
        TRACE_SCOPE("Build Sync Map: Shaders");
        // Collect dirty shader IDs.
        // We could/should try to be more sparse here, however finding the
        // intersection of shaders used by prims in this RenderPass is currently
        // *way* more expensive than just updating all shaders.
        TF_FOR_ALL(it, _shaderMap) {
            if (HdChangeTracker::IsClean(_tracker.GetShaderDirtyBits(it->first)))
                continue;
            syncMap[it->second.sceneDelegate].PushBackShader(it->first);
        }
    }

    {
        TRACE_SCOPE("Build Sync Map: Rprims");
        // Collect dirty Rprim IDs.
        HdSceneDelegate* curdel = nullptr;
        _RprimSyncRequestVector* curvec = nullptr;

        // PERFORMANCE: Hot loop.
        int numSkipped = 0;
        TF_FOR_ALL(idIt, dirtyIds) {
            _RprimMap::const_iterator it = _rprimMap.find(idIt->first);
            if (!TF_VERIFY(it != _rprimMap.end())) {
                continue;
            }

            const _RprimInfo &rprimInfo = it->second;

            int dirtyBits =
                           _tracker.GetRprimDirtyBits(rprimInfo.rprim->GetId());
            size_t reprsMask = idIt->second;

            if (HdChangeTracker::IsClean(dirtyBits)) {
                numSkipped++;
                continue;
            }

            // PERFORMANCE: This loop is constrained by memory access, avoid
            // re-fetching the sync request vector if possible.
            if (curdel != rprimInfo.sceneDelegate) {
                curdel = rprimInfo.sceneDelegate;
                curvec = &syncMap[curdel];
            }
            // XXX: maskedDirtyBits (the last argument) can be removed.
            curvec->PushBack(curdel, rprimInfo.rprim,
                             reprsMask, dirtyBits, dirtyBits);
        }

        // Use a heuristic to determine whether or not to destroy the entire
        // dirty state.  We say that if we've skipped more than 25% of the
        // rprims that were claimed dirty, then it's time to clean up this
        // list.  This leads to performance improvements after many rprims
        // get dirty and then cleaned one, and the steady state becomes a 
        // small number of dirty items.
        if (!dirtyIds.empty()) {
            resetVaryingState = 
                ((float )numSkipped / (float)dirtyIds.size()) > 0.25f;

            if (TfDebug::IsEnabled(HD_VARYING_STATE)) {
                std::cout << "Dirty List Redundancy  = "
                          << ((float )numSkipped * 100.0f / (float)dirtyIds.size())
                          << "% (" <<  numSkipped << " / "
                          << dirtyIds.size() << ")" << std::endl;
            }
        }

    }

    // Drop the GIL before we spawn parallel tasks.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    {
        TRACE_SCOPE("Delegate Sync");
        // Dispatch synchronization work to each delegate.
        _Worker worker(&syncMap);
        WorkParallelForN(syncMap.size(), 
                         boost::bind(&_Worker::Process, worker, _1, _2));
    }

    // Collect results and synchronize.
    WorkArenaDispatcher dispatcher;
    TF_FOR_ALL(dlgIt, syncMap) {
        HdSceneDelegate* sceneDelegate = dlgIt->first;
        _RprimSyncRequestVector& r = dlgIt->second;


        TF_FOR_ALL(textureID, r.request.textureIDs) {
            const _BprimInfo *bprimInfo = TfMapLookupPtr(textureMap, *textureID);

            if (bprimInfo != nullptr) {
                bprimInfo->bprim->Sync(sceneDelegate);
                _tracker.MarkBprimClean(*textureID, HdTexture::Clean);
            }
        }

        TF_FOR_ALL(shaderID, r.request.surfaceShaderIDs) {
            const _ShaderInfo *shaderInfo =
                                          TfMapLookupPtr(_shaderMap, *shaderID);

            if (shaderInfo != nullptr) {
                shaderInfo->shader->Sync(sceneDelegate);
                _tracker.MarkShaderClean(*shaderID, HdChangeTracker::Clean);
            }
        }

        {
            _SyncRPrims workerState(r, reprs, _tracker);

            if (!TfDebug::IsEnabled(HD_DISABLE_MULTITHREADED_RPRIM_SYNC) &&
                  sceneDelegate->IsEnabled(HdOptionTokens->parallelRprimSync)) {
                TRACE_SCOPE("Parallel Rprim Sync");
                dispatcher.Run([&r, workerState]() {
                    WorkParallelForN(r.rprims.size(),
                        boost::bind(&_SyncRPrims::Sync, workerState, _1, _2));
                });
            } else {          
                TRACE_SCOPE("Serial Rprim Sync");
                // Single-threaded version: Call worker directly
                workerState.Sync(0, r.rprims.size());
            }
        }
    }
    dispatcher.Wait();

    {
        TRACE_SCOPE("Clean Up");
        // Give Delegate's to do any post-parrellel work,
        // such as garbage collection.
        TF_FOR_ALL(dlgIt, syncMap) {
            HdSceneDelegate *delegate = dlgIt->first;
            delegate->PostSyncCleanup();
        }

        // Clear out the dirty list for future consumers.
        for (auto const& hdDirtyList : _syncQueue) {
            hdDirtyList->Clear();
        }

        if (resetVaryingState) {
            _tracker.ResetVaryingState();
        }

        
        // Clear all pending dirty lists
        _syncQueue.clear();
    }
}

void
HdRenderIndex::SyncSprims()
{
    TF_FOR_ALL(typeIt, _sprimTypeMap) {
        TF_FOR_ALL(primIt, typeIt->second.sprimMap) {
            const SdfPath &primPath = primIt->first;

            if (_tracker.GetSprimDirtyBits(primPath) != HdChangeTracker::Clean) {
                _SprimInfo &sprimInfo = primIt->second;

                sprimInfo.sprim->Sync(sprimInfo.sceneDelegate);

                _tracker.MarkSprimClean(primPath);
            }
        }
    }
}

void
HdRenderIndex::_CompactPrimIds()
{
    _rprimPrimIdMap.clear();
    // Start prim id as 1 because background for id
    // render is black (id 0)
    _nextPrimId = 1;
    TF_FOR_ALL(it, _rprimMap) {
        it->second.rprim->SetPrimId(_nextPrimId);
        _tracker.MarkRprimDirty(it->first, HdChangeTracker::DirtyPrimID);
        _rprimPrimIdMap[_nextPrimId] = it->first;
        ++_nextPrimId;
    }

}

void
HdRenderIndex::_AllocatePrimId(HdRprim *prim)
{
    HD_TRACE_FUNCTION();
    int32_t maxId = (1 << 24) - 1;
    if(_nextPrimId > maxId) {
        // We are wrapping around our max prim id.. time to reallocate
        _CompactPrimIds();
        // Make sure we have a valid next id after compacting
        TF_VERIFY(_nextPrimId <= maxId);
    }
    prim->SetPrimId(_nextPrimId);
    // note: not marking DirtyPrimID here to avoid undesirable variability tracking.
    _rprimPrimIdMap[_nextPrimId] = prim->GetId();

    ++ _nextPrimId;
}

SdfPath
HdRenderIndex::GetPrimPathFromPrimIdColor(GfVec4i const& primIdColor,
                                          GfVec4i const& instanceIdColor,
                                          int* instanceIndexOut) const
{
    int32_t primId = ((primIdColor[0] & 0xff) <<  0) | 
                     ((primIdColor[1] & 0xff) <<  8) |
                     ((primIdColor[2] & 0xff) << 16);

    _RprimPrimIDMap::const_iterator it = _rprimPrimIdMap.find(primId);
    if(it == _rprimPrimIdMap.end()) {
        return SdfPath();
    }

    if (instanceIndexOut) {
        *instanceIndexOut = ((instanceIdColor[0] & 0xff) <<  0) | 
                            ((instanceIdColor[1] & 0xff) <<  8) |
                            ((instanceIdColor[2] & 0xff) << 16);
    }

    return it->second;
}

void
HdRenderIndex::InsertInstancer(HdSceneDelegate* delegate,
                               SdfPath const &id,
                               SdfPath const &parentId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

#if 0
    // TODO: enable this after patching.
    if (!id.IsAbsolutePath()) {
        TF_CODING_ERROR("All Rprim IDs must be absolute paths <%s>\n",
                id.GetText());
        return;
    }
#endif

    HdInstancerSharedPtr instancer =
        HdInstancerSharedPtr(new HdInstancer(delegate, id, parentId));

    _instancerMap[id] = instancer;
    _tracker.InstancerInserted(id);
}

void
HdRenderIndex::RemoveInstancer(SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _InstancerMap::iterator it = _instancerMap.find(id);
    if (it == _instancerMap.end())
        return;

    _tracker.InstancerRemoved(id);
    _instancerMap.erase(it);
}

HdInstancerSharedPtr
HdRenderIndex::GetInstancer(SdfPath const &id) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdInstancerSharedPtr instancer;
    TfMapLookup(_instancerMap, id, &instancer);

    return instancer;
}

HdRprim const *
HdRenderIndex::GetRprim(SdfPath const &id) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _RprimMap::const_iterator it = _rprimMap.find(id);
    if (it != _rprimMap.end()) {
        return it->second.rprim;
    }

    return nullptr;
}

HdSceneDelegate *
HdRenderIndex::GetSceneDelegateForRprim(SdfPath const &id) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _RprimMap::const_iterator it = _rprimMap.find(id);
    if (it != _rprimMap.end()) {
        const _RprimInfo &rprimInfo = it->second;

        return rprimInfo.sceneDelegate;
    }

    return nullptr;
}

bool
HdRenderIndex::GetSceneDelegateAndInstancerIds(SdfPath const &id,
                                               SdfPath* delegateId,
                                               SdfPath* instancerId) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _RprimMap::const_iterator it = _rprimMap.find(id);
    if (it != _rprimMap.end()) {
        const _RprimInfo &rprimInfo = it->second;

        *delegateId  = rprimInfo.sceneDelegate->GetDelegateID();
        *instancerId = rprimInfo.rprim->GetInstancerId();

        return true;
    }

    return false;
}
