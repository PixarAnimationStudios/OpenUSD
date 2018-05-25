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

/// \file usdImaging/primAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/version.h"
#include "pxr/usdImaging/usdImaging/collectionCache.h"
#include "pxr/usdImaging/usdImaging/valueCache.h"
#include "pxr/usdImaging/usdImaging/inheritedCache.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/texture.h"
#include "pxr/imaging/hd/selection.h"
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

    USDIMAGING_API
    virtual ~UsdImagingPrimAdapter();

    /// Called to populate the RenderIndex for this UsdPrim. The adapter is
    /// expected to create one or more Rprims in the render index using the
    /// given proxy.
    virtual SdfPath Populate(UsdPrim const& prim,
                UsdImagingIndexProxy* index,
                UsdImagingInstancerContext const* instancerContext = NULL) = 0;

    // Allows the adapter to prune traversal by culling the children below the
    // given prim.
    USDIMAGING_API
    virtual bool ShouldCullChildren(UsdPrim const& prim);

    // Indicates the adapter is a multiplexing adapter (e.g. PointInstancer),
    // potentially managing its children. This flag is used in nested
    // instancer cases to determine which adapter is assigned to which prim.
    USDIMAGING_API
    virtual bool IsInstancerAdapter();

    // Indicates whether this adapter can populate a master prim. By policy,
    // you can't directly instance a gprim, but you can directly instance proxy
    // objects (like cards). Note: masters don't have attributes, so an adapter
    // opting in here needs to check if prims it's populating are master prims,
    // and if so find a copy of the instancing prim.
    virtual bool CanPopulateMaster() { return false; }

    // Indicates that this adapter populates the render index only when
    // directed by the population of another prim, e.g. materials are
    // populated on behalf of prims which use the material.
    USDIMAGING_API
    virtual bool IsPopulatedIndirectly();

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //
 
    /// Prepare local state and \p cache entries for parallel
    /// TrackVariability().
    virtual void TrackVariabilityPrep(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingInstancerContext const* 
                                          instancerContext = NULL) {};

    /// For the given \p prim, variability is detected and
    /// stored in \p timeVaryingBits. Initial values are cached into the value
    /// cache.
    ///
    /// This method is expected to be called from multiple threads.
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = NULL) const = 0;

    /// Prepare local state and \p cache entries for parallel UpdateForTime().
    virtual void UpdateForTimePrep(UsdPrim const& prim,
                                   SdfPath const& cachePath, 
                                   UsdTimeCode time,
                                   HdDirtyBits requestedBits,
                                   UsdImagingInstancerContext const* 
                                       instancerContext = NULL) {};

    /// Populates the \p cache for the given \p prim, \p time and \p
    /// requestedBits.
    ///
    /// This method is expected to be called from multiple threads.
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = NULL) const = 0;

    // ---------------------------------------------------------------------- //
    /// \name Change Processing 
    // ---------------------------------------------------------------------- //

    /// Returns a bit mask of attributes to be updated, or
    /// HdChangeTracker::AllDirty if the entire prim must be resynchronized.
    ///
    /// \p changedFields contains a list of changed scene description fields
    /// for this prim. This may be empty in certain cases, like the addition
    /// of an inert prim spec for the given \p prim.
    ///
    /// The default implementation returns HdChangeTracker::AllDirty if any of
    /// the changed fields are plugin metadata fields, HdChangeTracker::Clean
    /// otherwise.
    USDIMAGING_API
    virtual HdDirtyBits ProcessPrimChange(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          TfTokenVector const& changedFields);

    /// Returns a bit mask of attributes to be updated, or
    /// HdChangeTracker::AllDirty if the entire prim must be resynchronized.
    virtual HdDirtyBits ProcessPropertyChange(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              TfToken const& propertyName) = 0;

    /// When a PrimResync event occurs, the prim may have been deleted entirely,
    /// adapter plug-ins should override this method to free any per-prim state
    /// that was accumulated in the adapter.
    USDIMAGING_API
    virtual void ProcessPrimResync(SdfPath const& primPath,
                                   UsdImagingIndexProxy* index);

    /// Removes all associated Rprims and dependencies from the render index
    /// without scheduling them for repopulation. 
    USDIMAGING_API
    virtual void ProcessPrimRemoval(SdfPath const& primPath,
                                   UsdImagingIndexProxy* index);


    virtual void MarkDirty(UsdPrim const& prim,
                           SdfPath const& cachePath,
                           HdDirtyBits dirty,
                           UsdImagingIndexProxy* index) = 0;

    USDIMAGING_API
    virtual void MarkRefineLevelDirty(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingIndexProxy* index);

    USDIMAGING_API
    virtual void MarkReprDirty(UsdPrim const& prim,
                               SdfPath const& cachePath,
                               UsdImagingIndexProxy* index);

    USDIMAGING_API
    virtual void MarkCullStyleDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index);

    USDIMAGING_API
    virtual void MarkTransformDirty(UsdPrim const& prim,
                                    SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index);

    USDIMAGING_API
    virtual void MarkVisibilityDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
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
    USDIMAGING_API
    virtual SdfPath GetPathForInstanceIndex(
        SdfPath const &protoPath, int instanceIndex,
        int *instanceCountForThisLevel, int *absoluteInstanceIndex,
        SdfPath *resolvedPrimPath=NULL,
        SdfPathVector *resolvedInstanceContext=NULL);

    /// Returns the instancer path for given \p instancePath. If it's not
    /// instanced path, returns empty.
    USDIMAGING_API
    virtual SdfPath GetInstancer(SdfPath const &instancePath);

    /// Sample the instancer transform for the given prim.
    /// \see HdSceneDelegate::SampleInstancerTransform()
    USDIMAGING_API
    virtual size_t
    SampleInstancerTransform(UsdPrim const& instancerPrim,
                             SdfPath const& instancerPath,
                             UsdTimeCode time,
                             const std::vector<float>& configuredSampleTimes,
                             size_t maxSampleCount,
                             float *times,
                             GfMatrix4d *samples);

    /// Sample the primvar for the given prim.
    /// \see HdSceneDelegate::SamplePrimvar()
    USDIMAGING_API
    virtual size_t
    SamplePrimvar(UsdPrim const& usdPrim,
                  SdfPath const& cachePath,
                  TfToken const& key,
                  UsdTimeCode time,
                  const std::vector<float>& configuredSampleTimes,
                  size_t maxNumSamples, float *times,
                  VtValue *samples);

    // ---------------------------------------------------------------------- //
    /// \name Nested instancing support
    // ---------------------------------------------------------------------- //

    /// Returns the path of the instance prim corresponding to the
    /// instance index generated by the given instanced \p protoPath on
    /// the \p instancerPath. This method can be used if instancerPath
    /// can't be inferred from protoPath, such as nested instancing.
    /// \p protoPath can be either rprim or child instancer.
    USDIMAGING_API
    virtual SdfPath GetPathForInstanceIndex(
        SdfPath const &instancerPath, SdfPath const &protoPath,
        int instanceIndex, int *instanceCountForThisLevel,
        int *absoluteInstanceIndex,
        SdfPath *resolvedPrimPath,
        SdfPathVector *resolvedInstanceContext);

    /// Returns the instance index array for \p protoRprimPath, instanced
    /// by \p instancerPath. \p instancerPath must be managed by this
    /// adapter.
    USDIMAGING_API
    virtual VtIntArray GetInstanceIndices(SdfPath const &instancerPath,
                                          SdfPath const &protoRprimPath);

    /// Returns the transform of \p protoInstancerPath relative to
    /// \p instancerPath. \p instancerPath must be managed by this
    /// adapter.
    USDIMAGING_API
    virtual GfMatrix4d GetRelativeInstancerTransform(
        SdfPath const &instancerPath,
        SdfPath const &protoInstancerPath,
        UsdTimeCode time) const;

    // ---------------------------------------------------------------------- //
    /// \name Selection
    // ---------------------------------------------------------------------- //
    USDIMAGING_API
    virtual bool PopulateSelection(HdSelection::HighlightMode const& highlightMode,
                                   SdfPath const &usdPath,
                                   VtIntArray const &instanceIndices,
                                   HdSelectionSharedPtr const &result);

    // ---------------------------------------------------------------------- //
    /// \name Texture resources
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    virtual HdTextureResource::ID
    GetTextureResourceID(UsdPrim const& usdPrim, SdfPath const &id,
                         UsdTimeCode time, size_t salt) const;

    USDIMAGING_API
    virtual HdTextureResourceSharedPtr
    GetTextureResource(UsdPrim const& usdPrim, SdfPath const &id,
                       UsdTimeCode time) const;

    // ---------------------------------------------------------------------- //
    /// \name Utilities 
    // ---------------------------------------------------------------------- //

    /// The root transform provided by the delegate. 
    USDIMAGING_API
    GfMatrix4d GetRootTransform() const;

    /// A thread-local XformCache provided by the delegate.
    USDIMAGING_API
    void SetDelegate(UsdImagingDelegate* delegate);

    USDIMAGING_API
    bool IsChildPath(SdfPath const& path) const;
    
    /// Returns true if the given prim is visible, taking into account inherited
    /// visibility values. Inherited values are strongest, Usd has no notion of
    /// "super vis/invis".
    USDIMAGING_API
    bool GetVisible(UsdPrim const& prim, UsdTimeCode time) const;

    /// Fetches the transform for the given prim at the given time from a
    /// pre-computed cache of prim transforms. Requesting transforms at
    /// incoherent times is currently inefficient.
    USDIMAGING_API
    GfMatrix4d GetTransform(UsdPrim const& prim, UsdTimeCode time,
                            bool ignoreRootTransform = false) const;

    /// Gets the material path for the given prim, walking up namespace if
    /// necessary.  
    USDIMAGING_API
    SdfPath GetMaterialId(UsdPrim const& prim) const; 

    /// Gets the instancer ID for the given prim and instancerContext.
    USDIMAGING_API
    SdfPath GetInstancerBinding(UsdPrim const& prim,
                            UsdImagingInstancerContext const* instancerContext);

    /// Returns the depending rprim paths which don't exist in descendants.
    /// Used for change tracking over subtree boundary (e.g. instancing)
    USDIMAGING_API
    virtual SdfPathVector GetDependPaths(SdfPath const &path) const;

    /// Gets the model:drawMode attribute for the given prim, walking up
    /// the namespace if necessary.
    USDIMAGING_API
    TfToken GetModelDrawMode(UsdPrim const& prim);

    // ---------------------------------------------------------------------- //
    /// \name Render Index Compatibility
    // ---------------------------------------------------------------------- //

    /// Returns true if the adapter can be populated into the target index.
    virtual bool IsSupported(UsdImagingIndexProxy const* index) const {
        return true;
    }

