//
// Copyright 2018 Pixar
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
#include "pxr/usdImaging/usdImaging/indexProxy.h"

#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(USDIMAGING_LEGACY_UPDATE_FOR_TIME, 0,
        "Run UpdateForTime every time any prim is marked dirty (legacy behavior)");
static bool _LegacyUpdateForTime() {
    static bool _v = TfGetEnvSetting(USDIMAGING_LEGACY_UPDATE_FOR_TIME) == 0;
    return _v;
}

UsdImagingDelegate::_HdPrimInfo*
UsdImagingIndexProxy::_AddHdPrimInfo(SdfPath const &cachePath,
                                     UsdPrim const& usdPrim,
                                     UsdImagingPrimAdapterSharedPtr const& adapter)
{
    UsdImagingPrimAdapterSharedPtr adapterToInsert;
    if (adapter) {
        adapterToInsert = adapter;
    } else {
        adapterToInsert = _delegate->_AdapterLookup(usdPrim);

        // When no adapter was provided, look it up based on the type of the
        // prim.
        if (!adapterToInsert) {
            TF_CODING_ERROR("No adapter was found for <%s> (type: %s)\n",
                cachePath.GetText(),
                usdPrim ? usdPrim.GetTypeName().GetText() : "<expired prim>");
            return nullptr;
        }
    }

    TF_DEBUG(USDIMAGING_CHANGES).Msg(
        "[Add HdPrim Info] <%s> adapter=%s\n",
        cachePath.GetText(),
        TfType::GetCanonicalTypeName(typeid(*(adapterToInsert.get()))).c_str());

    // Currently, we don't support more than one adapter dependency per usd
    // prim, but we could relax this restriction if it's useful.

    bool inserted;
    UsdImagingDelegate::_HdPrimInfoMap::iterator it;
    std::tie(it, inserted) = _delegate->_hdPrimInfoMap.insert(
            UsdImagingDelegate::_HdPrimInfoMap::value_type(cachePath,
                                          UsdImagingDelegate::_HdPrimInfo()));

    UsdImagingDelegate::_HdPrimInfo &primInfo = it->second;

    if (!inserted) {
        // XXX: ideally, we'd TF_VERIFY(inserted) here, but usd resyncs can
        // sometimes cause double-inserts, and de-duplicating the usd
        // population list is potentially expensive...
        return nullptr;
    }

    primInfo.adapter         = adapterToInsert;
    primInfo.timeVaryingBits = 0;
    primInfo.dirtyBits       = 0;
    primInfo.usdPrim         = usdPrim;

    // Register the prim dependency; skip AddDependency so it doesn't get
    // added to the extraDependencies list.
    SdfPath usdPath = usdPrim.GetPath();
    _delegate->_dependencyInfo.insert(
        UsdImagingDelegate::_DependencyMap::value_type(usdPath, cachePath));
    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Add dependency] <%s> -> <%s>\n",
        usdPath.GetText(), cachePath.GetText());

    // precache cache path to index path translations
    SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
    _delegate->_cache2indexPath[cachePath] = indexPath;
    _delegate->_index2cachePath[indexPath] = cachePath;

    return &primInfo;
}

void
UsdImagingIndexProxy::_AddTask(SdfPath const& usdPath) 
{
    _delegate->_AddTask(_worker, usdPath);
}

void
UsdImagingIndexProxy::_RemoveDependencies(SdfPath const& cachePath)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _delegate->_GetHdPrimInfo(cachePath);
    if (!TF_VERIFY(primInfo != nullptr, "%s", cachePath.GetText())) {
        return;
    }
    _dependenciesToRemove.push_back(
        UsdImagingDelegate::_DependencyMap::value_type(
            primInfo->usdPrim.GetPath(), cachePath));

    for (SdfPath const& dep : primInfo->extraDependencies) {
        _dependenciesToRemove.push_back(
            UsdImagingDelegate::_DependencyMap::value_type(
                dep, cachePath));
    }
}

void
UsdImagingIndexProxy::RemovePrimInfoDependency(SdfPath const& cachePath)
{
    // This one doesn't go through ProcessRemovals...  It's intended to be
    // called right after _AddHdPrimInfo, to reverse the dependency that
    // function adds.
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _delegate->_GetHdPrimInfo(cachePath);
    if (!TF_VERIFY(primInfo != nullptr, "%s", cachePath.GetText())) {
        return;
    }
    auto range = _delegate->_dependencyInfo.equal_range(
        primInfo->usdPrim.GetPath());
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == cachePath) {
            TF_DEBUG(USDIMAGING_CHANGES).
                Msg("[Revert dependency] <%s> -> <%s>\n",
                    it->first.GetText(), it->second.GetText());
            _delegate->_dependencyInfo.erase(it);
            break;
        }
    }
}

