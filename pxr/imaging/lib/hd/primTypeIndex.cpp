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
#include "pxr/imaging/hd/primTypeIndex.h"
#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/tokens.h" // XXX: To be removed, so workaround below.

#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE

template <class PrimType>
Hd_PrimTypeIndex<PrimType>::Hd_PrimTypeIndex()
 : _entries()
 , _index()
{
}

template <class PrimType>
Hd_PrimTypeIndex<PrimType>::~Hd_PrimTypeIndex()
{
}


template <class PrimType>
void
Hd_PrimTypeIndex<PrimType>::InitPrimTypes(const TfTokenVector &primTypes)
{
    size_t primTypeCount = primTypes.size();
    _entries.resize(primTypeCount);

    for (size_t typeIdx = 0; typeIdx < primTypeCount; ++typeIdx) {
        _index.emplace(primTypes[typeIdx], typeIdx);
    }
}

template <class PrimType>
void
Hd_PrimTypeIndex<PrimType>::Clear(HdChangeTracker &tracker,
                                  HdRenderDelegate *renderDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    size_t primTypeCount = _entries.size();
    for (size_t typeIdx = 0; typeIdx < primTypeCount; ++typeIdx) {
        _PrimTypeEntry &typeEntry = _entries[typeIdx];

        for (typename _PrimMap::iterator primIt  = typeEntry.primMap.begin();
                                         primIt != typeEntry.primMap.end();
                                       ++primIt) {
            _TrackerRemovePrim(tracker, primIt->first);
            _PrimInfo &primInfo = primIt->second;
            _RenderDelegateDestroyPrim(renderDelegate, primInfo.prim);
            primInfo.prim = nullptr;
        }
        typeEntry.primMap.clear();
        typeEntry.primIds.clear();
    }
}

template <class PrimType>
void
Hd_PrimTypeIndex<PrimType>::InsertPrim(const TfToken    &typeId,
                                       HdSceneDelegate  *sceneDelegate,
                                       const SdfPath    &primId,
                                       HdChangeTracker  &tracker,
                                       HdRenderDelegate *renderDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (primId.IsEmpty()) {
        return;
    }

    typename _TypeIndex::iterator typeIt = _index.find(typeId);
    if (typeIt ==_index.end()) {
        TF_CODING_ERROR("Unsupported prim type: %s", typeId.GetText());
        return;
    }

    PrimType *prim = _RenderDelegateCreatePrim(renderDelegate, typeId, primId);

    if (prim == nullptr) {
        // Render Delegate is responsible for reporting reason creation failed.
        return;
    }


    HdDirtyBits initialDirtyState = prim->GetInitialDirtyBitsMask();

    _TrackerInsertPrim(tracker, primId, initialDirtyState);

    _PrimTypeEntry &typeEntry = _entries[typeIt->second];

    typeEntry.primMap.emplace(primId, _PrimInfo{sceneDelegate, prim});
    typeEntry.primIds.emplace(primId);
}


template <class PrimType>
void
Hd_PrimTypeIndex<PrimType>::RemovePrim(const TfToken    &typeId,
                                       const SdfPath    &primId,
                                       HdChangeTracker  &tracker,
                                       HdRenderDelegate *renderDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    typename _TypeIndex::iterator typeIt = _index.find(typeId);
    if (typeIt ==_index.end()) {
        TF_CODING_ERROR("Unsupported prim type: %s", typeId.GetText());
        return;
    }

    _PrimTypeEntry &typeEntry = _entries[typeIt->second];

    typename _PrimMap::iterator primIt = typeEntry.primMap.find(primId);
    if (primIt == typeEntry.primMap.end()) {
        return;
    }

    _TrackerRemovePrim(tracker, primId);
    _PrimInfo &primInfo = primIt->second;
    _RenderDelegateDestroyPrim(renderDelegate, primInfo.prim);
    primInfo.prim = nullptr;

    typeEntry.primMap.erase(primIt);
    typeEntry.primIds.erase(primId);
}


template <class PrimType>
PrimType *
Hd_PrimTypeIndex<PrimType>::GetPrim(const TfToken &typeId,
                                    const SdfPath &primId) const
{
    HD_TRACE_FUNCTION();

    typename _TypeIndex::const_iterator typeIt = _index.find(typeId);
    if (typeIt ==_index.end()) {
        TF_CODING_ERROR("Unsupported prim type: %s", typeId.GetText());
        return nullptr;
    }

    const _PrimTypeEntry &typeEntry = _entries[typeIt->second];

    typename _PrimMap::const_iterator it = typeEntry.primMap.find(primId);
    if (it != typeEntry.primMap.end()) {
        return it->second.prim;
    }

    return nullptr;
}

