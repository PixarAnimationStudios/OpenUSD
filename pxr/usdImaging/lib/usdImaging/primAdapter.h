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
#ifndef USDIMAGING_PRIM_ADAPTER_H
#define USDIMAGING_PRIM_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/version.h"
#include "pxr/usdImaging/usdImaging/valueCache.h"
#include "pxr/usdImaging/usdImaging/inheritedCache.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/usd/usdShade/shader.h"

#include "pxr/base/tf/type.h"

#include <boost/enable_shared_from_this.hpp> 
#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;

// Forward declaration for nested class.
class UsdImagingDelegate;
class UsdImagingIndexProxy;
class UsdImagingInstancerContext;

typedef boost::shared_ptr<class UsdImagingPrimAdapter> 
                                                UsdImagingPrimAdapterSharedPtr;

/// \class UsdImagingPrimAdapter
///
/// Base class for all PrimAdapters.
///
class UsdImagingPrimAdapter 
            : public boost::enable_shared_from_this<UsdImagingPrimAdapter> {
public:
    
    // ---------------------------------------------------------------------- //
    /// \name Initialization
    // ---------------------------------------------------------------------- //
 
    UsdImagingPrimAdapter()
    {}

    virtual ~UsdImagingPrimAdapter();

    /// Called to populate the RenderIndex for this UsdPrim. The adapter is
    /// expected to create one or more Rprims in the render index using the
    /// given proxy.
    virtual SdfPath Populate(UsdPrim const& prim,
                UsdImagingIndexProxy* index,
                UsdImagingInstancerContext const* instancerContext = NULL) = 0;

    // Allows the adapter to prune traversal by culling the children below the
    // given prim.
    virtual bool ShouldCullChildren(UsdPrim const& prim);

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //
 
    /// Prepare local state and \p cache entries for parallel
    /// TrackVariability().
    virtual void TrackVariabilityPrep(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      HdDirtyBits requestedBits,
                                      UsdImagingInstancerContext const* 
                                          instancerContext = NULL) = 0;

    /// For the given \p prim and \p requestedBits, variability is detected and
    /// stored in \p dirtyBits. Initial values are cached into the value cache.
    ///
    /// This method is expected to be called from multiple threads.
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits requestedBits,
                                  HdDirtyBits* dirtyBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL) = 0;

    /// Prepare local state and \p cache entries for parallel UpdateForTime().
    virtual void UpdateForTimePrep(UsdPrim const& prim,
                                   SdfPath const& cachePath, 
                                   UsdTimeCode time,
                                   HdDirtyBits requestedBits,
                                   UsdImagingInstancerContext const* 
                                       instancerContext = NULL) = 0;

    /// Populates the \p cache for the given \p prim, \p time and \p
    /// requestedBits.
    ///
    /// This method is expected to be called from multiple threads.
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               HdDirtyBits* resultBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL) = 0;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    /// Returns a bit mask of attributes to be udpated, or
    /// HdChangeTracker::AllDirty if the entire prim must be resynchronized.
    virtual int ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName) = 0;

    /// When a PrimResync event occurs, the prim may have been deleted entirely,
    /// adapter plug-ins should override this method to free any per-prim state
    /// that was accumulated in the adapter.
    virtual void ProcessPrimResync(SdfPath const& primPath,
                                   UsdImagingIndexProxy* index);

    /// Removes all associated Rprims and dependencies from the render index
    /// without scheduling them for repopulation. 
    virtual void ProcessPrimRemoval(SdfPath const& primPath,
                                   UsdImagingIndexProxy* index);

    // ---------------------------------------------------------------------- //
    /// \name Instancing
    // ---------------------------------------------------------------------- //

    /// Returns the path of the instance prim corresponding to the
    /// instance index generated by the given instanced \p protoPath.
    /// The instancer instances the rprim is derived from \p protoPath,
    /// following the naming convention of each instance adapter.
    /// (i.e. taking PrimPath of attribute path or variant selection path)
    /// If \protoPath is an instancer and also instanced by another parent
    /// instancer, return the instanceCountForThisLevel as the number of
    /// instances.
    virtual SdfPath GetPathForInstanceIndex(
        SdfPath const &protoPath, int instanceIndex,
        int *instanceCountForThisLevel, int *absoluteInstanceIndex,
        SdfPath *resolvedPrimPath=NULL,
        SdfPathVector *resolvedInstanceContext=NULL);

    /// Returns the instancer path for given \p instancePath. If it's not
    /// instanced path, returns empty.
    virtual SdfPath GetInstancer(SdfPath const &instancePath);

    // ---------------------------------------------------------------------- //
    /// \name Nested instancing support
    // ---------------------------------------------------------------------- //

    /// Returns the path of the instance prim corresponding to the
    /// instance index generated by the given instanced \p protoPath on
    /// the \p instancerPath. This method can be used if instancerPath
    /// can't be inferred from protoPath, such as nested instancing.
    /// \p protoPath can be either rprim or child instancer.
    virtual SdfPath GetPathForInstanceIndex(
        SdfPath const &instancerPath, SdfPath const &protoPath,
        int instanceIndex, int *instanceCountForThisLevel,
        int *absoluteInstanceIndex,
        SdfPath *resolvedPrimPath,
        SdfPathVector *resolvedInstanceContext);

    /// Returns the instance index array for \p protoRprimPath, instanced
    /// by \p instancerPath. \p instancerPath must be managed by this
    /// adapter.
    virtual VtIntArray GetInstanceIndices(SdfPath const &instancerPath,
                                          SdfPath const &protoRprimPath);

    /// Returns the transform of \p protoInstancerPath relative to
    /// \p instancerPath. \p instancerPath must be managed by this
    /// adapter.
    virtual GfMatrix4d GetRelativeInstancerTransform(
        SdfPath const &instancerPath,
        SdfPath const &protoInstancerPath,
        UsdTimeCode time);

    // ---------------------------------------------------------------------- //
    /// \name Selection
    // ---------------------------------------------------------------------- //
    virtual bool PopulateSelection(SdfPath const &path,
                                   VtIntArray const &instanceIndices,
                                   HdxSelectionSharedPtr const &result);

    // ---------------------------------------------------------------------- //
    /// \name Utilities 
    // ---------------------------------------------------------------------- //

    /// The root transform provided by the delegate. 
    GfMatrix4d GetRootTransform() const;

    /// A thread-local XformCache provided by the delegate.
    void SetDelegate(UsdImagingDelegate* delegate);

    bool IsChildPath(SdfPath const& path) const;
    
    /// Returns true if the given prim is visible, taking into account inherited
    /// visibility values. Inherited values are strongest, Usd has no notion of
    /// "super vis/invis".
    bool GetVisible(UsdPrim const& prim, UsdTimeCode time);

    /// Fetches the transform for the given prim at the given time from a
    /// pre-computed cache of prim transforms. Requesting transforms at
    /// incoherent times is currently inefficient.
    GfMatrix4d GetTransform(UsdPrim const& prim, UsdTimeCode time,
                            bool ignoreRootTransform = false);

    /// Gets the shader binding for the given prim, walking up namespace if
    /// necessary.  
    SdfPath GetShaderBinding(UsdPrim const& prim);

    /// Gets the instancer ID for the given prim and instancerContext.
    SdfPath GetInstancerBinding(UsdPrim const& prim,
                            UsdImagingInstancerContext const* instancerContext);

    /// Returns the depending rprim paths which don't exist in descendants.
    /// Used for change tracking over subtree boundary (e.g. instancing)
    virtual SdfPathVector GetDependPaths(SdfPath const &path) const;

