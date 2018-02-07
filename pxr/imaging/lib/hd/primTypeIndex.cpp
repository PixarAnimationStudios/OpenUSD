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
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sprim.h"

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
        typeEntry.primIds.Clear();
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

    typename _TypeIndex::iterator typeIt = _index.find(typeId);
    if (typeIt ==_index.end()) {
        TF_CODING_ERROR("Unsupported prim type: %s", typeId.GetText());
        return;
    }

    SdfPath const &sceneDelegateId = sceneDelegate->GetDelegateID();
    if (!primId.HasPrefix(sceneDelegateId)) {
        TF_CODING_ERROR("Scene Delegate Id (%s) must prefix prim Id (%s)",
                        sceneDelegateId.GetText(), primId.GetText());
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

    typeEntry.primIds.Insert(primId);
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
    typeEntry.primIds.Remove(primId);
}

template <class PrimType>
void
Hd_PrimTypeIndex<PrimType>::RemoveSubtree(const SdfPath &root,
                                          HdSceneDelegate* sceneDelegate,
                                          HdChangeTracker &tracker,
                                          HdRenderDelegate *renderDelegate)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    struct _Range {
        size_t _start;
        size_t _end;

        _Range() = default;
        _Range(size_t start, size_t end)
         : _start(start)
         , _end(end)
        {
        }
    };


    size_t numTypes = _entries.size();
    for (size_t typeIdx = 0; typeIdx < numTypes; ++typeIdx) {
        _PrimTypeEntry &typeEntry = _entries[typeIdx];

        HdPrimGather gather;
        _Range totalRange;
        std::vector<_Range> rangesToRemove;

        const SdfPathVector &ids = typeEntry.primIds.GetIds();
        if (gather.SubtreeAsRange(ids,
                                   root,
                                   &totalRange._start,
                                   &totalRange._end)) {

            // end is inclusive!
            size_t currentRangeStart = totalRange._start;
            for (size_t primIdIdx  = totalRange._start;
                        primIdIdx <= totalRange._end;
                      ++primIdIdx) {
                const SdfPath &primId = ids[primIdIdx];

                typename _PrimMap::iterator primIt = typeEntry.primMap.find(primId);
                if (primIt == typeEntry.primMap.end()) {
                    TF_CODING_ERROR("Prim in id list not in info map: %s",
                                    primId.GetText());
                } else {
                    _PrimInfo &primInfo = primIt->second;

                    if (primInfo.sceneDelegate == sceneDelegate) {
                        _TrackerRemovePrim(tracker, primId);
                        _RenderDelegateDestroyPrim(renderDelegate, primInfo.prim);
                        primInfo.prim = nullptr;

                        typeEntry.primMap.erase(primIt);
                    } else {
                        if (currentRangeStart < primIdIdx) {
                            rangesToRemove.emplace_back(currentRangeStart,
                                                        primIdIdx - 1);
                        }

                        currentRangeStart = primIdIdx + 1;
                    }
                }
            }

            // Remove final range
            if (currentRangeStart <= totalRange._end) {
                rangesToRemove.emplace_back(currentRangeStart,
                                            totalRange._end);
            }

            // Remove ranges from id's in back to front order to not invalidate indices
            while (!rangesToRemove.empty()) {
                _Range &range = rangesToRemove.back();

                typeEntry.primIds.RemoveRange(range._start, range._end);
                rangesToRemove.pop_back();
            }
        }
    }
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
                                           SdfPathVector *outPaths)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    typename _TypeIndex::const_iterator typeIt = _index.find(typeId);
    if (typeIt ==_index.end()) {
        TF_CODING_ERROR("Unsupported prim type: %s", typeId.GetText());
        return;
    }

    _PrimTypeEntry &typeEntry = _entries[typeIt->second];

    HdPrimGather gather;
    gather.Subtree(typeEntry.primIds.GetIds(), rootPath, outPaths);
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
Hd_PrimTypeIndex<PrimType>::SyncPrims(HdChangeTracker  &tracker,
                                      HdRenderParam    *renderParam)
{
    size_t numTypes = _entries.size();

    for (size_t typeIdx = 0; typeIdx < numTypes; ++typeIdx) {
        _PrimTypeEntry &typeEntry =  _entries[typeIdx];

        for (typename _PrimMap::iterator primIt  = typeEntry.primMap.begin();
                                         primIt != typeEntry.primMap.end();
                                       ++primIt) {
            const SdfPath &primPath = primIt->first;

            HdDirtyBits dirtyBits = _TrackerGetPrimDirtyBits(tracker, primPath);

            if (dirtyBits != HdChangeTracker::Clean) {

                _PrimInfo &primInfo = primIt->second;

                primInfo.prim->Sync(primInfo.sceneDelegate,
                                    renderParam,
                                    &dirtyBits);

                _TrackerMarkPrimClean(tracker, primPath, dirtyBits);
            }
        }
    }
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
Hd_PrimTypeIndex<HdSprim>::_TrackerMarkPrimClean(
                                           HdChangeTracker &tracker,
                                           const SdfPath &path,
                                           HdDirtyBits dirtyBits)
{
    tracker.MarkSprimClean(path, dirtyBits);
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
Hd_PrimTypeIndex<HdBprim>::_TrackerMarkPrimClean(
                                           HdChangeTracker &tracker,
                                           const SdfPath &path,
                                           HdDirtyBits dirtyBits)
{
    tracker.MarkBprimClean(path, dirtyBits);
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