template <class PrimType>
PrimType *
Hd_PrimTypeIndex<PrimType>::GetFallbackPrim(const TfToken &typeId) const
{
    HD_TRACE_FUNCTION();

    typename _TypeIndex::const_iterator typeIt = _index.find(typeId);
    if (typeIt ==_index.end()) {
        TF_CODING_ERROR("Unsupported prim type: %s", typeId.GetText());
        return nullptr;
    }

    const _PrimTypeEntry &typeEntry = _entries[typeIt->second];

    return typeEntry.fallbackPrim;
}


template <class PrimType>
void
Hd_PrimTypeIndex<PrimType>::GetPrimSubtree(const TfToken &typeId,
                                           const SdfPath &rootPath,
                                           SdfPathVector *outPaths) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    typename _TypeIndex::const_iterator typeIt = _index.find(typeId);
    if (typeIt ==_index.end()) {
        TF_CODING_ERROR("Unsupported prim type: %s", typeId.GetText());
        return;
    }

    const _PrimTypeEntry &typeEntry = _entries[typeIt->second];

    // Over-allocate paths, assuming worse-case all paths are going to be
    // returned.
    outPaths->reserve(typeEntry.primIds.size());

    typename _PrimIDSet::const_iterator pathIt =
                                        typeEntry.primIds.lower_bound(rootPath);
     while ((pathIt != typeEntry.primIds.end()) &&
            (pathIt->HasPrefix(rootPath))) {
         outPaths->push_back(*pathIt);
         ++pathIt;
    }
}

template <class PrimType>
bool
Hd_PrimTypeIndex<PrimType>::CreateFallbackPrims(HdRenderDelegate *renderDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    bool success = true;
    for (_TypeIndex::const_iterator typeIt  = _index.begin();
                                    typeIt != _index.end();
                                  ++typeIt) {
        _PrimTypeEntry &typeEntry =  _entries[typeIt->second];

        typeEntry.fallbackPrim =
                           _RenderDelegateCreateFallbackPrim(renderDelegate,
                                                             typeIt->first);

        success &= (typeEntry.fallbackPrim != nullptr);
    }

    return success;
}

template <class PrimType>
void
Hd_PrimTypeIndex<PrimType>::DestroyFallbackPrims(HdRenderDelegate *renderDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    size_t numTypes = _entries.size();
    for (size_t typeIdx = 0; typeIdx < numTypes; ++typeIdx) {
        _PrimTypeEntry &typeEntry =  _entries[typeIdx];

       _RenderDelegateDestroyPrim(renderDelegate, typeEntry.fallbackPrim);
       typeEntry.fallbackPrim = nullptr;
    }
}

template <class PrimType>
void
Hd_PrimTypeIndex<PrimType>::SyncPrims(HdChangeTracker &tracker)
{
    size_t numTypes = _entries.size();

    // XXX: Temporary solution to Sync Ordering issue.
    // Currently SPrims are synced before BPrims, but BPrims are supposed to
    // be first.  This is an issue for Shaders that depend on Textures being
    // synced and registered.
    // So for now, skip sync'ing shaders here and sync them later seperatly
    // after Bprims.

    size_t shaderTypeIdxXXX = -1;
    _TypeIndex::const_iterator typeItXXX = _index.find(HdPrimTypeTokens->shader);
    if (typeItXXX != _index.end()) {
        shaderTypeIdxXXX = typeItXXX->second;
    }



    for (size_t typeIdx = 0; typeIdx < numTypes; ++typeIdx) {
        if (typeIdx == shaderTypeIdxXXX) {
            continue;
        }

        _PrimTypeEntry &typeEntry =  _entries[typeIdx];

        for (typename _PrimMap::iterator primIt  = typeEntry.primMap.begin();
                                         primIt != typeEntry.primMap.end();
                                       ++primIt) {
            const SdfPath &primPath = primIt->first;

            HdDirtyBits dirtyBits = _TrackerGetPrimDirtyBits(tracker, primPath);

            if (dirtyBits != HdChangeTracker::Clean) {

                _PrimInfo &primInfo = primIt->second;

                primInfo.prim->Sync(primInfo.sceneDelegate);

                _TrackerMarkPrimClean(tracker, primPath);
            }
        }
    }
}

