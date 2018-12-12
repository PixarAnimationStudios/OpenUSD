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

PXR_NAMESPACE_OPEN_SCOPE

void
UsdImagingIndexProxy::AddPrimInfo(SdfPath const &cachePath,
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
            return;
        }
    }

    TF_DEBUG(USDIMAGING_CHANGES).Msg(
        "[Adding Prim Info] <%s> adapter=%s\n",
        cachePath.GetText(),
        TfType::GetCanonicalTypeName(typeid(*(adapterToInsert.get()))).c_str());

    // Currently, we don't support more than one adapter dependency per usd
    // prim, but we could relax this restriction if it's useful.

    bool inserted;
    UsdImagingDelegate::_PrimInfoMap::iterator it;
    std::tie(it, inserted) = _delegate->_primInfoMap.insert(
            UsdImagingDelegate::_PrimInfoMap::value_type(cachePath,
                                              UsdImagingDelegate::_PrimInfo()));

    UsdImagingDelegate::_PrimInfo &primInfo = it->second;

    if (!inserted) {
        // Native Instancing can add the same prim twice, because it
        // reuses the first prim as the master.  This is ok if adapter
        // and prim are the same (i.e. it's a no-op); in this case we
        // silently ignore the collision.  Otherwise it's an error.
        if ((adapterToInsert != primInfo.adapter) ||
            (usdPrim         != primInfo.usdPrim)) {

            TF_CODING_ERROR("Different prim added at same location: "
                            "path = <%s>, "
                            "new prim = <%s>, old prim = <%s>\n",
                            cachePath.GetText(),
                            usdPrim.GetPath().GetText(),
                            primInfo.usdPrim.GetPath().GetText());
        }
        return;
    }


    primInfo.adapter         = adapterToInsert;
    primInfo.timeVaryingBits = 0;
    primInfo.dirtyBits       = 0;
    primInfo.usdPrim         = usdPrim;

    _delegate->_usdIds.Insert(cachePath);

    // precache cache path to index path translations
    SdfPath indexPath = _delegate->GetPathForIndex(cachePath);
    _delegate->_cache2indexPath[cachePath] = indexPath;
    _delegate->_index2cachePath[indexPath] = cachePath;
}

void
UsdImagingIndexProxy::_AddTask(SdfPath const& usdPath) 
{
    _delegate->_AddTask(_worker, usdPath);
}

void
UsdImagingIndexProxy::InsertRprim(
                             TfToken const& primType,
                             SdfPath const& cachePath,
                             SdfPath const& parentPath,
                             UsdPrim const& usdPrim,
                             UsdImagingPrimAdapterSharedPtr adapter)
{
    _delegate->GetRenderIndex().InsertRprim(primType, _delegate,
        _delegate->GetPathForIndex(cachePath),
        _delegate->GetPathForIndex(parentPath));

    AddPrimInfo(cachePath, usdPrim, adapter);
    _AddTask(cachePath);
}

void
UsdImagingIndexProxy::InsertSprim(
                             TfToken const& primType,
                             SdfPath const& cachePath,
                             UsdPrim const& usdPrim,
                             UsdImagingPrimAdapterSharedPtr adapter)
{
    _delegate->GetRenderIndex().InsertSprim(primType, _delegate,
        _delegate->GetPathForIndex(cachePath));

    AddPrimInfo(cachePath, usdPrim, adapter);
    _AddTask(cachePath);
}

void
UsdImagingIndexProxy::InsertBprim(
                             TfToken const& primType,
                             SdfPath const& cachePath,
                             UsdPrim const& usdPrim,
                             UsdImagingPrimAdapterSharedPtr adapter)
{
    _delegate->GetRenderIndex().InsertBprim(primType, _delegate,
        _delegate->GetPathForIndex(cachePath));

    AddPrimInfo(cachePath, usdPrim, adapter);
    _AddTask(cachePath);
}
void
UsdImagingIndexProxy::Repopulate(SdfPath const& usdPath)
{ 
    // Repopulation is deferred to enable batch processing in parallel.
    _pathsToRepopulate.push_back(usdPath); 
}

void
UsdImagingIndexProxy::Refresh(SdfPath const& cachePath)
{
    _AddTask(cachePath);
}

void 
UsdImagingIndexProxy::RefreshInstancer(SdfPath const& instancerPath)
{
    _AddTask(instancerPath);
    MarkInstancerDirty(instancerPath, HdChangeTracker::AllDirty);
}

bool
UsdImagingIndexProxy::HasRprim(SdfPath const &cachePath)
{
    return _delegate->GetRenderIndex().HasRprim(
        _delegate->GetPathForIndex(cachePath));
}

void
UsdImagingIndexProxy::MarkRprimDirty(SdfPath const& cachePath,
                                     HdDirtyBits dirtyBits)
{
    HdChangeTracker &tracker = _delegate->GetRenderIndex().GetChangeTracker();
    SdfPath indexPath = _delegate->GetPathForIndex(cachePath);
    tracker.MarkRprimDirty(indexPath, dirtyBits);
}