protected:
    typedef UsdImagingValueCache::Key Keys;

    template <typename T>
    T _Get(UsdPrim const& prim, TfToken const& attrToken, 
           UsdTimeCode time) const {
        T value;
        prim.GetAttribute(attrToken).Get<T>(&value, time);
        return value;
    }

    template <typename T>
    void _GetPtr(UsdPrim const& prim, TfToken const& key, UsdTimeCode time,
                 T* out) const {
        prim.GetAttribute(key).Get<T>(out, time);
    }

    USDIMAGING_API
    UsdImagingValueCache* _GetValueCache() const;

    USDIMAGING_API
    UsdPrim _GetPrim(SdfPath const& usdPath) const;

    // Returns the prim adapter for the given \p prim, or an invalid pointer if
    // no adapter exists. If \p prim is an instance and \p ignoreInstancing
    // is \c true, the instancing adapter will be ignored and an adapter will
    // be looked up based on \p prim's type.
    USDIMAGING_API
    const UsdImagingPrimAdapterSharedPtr& 
    _GetPrimAdapter(UsdPrim const& prim, bool ignoreInstancing = false) const;

    // XXX: Transitional API
    // Returns the instance proxy prim path for a USD-instanced prim, given the
    // instance chain leading to that prim. The paths are sorted from more to
    // less local; the first path is the prim path (possibly in master), then
    // instance paths (possibly in master); the last path is the prim or
    // instance path in the scene.
    USDIMAGING_API
    SdfPath _GetPrimPathFromInstancerChain(SdfPathVector const& instancerChain);

    UsdTimeCode _GetTimeWithOffset(float offset) const;

    // Converts \p stagePath to the path in the render index
    SdfPath _GetPathForIndex(SdfPath const& usdPath) const;

    // Returns the rprim paths in the renderIndex rooted at \p indexPath.
    SdfPathVector _GetRprimSubtree(SdfPath const& indexPath) const;

    // Returns whether or not the render delegate can handle material networks.
    bool _CanComputeMaterialNetworks() const;

    // Returns \c true if \p usdPath is included in the scene delegate's
    // invised path list.
    bool _IsInInvisedPaths(SdfPath const& usdPath) const;

    // Determines if an attribute is varying and if so, sets the given
    // \p dirtyFlag in the \p dirtyFlags and increments a perf counter. Returns
    // true if the attribute is varying.
    USDIMAGING_API
    bool _IsVarying(UsdPrim prim, TfToken const& attrName, 
           HdDirtyBits dirtyFlag, TfToken const& perfToken,
           HdDirtyBits* dirtyFlags, bool isInherited) const;

    // Returns whether or not the rprim at \p cachePath is refined.
    bool _IsRefined(SdfPath const& cachePath) const;

    // Determines if the prim's transform (CTM) is varying and if so, sets the 
    // given \p dirtyFlag in the \p dirtyFlags and increments a perf counter. 
    // Returns true if the prim's transform is varying.
    USDIMAGING_API
    bool _IsTransformVarying(UsdPrim prim,
                             HdDirtyBits dirtyFlag,
                             TfToken const& perfToken,
                             HdDirtyBits* dirtyFlags) const;

    // Convenience method for adding or updating a primvar descriptor.
    // Role defaults to empty token (none).
    USDIMAGING_API
    void _MergePrimvar(
        HdPrimvarDescriptorVector* vec,
        TfToken const& name,
        HdInterpolation interp,
        TfToken const& role = TfToken()) const;

    virtual void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) = 0;

    UsdImaging_CollectionCache& _GetCollectionCache() const;

private:

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