protected:
    typedef std::vector<UsdImagingValueCache::PrimvarInfo> PrimvarInfoVector;
    typedef UsdImagingValueCache::Key Keys;

    template <typename T>
    T _Get(UsdPrim const& prim, TfToken const& attrToken, UsdTimeCode time) {
        T value;
        prim.GetAttribute(attrToken).Get<T>(&value, time);
        return value;
    }

    template <typename T>
    void _GetPtr(UsdPrim const& prim, TfToken const& key, UsdTimeCode time, T* out) {
        prim.GetAttribute(key).Get<T>(out, time);
    }

    UsdImagingValueCache* _GetValueCache();

    UsdPrim _GetPrim(SdfPath const& usdPath) const;

    // Returns the prim adapter for the given \p prim, or an invalid pointer if
    // no adapter exists. If \p prim is an instance and \p ignoreInstancing
    // is \c true, the instancing adapter will be ignored and an adapter will
    // be looked up based on \p prim's type.
    const UsdImagingPrimAdapterSharedPtr& 
    _GetPrimAdapter(UsdPrim const& prim, bool ignoreInstancing = false);

    // Determines if an attribute is varying and if so, sets the given
    // \p dirtyFlag in the \p dirtyFlags and increments a perf counter. Returns
    // true if the attribute is varying.
    bool _IsVarying(UsdPrim prim, TfToken const& attrName, 
           HdDirtyBits dirtyFlag, TfToken const& perfToken,
           HdDirtyBits* dirtyFlags, bool isInherited);

    // Determines if the prim's transform (CTM) is varying and if so, sets the 
    // given \p dirtyFlag in the \p dirtyFlags and increments a perf counter. 
    // Returns true if the prim's transform is varying.
    bool _IsTransformVarying(UsdPrim prim,
                             HdDirtyBits dirtyFlag,
                             TfToken const& perfToken,
                             HdDirtyBits* dirtyFlags);

    void _MergePrimvar(UsdImagingValueCache::PrimvarInfo const& primvar, 
                       PrimvarInfoVector* vec);

    UsdImagingDelegate* _delegate;
};

class UsdImagingPrimAdapterFactoryBase : public TfType::FactoryBase {
public:
    virtual UsdImagingPrimAdapterSharedPtr New() const = 0;
};

template <class T>
class UsdImagingPrimAdapterFactory : public UsdImagingPrimAdapterFactoryBase {
public:
    virtual UsdImagingPrimAdapterSharedPtr New() const
    {
        return UsdImagingPrimAdapterSharedPtr(new T);
    }
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGING_PRIM_ADAPTER_H