void
UsdImagingIndexProxy::AddDependency(SdfPath const& cachePath,
                                    UsdPrim const& usdPrim)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _delegate->_GetHdPrimInfo(cachePath);
    if (!TF_VERIFY(primInfo != nullptr, "%s", cachePath.GetText())) {
        return;
    }

    SdfPath usdPath = usdPrim.GetPath();
    if (primInfo->extraDependencies.count(usdPath) != 0) {
        // XXX: Ideally, we'd TF_VERIFY here, but usd resyncs can
        // sometimes cause double-inserts (see _AddHdPrimInfo), so we need to
        // silently guard against this.
        return;
    }

    _delegate->_dependencyInfo.insert(
        UsdImagingDelegate::_DependencyMap::value_type(usdPath, cachePath));
    primInfo->extraDependencies.insert(usdPath);

    TF_DEBUG(USDIMAGING_CHANGES).Msg("[Add dependency] <%s> -> <%s>\n",
        usdPath.GetText(), cachePath.GetText());
}

void
UsdImagingIndexProxy::InsertRprim(
                             TfToken const& primType,
                             SdfPath const& cachePath,
                             UsdPrim const& usdPrim,
                             UsdImagingPrimAdapterSharedPtr adapter)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _AddHdPrimInfo(cachePath, usdPrim, adapter);

    if (primInfo) {
        HdRenderIndex &renderIndex = _delegate->GetRenderIndex();
        SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
        renderIndex.InsertRprim(primType, _delegate, indexPath);

        // NOTE: Starting from AllDirty doesn't necessarily match what the
        //       render delegate's concrete implementation of a given Rprim
        //       might return but will be fully inclusive of it. Not querying it
        //       directly from the change tracker immediately provides
        //       flexibility as to when insertions will be processed. This is
        //       relevant to downstream consumption patterns when emulated via
        //       a scene index.
        primInfo->dirtyBits = HdChangeTracker::AllDirty;
        _delegate->_dirtyCachePaths.insert(cachePath);

        _AddTask(cachePath);
    }
}

void
UsdImagingIndexProxy::InsertSprim(
                             TfToken const& primType,
                             SdfPath const& cachePath,
                             UsdPrim const& usdPrim,
                             UsdImagingPrimAdapterSharedPtr adapter)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _AddHdPrimInfo(cachePath, usdPrim, adapter);

    if (primInfo) {
        HdRenderIndex &renderIndex = _delegate->GetRenderIndex();
        SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
        renderIndex.InsertSprim(primType, _delegate, indexPath);

        // NOTE: Starting from AllDirty doesn't necessarily match what the
        //       render delegate's concrete implementation of a given Sprim
        //       might return but will be fully inclusive of it. Not querying it
        //       directly from the change tracker immediately provides
        //       flexibility as to when insertions will be processed. This is
        //       relevant to downstream consumption patterns when emulated via
        //       a scene index.
        primInfo->dirtyBits = HdChangeTracker::AllDirty;
        _delegate->_dirtyCachePaths.insert(cachePath);

        _AddTask(cachePath);
    }
}

void
UsdImagingIndexProxy::InsertBprim(
                             TfToken const& primType,
                             SdfPath const& cachePath,
                             UsdPrim const& usdPrim,
                             UsdImagingPrimAdapterSharedPtr adapter)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _AddHdPrimInfo(cachePath, usdPrim, adapter);

    if (primInfo) {
        HdRenderIndex &renderIndex = _delegate->GetRenderIndex();
        SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
        renderIndex.InsertBprim(primType, _delegate, indexPath);

        // NOTE: Starting from AllDirty doesn't necessarily match what the
        //       render delegate's concrete implementation of a given Bprim
        //       might return but will be fully inclusive of it. Not querying it
        //       directly from the change tracker immediately provides
        //       flexibility as to when insertions will be processed. This is
        //       relevant to downstream consumption patterns when emulated via
        //       a scene index.
        primInfo->dirtyBits = HdChangeTracker::AllDirty;

        _delegate->_dirtyCachePaths.insert(cachePath);

        _AddTask(cachePath);
    }
}

