//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_PRIM_ADAPTER_H
#define PXR_USD_IMAGING_USD_IMAGING_PRIM_ADAPTER_H

/// \file usdImaging/primAdapter.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/version.h"
#include "pxr/usdImaging/usdImaging/collectionCache.h"
#include "pxr/usdImaging/usdImaging/primvarDescCache.h"
#include "pxr/usdImaging/usdImaging/resolvedAttributeCache.h"
#include "pxr/usdImaging/usdImaging/types.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include "pxr/usd/usdShade/shader.h"

#include "pxr/base/tf/type.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class UsdPrim;

// Forward declaration for nested class.
class UsdImagingDelegate;
class UsdImagingIndexProxy;
class UsdImagingInstancerContext;
class HdExtComputationContext;

class UsdImagingDataSourceStageGlobals;

using UsdImagingPrimAdapterSharedPtr = 
    std::shared_ptr<class UsdImagingPrimAdapter>;

/// \class UsdImagingPrimAdapter
///
/// Base class for all PrimAdapters.
///
class UsdImagingPrimAdapter 
  : public std::enable_shared_from_this<UsdImagingPrimAdapter>
{
public:
    USDIMAGING_API
    virtual ~UsdImagingPrimAdapter() = default;

    // ---------------------------------------------------------------------- //
    /// \name Scene Index Support
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    virtual TfTokenVector GetImagingSubprims(UsdPrim const& prim);

    USDIMAGING_API
    virtual TfToken GetImagingSubprimType(
        UsdPrim const& prim, TfToken const& subprim);

    USDIMAGING_API
    virtual HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

    USDIMAGING_API
    virtual HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType);

    /// \enum Scope
    ///
    /// Determines what USD prims an adapter type is responsible for from a
    /// population and invalidation standpoint.
    ///
    enum PopulationMode
    {
        /// The adapter is responsible only for USD prims of its registered
        /// type. Any descendent USD prims are managed independently.
        RepresentsSelf,

        /// The adapter is responsible for USD prims of its registered type as
        /// well as any descendents of those prims. No population occurs for
        /// descendent prims. USD changes to descendent prims whose own PopulationMode
        /// is set to RepresentedByAncestor will be send to this adapter.
        RepresentsSelfAndDescendents,

        /// Changes to prims of this adapter's registered type are sent to the
        /// first ancestor prim whose adapter's PopulationMode value is 
        /// RepresentsSelfAndDescendents.
        ///
        /// This value alone does not prevent population as it is expected that
        /// such prims appear beneath another prim whose own PopulationMode value
        /// prevents descendents from being populated.
        RepresentedByAncestor,
    };

    /// Returns the prim's behavior with regard to population and invalidation.
    /// See PopulationMode for possible values.
    USDIMAGING_API
    virtual PopulationMode GetPopulationMode();

    /// This is called (for each result of GetImagingSubprims) when this
    /// adapter's GetScope() result is RepresentsSelfAndDescendents and
    /// USD properties have changed on a descendent prim whose adapter's
    /// GetScope() result is RepresentedByAncestor.
    USDIMAGING_API
    virtual HdDataSourceLocatorSet InvalidateImagingSubprimFromDescendent(
            UsdPrim const& prim,
            UsdPrim const& descendentPrim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType);

    // ---------------------------------------------------------------------- //
    /// \name Initialization
    // ---------------------------------------------------------------------- //
 
    /// Called to populate the RenderIndex for this UsdPrim. The adapter is
    /// expected to create one or more prims in the render index using the
    /// given proxy.
    virtual SdfPath Populate(UsdPrim const& prim,
                             UsdImagingIndexProxy* index,
                             UsdImagingInstancerContext const*
                                 instancerContext = nullptr) = 0;

    // Indicates whether population traversal should be pruned based on
    // prim-specific features (like whether it's imageable).
    USDIMAGING_API
    static bool ShouldCullSubtree(UsdPrim const& prim);

    // Indicates whether population traversal should be pruned based on
    // adapter-specific features (like whether the adapter is an instance
    // adapter, and wants to do its own population).
    USDIMAGING_API
    virtual bool ShouldCullChildren() const;

    // Indicates whether or not native USD prim instancing should be ignored
    // for prims using this delegate, along with their descendants.
    USDIMAGING_API
    virtual bool ShouldIgnoreNativeInstanceSubtrees() const;

    // Indicates the adapter is a multiplexing adapter (e.g. PointInstancer),
    // potentially managing its children. This flag is used in nested
    // instancer cases to determine which adapter is assigned to which prim.
    USDIMAGING_API
    virtual bool IsInstancerAdapter() const;

    // Indicates whether this adapter can directly populate USD instance prims.
    //
    // Normally, with USD instances, we make a firewall between the instance
    // prim and the USD prototype tree. The instance adapter creates one
    // hydra prototype per prim in the USD prototype tree, shared by all USD
    // instances; this lets us recognize the benefits of instancing,
    // by hopefully having a high instance count per prototype.  The instance
    // adapter additionally configures a hydra instancer for the prototype tree;
    // and a small set of specially-handled data is allowed through: things like
    // inherited constant primvars, transforms, visibility, and other things
    // we know how to vary per-instance.
    //
    // We enforce the above policy by refusing to populate gprims which are
    // USD instances, since we'd need one prototype per instance and would lose
    // any instancing benefit.
    //
    // There are a handful of times when it really is useful to directly
    // populate instance prims: for example, instances with cards applied,
    // or instances of type UsdSkelRoot.  In those cases, the adapters can
    // opt into this scheme with "CanPopulateUsdInstance".
    //
    // Note that any adapters taking advantage of this feature will need
    // extensive code support in instanceAdapter: the instance adapter will
    // need to potentially create and track  multiple hydra prototypes per
    // USD prototype, and the adapter will need special handling to pass down
    // any relevant instance-varying data.
    //
    // In summary: use with caution.
    USDIMAGING_API
    virtual bool CanPopulateUsdInstance() const;

    // ---------------------------------------------------------------------- //
    /// \name Parallel Setup and Resolve
    // ---------------------------------------------------------------------- //
 
    /// For the given \p prim, variability is detected and
    /// stored in \p timeVaryingBits. Initial values are cached into the value
    /// cache.
    ///
    /// This method is expected to be called from multiple threads.
    virtual void TrackVariability(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits* timeVaryingBits,
                                  UsdImagingInstancerContext const* 
                                      instancerContext = nullptr) const = 0;

    /// Populates the \p cache for the given \p prim, \p time and \p
    /// requestedBits.
    ///
    /// This method is expected to be called from multiple threads.
    virtual void UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext = nullptr) const = 0;

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
    virtual void ProcessPrimResync(SdfPath const& cachePath,
                                   UsdImagingIndexProxy* index);

    /// Removes all associated Rprims and dependencies from the render index
    /// without scheduling them for repopulation. 
    USDIMAGING_API
    virtual void ProcessPrimRemoval(SdfPath const& cachePath,
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
    virtual void MarkRenderTagDirty(UsdPrim const& prim,
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

    USDIMAGING_API
    virtual void MarkMaterialDirty(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdImagingIndexProxy* index);

    USDIMAGING_API
    virtual void MarkLightParamsDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdImagingIndexProxy* index);

    USDIMAGING_API
    virtual void MarkWindowPolicyDirty(UsdPrim const& prim,
                                       SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index);

    USDIMAGING_API
    virtual void MarkCollectionsDirty(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingIndexProxy* index);

    // ---------------------------------------------------------------------- //
    /// \name Computations 
    // ---------------------------------------------------------------------- //
    USDIMAGING_API
    virtual void InvokeComputation(SdfPath const& cachePath,
                                   HdExtComputationContext* context);

    // ---------------------------------------------------------------------- //
    /// \name Instancing
    // ---------------------------------------------------------------------- //

    /// Return an array of the categories used by each instance.
    USDIMAGING_API
    virtual std::vector<VtArray<TfToken>>
    GetInstanceCategories(UsdPrim const& prim);

    /// Get the instancer transform for the given prim.
    /// \see HdSceneDelegate::GetInstancerTransform()
    USDIMAGING_API
    virtual GfMatrix4d GetInstancerTransform(
        UsdPrim const& instancerPrim,
        SdfPath const& instancerPath,
        UsdTimeCode time) const;

    /// Sample the instancer transform for the given prim.
    /// \see HdSceneDelegate::SampleInstancerTransform()
    USDIMAGING_API
    virtual size_t SampleInstancerTransform(
        UsdPrim const& instancerPrim,
        SdfPath const& instancerPath,
        UsdTimeCode time,
        size_t maxNumSamples,
        float *sampleTimes,
        GfMatrix4d *sampleValues);

    /// Return the instancerId for this prim.
    USDIMAGING_API
    virtual SdfPath GetInstancerId(
        UsdPrim const& usdPrim,
        SdfPath const& cachePath) const;

    /// Return the list of known prototypes of this prim.
    USDIMAGING_API
    virtual SdfPathVector GetInstancerPrototypes(
        UsdPrim const& usdPrim,
        SdfPath const& cachePath) const;

    /// Sample the primvar for the given prim. If *sampleIndices is not nullptr 
    /// and the primvar has indices, it will sample the unflattened primvar and 
    /// set *sampleIndices to the primvar's sampled indices.
    /// \see HdSceneDelegate::SamplePrimvar() and 
    /// HdSceneDelegate::SampleIndexedPrimvar()
    USDIMAGING_API
    virtual size_t
    SamplePrimvar(UsdPrim const& usdPrim,
                  SdfPath const& cachePath,
                  TfToken const& key,
                  UsdTimeCode time,
                  size_t maxNumSamples, 
                  float *sampleTimes,
                  VtValue *sampleValues,
                  VtIntArray *sampleIndices);

    /// Get the subdiv tags for this prim.
    USDIMAGING_API
    virtual PxOsdSubdivTags GetSubdivTags(UsdPrim const& usdPrim,
                                          SdfPath const& cachePath,
                                          UsdTimeCode time) const;

    // ---------------------------------------------------------------------- //
    /// \name Nested instancing support
    // ---------------------------------------------------------------------- //

    // NOTE: This method is currently only used by PointInstancer
    // style instances, and not instanceable-references.

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

    /// \deprecated Call and implement GetScenePrimPaths instead.
    USDIMAGING_API
    virtual SdfPath GetScenePrimPath(SdfPath const& cachePath,
        int instanceIndex,
        HdInstancerContext *instancerCtx) const;

    USDIMAGING_API
    virtual SdfPathVector GetScenePrimPaths(SdfPath const& cachePath,
        std::vector<int> const& instanceIndices,
        std::vector<HdInstancerContext> *instancerCtxs) const;

    // Add the given usdPrim to the HdSelection object, to mark it for
    // selection highlighting. cachePath is the path of the object referencing
    // this adapter.
    //
    // If an instance index is provided to Delegate::PopulateSelection, it's
    // interpreted as a hydra instance index and left unchanged (to make
    // picking/selection round-tripping work).  Otherwise, instance adapters
    // will build up a composite instance index range at each level.
    //
    // Consider:
    //   /World/A (2 instances)
    //           /B (2 instances)
    //             /C (gprim)
    // ... to select /World/A, instance 0, you want to select cartesian
    // coordinates (0, *) -> (0, 0) and (0, 1).  The flattened representation
    // of this is:
    //   index = coordinate[0] * instance_count[1] + coordinate[1]
    // Likewise, for one more nesting level you get:
    //   index = c[0] * count[1] * count[2] + c[1] * count[2] + c[2]
    // ... since the adapter for /World/A has no idea what count[1+] are,
    // this needs to be built up.  The delegate initially sets
    // parentInstanceIndices to [].  /World/A sets this to [0].  /World/A/B,
    // since it is selecting *, adds all possible instance indices:
    // 0 * 2 + 0 = 0, 0 * 2 + 1 = 1. /World/A/B/C is a gprim, and adds
    // instances [0,1] to its selection.
    USDIMAGING_API
    virtual bool PopulateSelection(
        HdSelection::HighlightMode const& highlightMode,
        SdfPath const &cachePath,
        UsdPrim const &usdPrim,
        int const hydraInstanceIndex,
        VtIntArray const &parentInstanceIndices,
        HdSelectionSharedPtr const &result) const;

    // ---------------------------------------------------------------------- //
    /// \name Volume field information
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    virtual HdVolumeFieldDescriptorVector
    GetVolumeFieldDescriptors(UsdPrim const& usdPrim, SdfPath const &id,
                              UsdTimeCode time) const;

    // ---------------------------------------------------------------------- //
    /// \name Light Params
    // ---------------------------------------------------------------------- //

    USDIMAGING_API
    virtual VtValue
    GetLightParamValue(
        const UsdPrim& prim,
        const SdfPath& cachePath,
        const TfToken& paramName,
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
    virtual bool IsChildPath(const SdfPath& path) const;
    
    /// Returns true if the given prim is visible, taking into account inherited
    /// visibility values. Inherited values are strongest, Usd has no notion of
    /// "super vis/invis".
    USDIMAGING_API
    virtual bool GetVisible(
        UsdPrim const& prim, 
        SdfPath const& cachePath,
        UsdTimeCode time) const;

    /// Returns the purpose token for \p prim. If a non-empty \p
    /// instanceInheritablePurpose is specified and the prim doesn't have an 
    /// explicitly authored or inherited purpose, it may inherit the 
    /// instancer's purpose if the instance has an explicit purpose.
    USDIMAGING_API
    virtual TfToken GetPurpose(
        UsdPrim const& prim, 
        SdfPath const& cachePath,
        TfToken const& instanceInheritablePurpose) const;

    /// Returns the purpose token for \p prim, but only if it is inheritable 
    /// by child prims (i.e. it is an explicitly authored purpose on the prim
    /// itself or one of the prim's ancestors), otherwise it returns the empty 
    /// token.
    USDIMAGING_API
    TfToken GetInheritablePurpose(UsdPrim const& prim) const;

    /// Fetches the transform for the given prim at the given time from a
    /// pre-computed cache of prim transforms. Requesting transforms at
    /// incoherent times is currently inefficient.
    USDIMAGING_API
    virtual GfMatrix4d GetTransform(UsdPrim const& prim, 
                                    SdfPath const& cachePath,
                                    UsdTimeCode time,
                                    bool ignoreRootTransform = false) const;

    /// Samples the transform for the given prim.
    USDIMAGING_API
    virtual size_t SampleTransform(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   UsdTimeCode time,
                                   size_t maxNumSamples, 
                                   float *sampleTimes,
                                   GfMatrix4d *sampleValues);

    /// Gets the value of the parameter named key for the given prim (which
    /// has the given cache path) and given time. If outIndices is not nullptr 
    /// and the value has indices, it will return the unflattened value and set 
    /// outIndices to the value's associated indices.
    USDIMAGING_API
    virtual VtValue Get(UsdPrim const& prim,
                        SdfPath const& cachePath,
                        TfToken const& key,
                        UsdTimeCode time, 
                        VtIntArray *outIndices) const;

    /// Gets the cullstyle of a specific path in the scene graph.
    USDIMAGING_API
    virtual HdCullStyle GetCullStyle(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     UsdTimeCode time) const;

    /// Gets the material path for the given prim, walking up namespace if
    /// necessary.  
    USDIMAGING_API
    SdfPath GetMaterialUsdPath(UsdPrim const& prim) const;

    /// Gets the model:drawMode attribute for the given prim, walking up
    /// the namespace if necessary.
    USDIMAGING_API
    TfToken GetModelDrawMode(UsdPrim const& prim);

    /// Gets the model draw mode object for the given prim, walking up the 
    /// namespace if necessary.
    USDIMAGING_API
    HdModelDrawMode GetFullModelDrawMode(UsdPrim const& prim);

    /// Computes the per-prototype instance indices for a UsdGeomPointInstancer.
    /// XXX: This needs to be defined on the base class, to have access to the
    /// delegate, but it's a clear violation of abstraction.  This call is only
    /// legal for prims of type UsdGeomPointInstancer; in other cases, the
    /// returned array will be empty and the computation will issue errors.
    USDIMAGING_API
    VtArray<VtIntArray> GetPerPrototypeIndices(UsdPrim const& prim,
                                               UsdTimeCode time) const;

    /// Gets the topology object of a specific Usd prim. If the
    /// adapter is a mesh it will return an HdMeshTopology,
    /// if it is of type basis curves, it will return an HdBasisCurvesTopology.
    /// If the adapter does not have a topology, it returns an empty VtValue.
    USDIMAGING_API
    virtual VtValue GetTopology(UsdPrim const& prim,
                                SdfPath const& cachePath,
                                UsdTimeCode time) const;

    /// Reads the extent from the given prim. If the extent is not authored,
    /// an empty GfRange3d is returned, the extent will not be computed.
    USDIMAGING_API
    virtual GfRange3d GetExtent(UsdPrim const& prim, 
                                SdfPath const& cachePath, 
                                UsdTimeCode time) const;

    /// Reads double-sided from the given prim. If not authored, returns false
    USDIMAGING_API
    virtual bool GetDoubleSided(UsdPrim const& prim, 
                                SdfPath const& cachePath, 
                                UsdTimeCode time) const;

    USDIMAGING_API
    virtual SdfPath GetMaterialId(UsdPrim const& prim, 
                                  SdfPath const& cachePath, 
                                  UsdTimeCode time) const;

    USDIMAGING_API
    virtual VtValue GetMaterialResource(UsdPrim const& prim, 
                                  SdfPath const& cachePath, 
                                  UsdTimeCode time) const;

    // ---------------------------------------------------------------------- //
    /// \name ExtComputations
    // ---------------------------------------------------------------------- //
    USDIMAGING_API
    virtual const TfTokenVector &GetExtComputationSceneInputNames(
        SdfPath const& cachePath) const;

    USDIMAGING_API
    virtual HdExtComputationInputDescriptorVector
    GetExtComputationInputs(UsdPrim const& prim,
                            SdfPath const& cachePath,
                            const UsdImagingInstancerContext* instancerContext)
                                    const;

    USDIMAGING_API
    virtual HdExtComputationOutputDescriptorVector
    GetExtComputationOutputs(UsdPrim const& prim,
                             SdfPath const& cachePath,
                             const UsdImagingInstancerContext* instancerContext)
                                    const;

    USDIMAGING_API
    virtual HdExtComputationPrimvarDescriptorVector
    GetExtComputationPrimvars(
            UsdPrim const& prim,
            SdfPath const& cachePath,
            HdInterpolation interpolation,
            const UsdImagingInstancerContext* instancerContext) const;

    USDIMAGING_API
    virtual VtValue 
    GetExtComputationInput(
            UsdPrim const& prim,
            SdfPath const& cachePath,
            TfToken const& name,
            UsdTimeCode time,
            const UsdImagingInstancerContext* instancerContext) const;

    USDIMAGING_API
    virtual size_t
    SampleExtComputationInput(
            UsdPrim const& prim,
            SdfPath const& cachePath,
            TfToken const& name,
            UsdTimeCode time,
            const UsdImagingInstancerContext* instancerContext,
            size_t maxSampleCount,
            float *sampleTimes,
            VtValue *sampleValues);

    USDIMAGING_API
    virtual std::string 
    GetExtComputationKernel(
            UsdPrim const& prim,
            SdfPath const& cachePath,
            const UsdImagingInstancerContext* instancerContext) const;

    USDIMAGING_API
    virtual VtValue
    GetInstanceIndices(UsdPrim const& instancerPrim,
                       SdfPath const& instancerCachePath,
                       SdfPath const& prototypeCachePath,
                       UsdTimeCode time) const;

    // ---------------------------------------------------------------------- //
    /// \name Render Index Compatibility
    // ---------------------------------------------------------------------- //

    /// Returns true if the adapter can be populated into the target index.
    virtual bool IsSupported(UsdImagingIndexProxy const* index) const
    {
        return true;
    }

    // ---------------------------------------------------------------------- //
    /// \name Utilties
    // ---------------------------------------------------------------------- //

    /// Provides to paramName->UsdAttribute value mappings
    USDIMAGING_API
    static UsdAttribute LookupLightParamAttribute(
            UsdPrim const& prim,
            TfToken const& paramName);

protected:
    friend class UsdImagingInstanceAdapter;
    friend class UsdImagingPointInstancerAdapter;
    // ---------------------------------------------------------------------- //
    /// \name Utility
    // ---------------------------------------------------------------------- //
    
    // Given the USD path for a prim of this adapter's type, returns
    // the prim's Hydra cache path.
    USDIMAGING_API
    virtual SdfPath
    ResolveCachePath(
        const SdfPath& usdPath,
        const UsdImagingInstancerContext* instancerContext = nullptr) const;

    using Keys = UsdImagingPrimvarDescCache::Key;

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
    UsdImagingPrimvarDescCache* _GetPrimvarDescCache() const;

    USDIMAGING_API
    UsdImaging_NonlinearSampleCountCache*
        _GetNonlinearSampleCountCache() const;

    USDIMAGING_API
    UsdImaging_BlurScaleCache*
        _GetBlurScaleCache() const;

    USDIMAGING_API
    UsdPrim _GetPrim(SdfPath const& usdPath) const;

    // Returns the prim adapter for the given \p prim, or an invalid pointer if
    // no adapter exists. If \p prim is an instance and \p ignoreInstancing
    // is \c true, the instancing adapter will be ignored and an adapter will
    // be looked up based on \p prim's type.
    USDIMAGING_API
    const UsdImagingPrimAdapterSharedPtr& 
    _GetPrimAdapter(UsdPrim const& prim, bool ignoreInstancing = false) const;

    USDIMAGING_API
    const UsdImagingPrimAdapterSharedPtr& 
    _GetAdapter(TfToken const& adapterKey) const;

    // XXX: Transitional API
    // Returns the instance proxy prim path for a USD-instanced prim, given the
    // instance chain leading to that prim. The paths are sorted from more to
    // less local; the first path is the prim path (possibly in prototype), then
    // instance paths (possibly in prototype); the last path is the prim or
    // instance path in the scene.
    USDIMAGING_API
    SdfPath _GetPrimPathFromInstancerChain(
                                    SdfPathVector const& instancerChain) const;

    USDIMAGING_API
    UsdTimeCode _GetTimeWithOffset(float offset) const;

    // Converts \p cachePath to the path in the render index.
    USDIMAGING_API
    SdfPath _ConvertCachePathToIndexPath(SdfPath const& cachePath) const;

    // Converts \p indexPath to the path in the USD stage
    USDIMAGING_API
    SdfPath _ConvertIndexPathToCachePath(SdfPath const& indexPath) const;

    // Returns the material binding purpose from the renderer delegate.
    USDIMAGING_API
    TfToken _GetMaterialBindingPurpose() const;

    // Returns the material contexts from the renderer delegate.
    USDIMAGING_API
    TfTokenVector _GetMaterialRenderContexts() const;

    // Returns the namespace prefixes for render settings attributes relevant 
    // to a renderer delegate.
    USDIMAGING_API
    TfTokenVector _GetRenderSettingsNamespaces() const;

    /// Returns whether custom shading of prims is enabled.
    USDIMAGING_API
    bool _GetSceneMaterialsEnabled() const;

    /// Returns whether lights found in the usdscene are enabled.
    USDIMAGING_API
    bool _GetSceneLightsEnabled() const;

    // Returns true if render delegate wants primvars to be filtered based.
    // This will filter the primvars based on the bound material primvar needs.
    USDIMAGING_API
    bool _IsPrimvarFilteringNeeded() const;

    // Returns the shader source type from the render delegate.
    USDIMAGING_API
    TfTokenVector _GetShaderSourceTypes() const;

    // Returns \c true if \p usdPath is included in the scene delegate's
    // invised path list.
    USDIMAGING_API
    bool _IsInInvisedPaths(SdfPath const& usdPath) const;

    // Determines if an attribute is varying and if so, sets the given
    // \p dirtyFlag in the \p dirtyFlags and increments a perf counter. Returns
    // true if the attribute is varying.
    //
    // If \p exists is non-null, _IsVarying will store whether the attribute
    // was found.  If the attribute is not found, it counts as non-varying.
    // 
    // This only sets the dirty bit, never un-sets.  The caller is responsible
    // for setting the initial state correctly.
    USDIMAGING_API
    bool _IsVarying(UsdPrim prim, TfToken const& attrName, 
           HdDirtyBits dirtyFlag, TfToken const& perfToken,
           HdDirtyBits* dirtyFlags, bool isInherited,
           bool* exists = nullptr) const;

    // Determines if the prim's transform (CTM) is varying and if so, sets the 
    // given \p dirtyFlag in the \p dirtyFlags and increments a perf counter. 
    // Returns true if the prim's transform is varying.
    //
    // This only sets the dirty bit, never un-sets.  The caller is responsible
    // for setting the initial state correctly.
    USDIMAGING_API
    bool _IsTransformVarying(UsdPrim prim,
                             HdDirtyBits dirtyFlag,
                             TfToken const& perfToken,
                             HdDirtyBits* dirtyFlags) const;

    // Convenience method for adding or updating a primvar descriptor.
    // Role defaults to empty token (none). Indexed defaults to false.
    USDIMAGING_API
    void _MergePrimvar(
        HdPrimvarDescriptorVector* vec,
        TfToken const& name,
        HdInterpolation interp,
        TfToken const& role = TfToken(), 
        bool indexed = false) const;

    // Convenience method for removing a primvar descriptor.
    USDIMAGING_API
    void _RemovePrimvar(
        HdPrimvarDescriptorVector* vec,
        TfToken const& name) const;

    // Convenience method for computing a primvar. The primvar will only be
    // added to the list of prim desc if there is no primvar of the same
    // name already present.  Thus, "local" primvars should be merged before
    // inherited primvars.
    USDIMAGING_API
    void _ComputeAndMergePrimvar(
        UsdPrim const& prim,
        UsdGeomPrimvar const& primvar,
        UsdTimeCode time,
        HdPrimvarDescriptorVector* primvarDescs,
        HdInterpolation *interpOverride = nullptr) const;

    // Returns true if the property name has the "primvars:" prefix.
    USDIMAGING_API
    static bool _HasPrimvarsPrefix(TfToken const& propertyName);

    // Convenience methods to figure out what changed about the primvar and
    // return the appropriate dirty bit.
    // Caller can optionally pass in a dirty bit to set for primvar value
    // changes. This is useful for attributes that have a special dirty bit
    // such as normals and widths.
    //
    // Handle USD attributes that are treated as primvars by Hydra. This
    // requires the interpolation to be passed in, as well as the primvar
    // name passed to Hydra.
    USDIMAGING_API
    HdDirtyBits _ProcessNonPrefixedPrimvarPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName,
        TfToken const& primvarName,
        HdInterpolation const& primvarInterp,
        HdDirtyBits valueChangeDirtyBit = HdChangeTracker::DirtyPrimvar) const;

    // Handle UsdGeomPrimvars that use the "primvars:" prefix, while also
    // accommodating inheritance.
    USDIMAGING_API
    HdDirtyBits _ProcessPrefixedPrimvarPropertyChange(
        UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName,
        HdDirtyBits valueChangeDirtyBit = HdChangeTracker::DirtyPrimvar,
        bool inherited = true) const;

    virtual void _RemovePrim(SdfPath const& cachePath,
                             UsdImagingIndexProxy* index) = 0;

    // Utility to resync bound dependencies of a particular usd path.
    // This is necessary for the resync processing of certain prim types
    // (e.g. materials).
    USDIMAGING_API
    void _ResyncDependents(SdfPath const& usdPath,
                           UsdImagingIndexProxy *index);

    USDIMAGING_API
    UsdImaging_CollectionCache& _GetCollectionCache() const;

    USDIMAGING_API
    UsdStageRefPtr _GetStage() const;

    USDIMAGING_API
    UsdImaging_CoordSysBindingStrategy::value_type
    _GetCoordSysBindings(UsdPrim const& prim) const;

    USDIMAGING_API
    UsdImaging_InheritedPrimvarStrategy::value_type
    _GetInheritedPrimvars(UsdPrim const& prim) const;

    // Utility for derived classes to try to find an inherited primvar.
    USDIMAGING_API
    UsdGeomPrimvar _GetInheritedPrimvar(UsdPrim const& prim,
                                        TfToken const& primvarName) const;

    USDIMAGING_API
    GfInterval _GetCurrentTimeSamplingInterval();

    USDIMAGING_API
    Usd_PrimFlagsConjunction _GetDisplayPredicate() const;

    USDIMAGING_API
    Usd_PrimFlagsConjunction _GetDisplayPredicateForPrototypes() const;

    USDIMAGING_API
    bool _DoesDelegateSupportCoordSys() const;

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
        return std::make_shared<T>();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_PRIM_ADAPTER_H
