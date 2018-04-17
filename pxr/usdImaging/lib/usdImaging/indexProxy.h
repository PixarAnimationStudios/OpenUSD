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
#ifndef USDIMAGING_INDEXPROXY_H
#define USDIMAGING_INDEXPROXY_H

/// \file usdImaging/indexProxy.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/delegate.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/base/tf/token.h"


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingIndexProxy
///
/// This proxy class exposes a subset of the private Delegate API to
/// PrimAdapters.
///
class UsdImagingIndexProxy {
public: 
    /// Adds a new prim to be tracked to the delegate.
    /// "cachePath" is the index path minus the delegate prefix (i.e. the result
    /// of GetPathForUsd()).
    /// usdPrim reference the prim to track in usd.
    /// If adapter is null, AddPrimInfo will assign an appropriate adapter based
    /// off the type of the UsdPrim.  However, this can be overridden
    /// (for instancing), by specifying a specific adapter.
    ///
    /// While the cachePath could be obtain from the usdPrim, in the case of
    /// instancing these may differ, so their in an option to specify a specific
    /// cachePath.
    ///
    /// Also for instancing, the function allows the same cachePath to be added
    /// twice without causing an error.  However, the UsdPrim and Adpater have
    /// to be the same as what is already inserted in the tracking.
    USDIMAGING_API
    void AddPrimInfo(SdfPath const& cachePath,
                     UsdPrim const& usdPrim,
                     UsdImagingPrimAdapterSharedPtr const& adapter);

    USDIMAGING_API
    void InsertRprim(TfToken const& primType,
                     SdfPath const& cachePath,
                     SdfPath const& parentPath,
                     UsdPrim const& usdPrim,
                     UsdImagingPrimAdapterSharedPtr adapter =
                        UsdImagingPrimAdapterSharedPtr());

    USDIMAGING_API
    void InsertSprim(TfToken const& primType,
                     SdfPath const& cachePath,
                     UsdPrim const& usdPrim,
                     UsdImagingPrimAdapterSharedPtr adapter =
                        UsdImagingPrimAdapterSharedPtr());

    USDIMAGING_API
    void InsertBprim(TfToken const& primType,
                     SdfPath const& cachePath,
                     UsdPrim const& usdPrim,
                     UsdImagingPrimAdapterSharedPtr adapter =
                        UsdImagingPrimAdapterSharedPtr());

    // Inserts an instancer into the HdRenderIndex and schedules it for updates
    // from the delegate.
    USDIMAGING_API
    void InsertInstancer(SdfPath const& cachePath,
                         SdfPath const& parentPath,
                         UsdPrim const& usdPrim,
                         UsdImagingPrimAdapterSharedPtr adapter =
                            UsdImagingPrimAdapterSharedPtr());

    // Refresh the prim at the specified render index path.
    USDIMAGING_API
    void Refresh(SdfPath const& cachePath);

    // Refresh the HdInstancer at the specified render index path.
    USDIMAGING_API
    void RefreshInstancer(SdfPath const& instancerPath);

    //
    // All removals are deferred to avoid surprises during change processing.
    //
    
    // Designates that the given prim should no longer be tracked and thus
    // removed from the tracking structure.
    void RemovePrimInfo(SdfPath const& cachePath) {
        _primInfoToRemove.push_back(cachePath);
    }

    // Removes the Rprim at the specified cache path.
    void RemoveRprim(SdfPath const& cachePath) { 
        _rprimsToRemove.push_back(cachePath);
    }

     // Removes the Sprim at the specified cache path.
     void RemoveSprim(TfToken const& primType, SdfPath const& cachePath) {
         _TypeAndPath primToRemove = {primType, cachePath};
         _sprimsToRemove.push_back(primToRemove);
     }

     // Removes the Bprim at the specified render index path.
     void RemoveBprim(TfToken const& primType, SdfPath const& cachePath) {
         _TypeAndPath primToRemove = {primType, cachePath};
         _bprimsToRemove.push_back(primToRemove);
     }


    // Removes the HdInstancer at the specified render index path.
    void RemoveInstancer(SdfPath const& instancerPath) { 
        _instancersToRemove.push_back(instancerPath);
    }

    USDIMAGING_API
    bool HasRprim(SdfPath const &cachePath);

    USDIMAGING_API
    void MarkRprimDirty(SdfPath const& cachePath, HdDirtyBits dirtyBits);

    USDIMAGING_API
    void MarkSprimDirty(SdfPath const& cachePath, HdDirtyBits dirtyBits);

    USDIMAGING_API
    void MarkBprimDirty(SdfPath const& cachePath, HdDirtyBits dirtyBits);

    USDIMAGING_API
    void MarkInstancerDirty(SdfPath const& cachePath, HdDirtyBits dirtyBits);

    USDIMAGING_API
    bool IsRprimTypeSupported(TfToken const& typeId) const;

    USDIMAGING_API
    bool IsSprimTypeSupported(TfToken const& typeId) const;

    USDIMAGING_API
    bool IsBprimTypeSupported(TfToken const& typeId) const;

    // Check if the given path has been populated yet.
    USDIMAGING_API
    bool IsPopulated(SdfPath const& cachePath) const;

    // Recursively repopulate the specified usdPath into the render index.
    USDIMAGING_API
    void Repopulate(SdfPath const& usdPath);

    USDIMAGING_API
    UsdImagingPrimAdapterSharedPtr GetMaterialAdapter(
        UsdPrim const& materialPrim);

private:
    friend class UsdImagingDelegate;
    UsdImagingIndexProxy(UsdImagingDelegate* delegate,
                            UsdImagingDelegate::_Worker* worker) 
        : _delegate(delegate)
        , _worker(worker)
    {}

    SdfPathVector const& _GetPathsToRepopulate() { return _pathsToRepopulate; }
    void _ProcessRemovals();

    void _AddTask(SdfPath const& usdPath);   

    struct _TypeAndPath {
        TfToken primType;
        SdfPath cachePath;
    };

    typedef std::vector<_TypeAndPath> _TypeAndPathVector;

    UsdImagingDelegate* _delegate;
    UsdImagingDelegate::_Worker* _worker;
    SdfPathVector _pathsToRepopulate;
    SdfPathVector _rprimsToRemove;
    _TypeAndPathVector _sprimsToRemove;
    _TypeAndPathVector _bprimsToRemove;
    SdfPathVector _instancersToRemove;
    SdfPathVector _primInfoToRemove;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //USDIMAGING_INDEXPROXY_H