void
UsdImagingIndexProxy::InsertInstancer(
                             SdfPath const& cachePath,
                             UsdPrim const& usdPrim,
                             UsdImagingPrimAdapterSharedPtr adapter)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _AddHdPrimInfo(cachePath, usdPrim, adapter);

    if (primInfo) {
        HdRenderIndex &renderIndex = _delegate->GetRenderIndex();
        SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
        renderIndex.InsertInstancer(_delegate, indexPath);

        // NOTE: Starting from AllDirty doesn't necessarily match what the
        //       render delegate's concrete implementation of a given Bprim
        //       might return but will be fully inclusive of it. Not querying it
        //       directly from the change tracker immediately provides
        //       flexibility as to when insertions will be processed. This is
        //       relevant to downstream consumption patterns when emulated via
        //       a scene index.
        primInfo->dirtyBits = HdChangeTracker::AllDirty;
        _delegate->_dirtyCachePaths.insert(cachePath);

        TF_DEBUG(USDIMAGING_INSTANCER).Msg(
            "[Instancer Inserted] %s, adapter = %s\n",
            cachePath.GetText(),
            adapter ? TfType::GetCanonicalTypeName(typeid(*adapter)).c_str()
                    : "none");

        _AddTask(cachePath);
    }
}

void
UsdImagingIndexProxy::Repopulate(SdfPath const& usdPath)
{ 
    // Repopulation is deferred to enable batch processing in parallel.
    _usdPathsToRepopulate.push_back(usdPath); 
}

void
UsdImagingIndexProxy::RequestTrackVariability(SdfPath const& cachePath)
{
    _AddTask(cachePath);
}

void
UsdImagingIndexProxy::RequestUpdateForTime(SdfPath const& cachePath)
{
    _delegate->_dirtyCachePaths.insert(cachePath);
}

void
UsdImagingIndexProxy::_UniqueifyPathsToRepopulate()
{
    if (_usdPathsToRepopulate.empty()) {
        return;
    }

    std::sort(_usdPathsToRepopulate.begin(), _usdPathsToRepopulate.end());
    auto last = std::unique(_usdPathsToRepopulate.begin(),
                            _usdPathsToRepopulate.end(),
                            [](SdfPath const &l, SdfPath const &r) {
                                return r.HasPrefix(l);
                            });
    _usdPathsToRepopulate.erase(last, _usdPathsToRepopulate.end());
}

void
UsdImagingIndexProxy::MarkRprimDirty(SdfPath const& cachePath,
                                     HdDirtyBits dirtyBits)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _delegate->_GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo, "%s", cachePath.GetText())) {
        primInfo->dirtyBits |= dirtyBits;
        if (_LegacyUpdateForTime()) {
            _delegate->_dirtyCachePaths.insert(cachePath);
        }
    }

    HdChangeTracker &tracker = _delegate->GetRenderIndex().GetChangeTracker();
    SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
    tracker.MarkRprimDirty(indexPath, dirtyBits);
}

void
UsdImagingIndexProxy::MarkSprimDirty(SdfPath const& cachePath,
                                     HdDirtyBits dirtyBits)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _delegate->_GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo, "%s", cachePath.GetText())) {
        primInfo->dirtyBits |= dirtyBits;
        if (_LegacyUpdateForTime()) {
            _delegate->_dirtyCachePaths.insert(cachePath);
        }
    }

    HdChangeTracker &tracker = _delegate->GetRenderIndex().GetChangeTracker();
    SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
    tracker.MarkSprimDirty(indexPath, dirtyBits);
}

void
UsdImagingIndexProxy::MarkBprimDirty(SdfPath const& cachePath,
                                     HdDirtyBits dirtyBits)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _delegate->_GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo, "%s", cachePath.GetText())) {
        primInfo->dirtyBits |= dirtyBits;
        if (_LegacyUpdateForTime()) {
            _delegate->_dirtyCachePaths.insert(cachePath);
        }
    }

    HdChangeTracker &tracker = _delegate->GetRenderIndex().GetChangeTracker();
    SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
    tracker.MarkBprimDirty(indexPath, dirtyBits);
}

void
UsdImagingIndexProxy::MarkInstancerDirty(SdfPath const& cachePath,
                                         HdDirtyBits dirtyBits)
{
    UsdImagingDelegate::_HdPrimInfo *primInfo =
        _delegate->_GetHdPrimInfo(cachePath);
    if (TF_VERIFY(primInfo, "%s", cachePath.GetText())) {
        primInfo->dirtyBits |= dirtyBits;
        if (_LegacyUpdateForTime()) {
            _delegate->_dirtyCachePaths.insert(cachePath);
        }
    }

    HdChangeTracker &tracker = _delegate->GetRenderIndex().GetChangeTracker();
    SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
    tracker.MarkInstancerDirty(indexPath, dirtyBits);
}

