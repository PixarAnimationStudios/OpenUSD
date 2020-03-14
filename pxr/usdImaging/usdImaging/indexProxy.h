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
#ifndef PXR_USD_IMAGING_USD_IMAGING_INDEX_PROXY_H
#define PXR_USD_IMAGING_USD_IMAGING_INDEX_PROXY_H

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
    /// A note on paths/prims: the core function of UsdImagingIndexProxy and
    /// UsdImagingDelegate is to maintain a set of mappings between USD prims
    /// and hydra prims (and a set of adapters that know how to translate
    /// USD properties to hydra buffers).  A USD prim can represent multiple
    /// hydra prims (e.g. point instancer prototypes that are referenced twice),
    /// and a hydra prim can represent multiple USD prims (e.g. a single hydra
    /// instancer prim representing multiple native instances).
    /// 
    /// There are three different prim "namespaces" that the delegate works
    /// with: "USD paths", which represent paths to USD prims; "index paths",
    /// which represent paths to hydra prims in the render index; and
    /// "cache paths", which represent paths to buffers in the value cache
    /// backing hydra prims.  Cache paths and index paths are the same,
    /// except that index paths have the "delegateID" prefixed onto their path.
    ///
    /// Index paths are only used in the interface to hydra.  In IndexProxy
    /// and the adapters, the pattern is to use "cachePath" as a key to look
    /// up state for a hydra prim; and pass "usdPrim" when we additionally
    /// need to specify the related USD prim.  The naming helps clarify them,
    /// and the fact that we pass a UsdPrim and not an SdfPath further ensures
    /// that we pass valid USD paths instead of passing cache paths by mistake.


    /// Adds a dependency from the given usdPrim to the given cache path.
    /// Insert* will automatically add a dependency, so this is for hydra prims
    /// that may depend on more than one usd prim (e.g. subsets, instancers)
    USDIMAGING_API
    void AddDependency(SdfPath const& cachePath,
                       UsdPrim const& usdPrim);

    /// Insert a hydra prim with the specified cache path.  As mentioned above,
    /// the delegateID will be prepended before adding the prim in hydra, but
    /// cachePath will be the key into all internal usdImaging datastructures.
    /// usdPrim is the USD prim this hydra prim is representing (e.g. the
    /// UsdGeomMesh for which we are inserting a hydra mesh).  If no
    /// adapter is specified, UsdImagingDelegate will choose based on Usd
    /// prim type; some clients (e.g. instancing) want to override the adapter
    /// choice but this should be used sparingly.
    ///
    /// For Rprims and Instancers, "parentPath" is the parent instancer.
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

    USDIMAGING_API
    void InsertInstancer(SdfPath const& cachePath,
                         SdfPath const& parentPath,
                         UsdPrim const& usdPrim,
                         UsdImagingPrimAdapterSharedPtr adapter =
                            UsdImagingPrimAdapterSharedPtr());

    // Mark a prim as needing follow-up work by the delegate
    // (e.g. TrackVariability); this is automatically called on Insert*, but
    // needs to manually be called in some special cases like native
    // instancer population.
    USDIMAGING_API
    void Refresh(SdfPath const& cachePath);

    //
    // All removals are deferred to avoid surprises during change processing.
    //
    
    // Removes the Rprim at the specified cache path.
    void RemoveRprim(SdfPath const& cachePath) {
        _rprimsToRemove.push_back(cachePath);
        _hdPrimInfoToRemove.push_back(cachePath);
        _RemoveDependencies(cachePath);
    }

    // Removes the Sprim at the specified cache path.
    void RemoveSprim(TfToken const& primType, SdfPath const& cachePath) {
        _TypeAndPath primToRemove = {primType, cachePath};
        _sprimsToRemove.push_back(primToRemove);
        _hdPrimInfoToRemove.push_back(cachePath);
        _RemoveDependencies(cachePath);
    }

    // Removes the Bprim at the specified cache path.
    void RemoveBprim(TfToken const& primType, SdfPath const& cachePath) {
        _TypeAndPath primToRemove = {primType, cachePath};
        _bprimsToRemove.push_back(primToRemove);
        _hdPrimInfoToRemove.push_back(cachePath);
        _RemoveDependencies(cachePath);
    }

    // Removes the HdInstancer at the specified cache path.
    void RemoveInstancer(SdfPath const& cachePath) {
        _instancersToRemove.push_back(cachePath);
        _hdPrimInfoToRemove.push_back(cachePath);
        _RemoveDependencies(cachePath);
    }

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

    // Sort and de-duplicate "repopulate" paths to prevent double-inserts.
    // Called by UsdImagingDelegate::ApplyPendingUpdates.
    void _UniqueifyPathsToRepopulate();

    bool _AddHdPrimInfo(SdfPath const& cachePath,
                        UsdPrim const& usdPrim,
                        UsdImagingPrimAdapterSharedPtr const& adapter);

    // XXX: Workaround for some bugs in USD edit processing, and weird uses
    // of HdPrimInfo by instanced prims.  Remove the dependency between
    // a hydra prim and whatever USD prim is in its primInfo.
    friend class UsdImagingGprimAdapter;
    void _RemovePrimInfoDependency(SdfPath const& cachePath);

    USDIMAGING_API
    void _RemoveDependencies(SdfPath const& cachePath);

    SdfPathVector const& _GetUsdPathsToRepopulate() {
        return _usdPathsToRepopulate;
    }
    void _ProcessRemovals();

    void _AddTask(SdfPath const& usdPath);   

    struct _TypeAndPath {
        TfToken primType;
        SdfPath cachePath;
    };

    typedef std::vector<_TypeAndPath> _TypeAndPathVector;

    typedef std::vector<UsdImagingDelegate::_DependencyMap::value_type>
        _DependencyVector;

    UsdImagingDelegate* _delegate;
    UsdImagingDelegate::_Worker* _worker;
    SdfPathVector _usdPathsToRepopulate;
    SdfPathVector _rprimsToRemove;
    _TypeAndPathVector _sprimsToRemove;
    _TypeAndPathVector _bprimsToRemove;
    SdfPathVector _instancersToRemove;
    SdfPathVector _hdPrimInfoToRemove;
    _DependencyVector _dependenciesToRemove;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_IMAGING_USD_IMAGING_INDEX_PROXY_H