// XXX: Transitional API
template <class PrimType>
const typename Hd_PrimTypeIndex<PrimType>::_PrimMap *
Hd_PrimTypeIndex<PrimType>::GetPrimMapXXX(TfToken const &typeId) const
{

    typename _TypeIndex::const_iterator typeIt = _index.find(typeId);
    if (typeIt ==_index.end()) {
        TF_CODING_ERROR("Unsupported prim type: %s", typeId.GetText());
        return nullptr;
    }

    return &_entries[typeIt->second].primMap;
}

////////////////////////////////////////////////////////////////////////////////
//
// Sprim Template Specialization
//
template <>
// static
void
Hd_PrimTypeIndex<HdSprim>::_TrackerInsertPrim(HdChangeTracker &tracker,
                                              const SdfPath  &path,
                                              HdDirtyBits initialDirtyState)
{
    tracker.SprimInserted(path, initialDirtyState);
}

template <>
// static
void
Hd_PrimTypeIndex<HdSprim>::_TrackerRemovePrim(HdChangeTracker &tracker,
                                              const SdfPath &path)
{
    tracker.SprimRemoved(path);
}

template <>
// static
HdDirtyBits
Hd_PrimTypeIndex<HdSprim>::_TrackerGetPrimDirtyBits(HdChangeTracker &tracker,
                                                   const SdfPath &path)
{
    return tracker.GetSprimDirtyBits(path);
}

template <>
// static
void
Hd_PrimTypeIndex<HdSprim>::_TrackerMarkPrimClean(HdChangeTracker &tracker,
                                                 const SdfPath &path)
{
    tracker.MarkSprimClean(path);
}

template <>
// static
HdSprim *
Hd_PrimTypeIndex<HdSprim>::_RenderDelegateCreatePrim(HdRenderDelegate *renderDelegate,
                                                     const TfToken &typeId,
                                                     const SdfPath &primId)
{
    return renderDelegate->CreateSprim(typeId, primId);
}

template <>
// static
HdSprim *
Hd_PrimTypeIndex<HdSprim>::_RenderDelegateCreateFallbackPrim(HdRenderDelegate *renderDelegate,
                                                             const TfToken &typeId)
{
    return renderDelegate->CreateFallbackSprim(typeId);
}

template <>
// static
void
Hd_PrimTypeIndex<HdSprim>::_RenderDelegateDestroyPrim(HdRenderDelegate *renderDelegate,
                                                      HdSprim *prim)
{
  renderDelegate->DestroySprim(prim);
}

template class Hd_PrimTypeIndex<HdSprim>;
////////////////////////////////////////////////////////////////////////////////
//
// Bprim Template Specialization
//
template <>
// static
void
Hd_PrimTypeIndex<HdBprim>::_TrackerInsertPrim(HdChangeTracker &tracker,
                                              const SdfPath   &path,
                                              HdDirtyBits     initialDirtyState)
{
    tracker.BprimInserted(path, initialDirtyState);
}

template <>
// static
void
Hd_PrimTypeIndex<HdBprim>::_TrackerRemovePrim(HdChangeTracker &tracker,
                                              const SdfPath &path)
{
    tracker.BprimRemoved(path);
}

template <>
// static
HdDirtyBits
Hd_PrimTypeIndex<HdBprim>::_TrackerGetPrimDirtyBits(HdChangeTracker &tracker,
                                                    const SdfPath   &path)
{
    return tracker.GetBprimDirtyBits(path);
}

template <>
// static
void
Hd_PrimTypeIndex<HdBprim>::_TrackerMarkPrimClean(HdChangeTracker &tracker,
                                                 const SdfPath &path)
{
    tracker.MarkBprimClean(path);
}

template <>
// static
HdBprim *
Hd_PrimTypeIndex<HdBprim>::_RenderDelegateCreatePrim(HdRenderDelegate *renderDelegate,
                                                     const TfToken &typeId,
                                                     const SdfPath &primId)
{
    return renderDelegate->CreateBprim(typeId, primId);
}

template <>
// static
HdBprim *
Hd_PrimTypeIndex<HdBprim>::_RenderDelegateCreateFallbackPrim(HdRenderDelegate *renderDelegate,
                                                             const TfToken &typeId)
{
    return renderDelegate->CreateFallbackBprim(typeId);
}

template <>
// static
void
Hd_PrimTypeIndex<HdBprim>::_RenderDelegateDestroyPrim(HdRenderDelegate *renderDelegate,
                                                      HdBprim *prim)
{
  renderDelegate->DestroyBprim(prim);
}

template class Hd_PrimTypeIndex<HdBprim>;

PXR_NAMESPACE_CLOSE_SCOPE