UsdImagingPrimAdapterSharedPtr
UsdImagingIndexProxy::GetMaterialAdapter(UsdPrim const& materialPrim)
{
    // Note that if the material is instanced, we ignore the instancing
    // and just return a material adapter for the instance path instead.
    UsdImagingPrimAdapterSharedPtr materialAdapter =
        _delegate->_AdapterLookup(materialPrim, true);
    return materialAdapter &&
           materialAdapter->IsSupported(this) ? materialAdapter : nullptr;
}

bool
UsdImagingIndexProxy::IsPopulated(SdfPath const& cachePath) const
{
    return _delegate->_hdPrimInfoMap.find(cachePath) !=
           _delegate->_hdPrimInfoMap.end();
}

bool
UsdImagingIndexProxy::IsRprimTypeSupported(TfToken const& typeId) const
{
    return _delegate->GetRenderIndex().IsRprimTypeSupported(typeId);
}

bool
UsdImagingIndexProxy::IsSprimTypeSupported(TfToken const& typeId) const
{
    return _delegate->GetRenderIndex().IsSprimTypeSupported(typeId);
}

bool
UsdImagingIndexProxy::IsBprimTypeSupported(TfToken const& typeId) const
{
    return _delegate->GetRenderIndex().IsBprimTypeSupported(typeId);
}

void
UsdImagingIndexProxy::_ProcessRemovals()
{
    TRACE_FUNCTION();
    HdRenderIndex& index = _delegate->GetRenderIndex();
    
    {
        TRACE_FUNCTION_SCOPE("Rprims");
        TF_FOR_ALL(it, _rprimsToRemove) {
            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Remove Rprim] <%s>\n",
                                    it->GetText());

            index.RemoveRprim(_delegate->ConvertCachePathToIndexPath(*it));
        }
        _rprimsToRemove.clear();
    }

    {
        TRACE_FUNCTION_SCOPE("instancers");
        TF_FOR_ALL(it, _instancersToRemove) {
            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Remove Instancer] <%s>\n",
                                    it->GetText());

            index.RemoveInstancer(_delegate->ConvertCachePathToIndexPath(*it));
        }
        _instancersToRemove.clear();
    }

    {
        TRACE_FUNCTION_SCOPE("sprims");
        TF_FOR_ALL(it, _sprimsToRemove) {
            const TfToken &primType  = it->primType;
            const SdfPath &cachePath = it->cachePath;

            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Remove Sprim] <%s>\n",
                                             cachePath.GetText());

            index.RemoveSprim(primType,
                              _delegate->ConvertCachePathToIndexPath(cachePath));
        }
        _sprimsToRemove.clear();
    }

    {
        TRACE_FUNCTION_SCOPE("bprims");
        TF_FOR_ALL(it, _bprimsToRemove) {
            const TfToken &primType  = it->primType;
            const SdfPath &cachePath = it->cachePath;

            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Remove Bprim] <%s>\n",
                                             cachePath.GetText());

            index.RemoveBprim(primType,
                              _delegate->ConvertCachePathToIndexPath(cachePath));
        }
        _bprimsToRemove.clear();
    }

    // If we're removing hdPrimInfo entries, we need to rebuild the
    // time-varying cache.
    if (_hdPrimInfoToRemove.size() > 0) {
        _delegate->_timeVaryingPrimCacheValid = false;
    }

    {
        TRACE_FUNCTION_SCOPE("primInfo");
        TF_FOR_ALL(it, _hdPrimInfoToRemove) {
            SdfPath cachePath = *it;

            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Remove PrimInfo] <%s>\n",
                                             cachePath.GetText());


            _delegate->_primvarDescCache.Clear(cachePath);
            _delegate->_refineLevelMap.erase(cachePath);
            _delegate->_pickablesMap.erase(cachePath);

            _delegate->_hdPrimInfoMap.erase(cachePath);

            SdfPath indexPath = _delegate->ConvertCachePathToIndexPath(cachePath);
            _delegate->_cache2indexPath.erase(cachePath);
            _delegate->_index2cachePath.erase(indexPath);
        }
        _hdPrimInfoToRemove.clear();
    }

    {
        TRACE_FUNCTION_SCOPE("dependency");
        TF_FOR_ALL(it, _dependenciesToRemove) {
            UsdImagingDelegate::_DependencyMap::value_type dep = *it;

            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Remove dependency] <%s> -> <%s>\n",
                dep.first.GetText(), dep.second.GetText());

            auto range = _delegate->_dependencyInfo.equal_range(dep.first);
            for (auto it = range.first; it != range.second; ++it) {
                if (it->second == dep.second) {
                    _delegate->_dependencyInfo.erase(it);
                    break;
                }
            }
        }
        _dependenciesToRemove.clear();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