void
UsdImagingIndexProxy::MarkSprimDirty(SdfPath const& cachePath,
                                     HdDirtyBits dirtyBits)
{
    HdChangeTracker &tracker = _delegate->GetRenderIndex().GetChangeTracker();
    SdfPath indexPath = _delegate->GetPathForIndex(cachePath);
    tracker.MarkSprimDirty(indexPath, dirtyBits);
}

void
UsdImagingIndexProxy::MarkBprimDirty(SdfPath const& cachePath,
                                     HdDirtyBits dirtyBits)
{
    HdChangeTracker &tracker = _delegate->GetRenderIndex().GetChangeTracker();
    SdfPath indexPath = _delegate->GetPathForIndex(cachePath);
    tracker.MarkBprimDirty(indexPath, dirtyBits);
}

void
UsdImagingIndexProxy::MarkInstancerDirty(SdfPath const& cachePath,
                                         HdDirtyBits dirtyBits)
{
    HdChangeTracker &tracker = _delegate->GetRenderIndex().GetChangeTracker();
    SdfPath indexPath = _delegate->GetPathForIndex(cachePath);
    tracker.MarkInstancerDirty(indexPath, dirtyBits);

    // XXX: Currently, instancers are part of delegate sync even though they
    // aren't in the sync request. This means we need to duplicate their
    // change tracking. This can go away when instancers are part of delegate
    // sync.
    UsdImagingDelegate::_PrimInfo *primInfo = _delegate->GetPrimInfo(cachePath);
    if (TF_VERIFY(primInfo, "%s", cachePath.GetText())) {
        primInfo->dirtyBits |= dirtyBits;
    }
}

UsdImagingPrimAdapterSharedPtr
UsdImagingIndexProxy::GetMaterialAdapter(UsdPrim const& materialPrim)
{
    if (!TF_VERIFY(!materialPrim.IsInstance())) {
        return nullptr;
    }
    UsdImagingPrimAdapterSharedPtr materialAdapter =
        _delegate->_AdapterLookup(materialPrim, false);
    return materialAdapter &&
           materialAdapter->IsSupported(this) ? materialAdapter : nullptr;
}

bool
UsdImagingIndexProxy::IsPopulated(SdfPath const& cachePath) const
{
    return _delegate->_primInfoMap.find(cachePath) !=
           _delegate->_primInfoMap.end();
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

            index.RemoveRprim(_delegate->GetPathForIndex(*it));
        }
        _rprimsToRemove.clear();

    }

    {
        TRACE_FUNCTION_SCOPE("instancers");
        TF_FOR_ALL(it, _instancersToRemove) {

            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Remove Instancer] <%s>\n",
                                    it->GetText());

            _delegate->_instancerPrimPaths.erase(*it);
            index.RemoveInstancer(_delegate->GetPathForIndex(*it));
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
                              _delegate->GetPathForIndex(cachePath));
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
                              _delegate->GetPathForIndex(cachePath));
        }
        _bprimsToRemove.clear();
    }

    {
        TRACE_FUNCTION_SCOPE("primInfo");
        TF_FOR_ALL(it, _primInfoToRemove) {
            SdfPath cachePath = *it;

            TF_DEBUG(USDIMAGING_CHANGES).Msg("[Remove PrimInfo] <%s>\n",
                                             cachePath.GetText());


            _delegate->_valueCache.Clear(cachePath);
            _delegate->_refineLevelMap.erase(cachePath);
            _delegate->_pickablesMap.erase(cachePath);

            _delegate->_primInfoMap.erase(cachePath);
            _delegate->_usdIds.Remove(cachePath);

            SdfPath indexPath = _delegate->GetPathForIndex(cachePath);
            _delegate->_cache2indexPath.erase(cachePath);
            _delegate->_index2cachePath.erase(indexPath);
        }
        _primInfoToRemove.clear();
    }
}

void
UsdImagingIndexProxy::InsertInstancer(
                             SdfPath const& cachePath,
                             SdfPath const& parentPath,
                             UsdPrim const& usdPrim,
                             UsdImagingPrimAdapterSharedPtr adapter)
{
    _delegate->GetRenderIndex().InsertInstancer(_delegate,
        _delegate->GetPathForIndex(cachePath),
        _delegate->GetPathForIndex(parentPath));

    _delegate->_instancerPrimPaths.insert(cachePath);

    TF_DEBUG(USDIMAGING_INSTANCER).Msg(
        "[Instancer Inserted] %s, parent = %s, adapter = %s\n",
        cachePath.GetText(), parentPath.GetText(),
        adapter ? TfType::GetCanonicalTypeName(typeid(*adapter)).c_str()
                : "none");

    AddPrimInfo(cachePath, usdPrim, adapter);
    _AddTask(cachePath);
}

PXR_NAMESPACE_CLOSE_SCOPE

