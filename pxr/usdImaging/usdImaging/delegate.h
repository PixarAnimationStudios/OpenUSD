//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DELEGATE_H
#define PXR_USD_IMAGING_USD_IMAGING_DELEGATE_H

/// \file usdImaging/delegate.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/version.h"
#include "pxr/usdImaging/usdImaging/collectionCache.h"
#include "pxr/usdImaging/usdImaging/primvarDescCache.h"
#include "pxr/usdImaging/usdImaging/resolvedAttributeCache.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/imaging/hd/coordSys.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/pxOsd/subdivTags.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathTable.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/cube.h"
#include "pxr/usd/usdGeom/sphere.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/denseHashSet.h"

#include <tbb/spin_rw_mutex.h>
#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_PTRS(UsdImagingDelegate);
typedef std::vector<UsdPrim> UsdPrimVector;

class UsdImagingPrimAdapter;
class UsdImagingIndexProxy;
class UsdImagingInstancerContext;

using UsdImagingPrimAdapterSharedPtr = std::shared_ptr<UsdImagingPrimAdapter>;

/// \class UsdImagingDelegate
///
/// The primary translation layer between the Hydra (Hd) core and the Usd
/// scene graph.
///
class UsdImagingDelegate : public HdSceneDelegate, public TfWeakBase {
    typedef UsdImagingDelegate This;
public:

    typedef TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> RigidXformOverridesMap;

    USDIMAGING_API
    UsdImagingDelegate(HdRenderIndex *parentIndex,
                       SdfPath const& delegateID);

    USDIMAGING_API
    virtual ~UsdImagingDelegate();

    USDIMAGING_API
    virtual void Sync(HdSyncRequestVector* request) override;

    // Helper for clients who don't want to drive the sync behavior (unit
    // tests). Note this method is not virtual.
    USDIMAGING_API
    void SyncAll(bool includeUnvarying);

    /// Populates the rootPrim in the HdRenderIndex.
    USDIMAGING_API
    void Populate(UsdPrim const& rootPrim);

    /// Populates the rootPrim in the HdRenderIndex, excluding all paths in the
    /// \p excludedPrimPaths, as well as their prim children.
    USDIMAGING_API
    void Populate(UsdPrim const& rootPrim,
                  SdfPathVector const& excludedPrimPaths,
                  SdfPathVector const &invisedPrimPaths=SdfPathVector());

    /// For each delegate in \p delegates, sets the current time from
    /// which data wil be read to the corresponding time in \p times.
    ///
    /// This is equivalent to calling SetTime on each delegate individually.
    /// However, this method will try to parallelize certain operations,
    /// making it potentially more efficient.
    USDIMAGING_API
    static void SetTimes(const std::vector<UsdImagingDelegate*>& delegates,
                         const std::vector<UsdTimeCode>& times);

    /// Sets the current time from which data will be read by the delegate.
    ///
    /// Changing the current time immediately triggers invalidation in the
    /// HdChangeTracker. Redundantly setting the time to its existing value is a
    /// no-op and will not trigger invalidation.
    USDIMAGING_API
    void SetTime(UsdTimeCode time);

    /// Returns the current time.
    UsdTimeCode GetTime() const { return _time; }

    /// Apply a relative offset to the current time.
    /// This has no effect in the case of the default USD timecode.
    UsdTimeCode GetTimeWithOffset(float offset) const;

    /// Applies any scene edits which have been queued up by notices from USD.
    USDIMAGING_API
    void ApplyPendingUpdates();

    /// Returns the refinement level that is used when prims have no explicit
    /// level set.
    ///
    /// The refinement level indicates how many iterations to apply when
    /// subdividing subdivision surfaces or other refinable primitives.
    ///
    /// Refinement level is always in the range [0,8].
    int GetRefineLevelFallback() const { return _refineLevelFallback; }

    /// Sets the fallback refinement level to \p level, triggers dirty
    /// refine level bit to be set on all Rprims that don't have explicit refine
    /// levels set.
    ///
    /// Level is expected to be in the range [0,8], where 0 indicates no
    /// refinement.
    USDIMAGING_API
    void SetRefineLevelFallback(int level);

    /// Removes any explicit refine level set for the given USD prim.
    /// Marks dirty if a change in level occurs.
    USDIMAGING_API
    void ClearRefineLevel(SdfPath const& usdPath);

    /// Sets an explicit refinement level for the given USD prim.
    /// If no level is explicitly set, the fallback is used;
    /// see GetRefineLevelFallback().
    /// If setting an explicit level does not change the effective level, no
    /// dirty bit is set.
    USDIMAGING_API
    void SetRefineLevel(SdfPath const& usdPath, int level);

    /// Returns the fallback repr name.
    HdReprSelector GetReprFallback() const { return _reprFallback; }

    /// Sets the fallback repr name. Note that currently UsdImagingDelegate
    /// doesn't support per-prim repr.
    USDIMAGING_API
    void SetReprFallback(HdReprSelector const &repr);

    /// Returns the fallback cull style.
    HdCullStyle GetCullStyleFallback() const { return _cullStyleFallback; }

    /// Sets the fallback cull style.
    USDIMAGING_API
    void SetCullStyleFallback(HdCullStyle cullStyle);

    /// Sets the root transform for the entire delegate, which is applied to all
    /// render prims generated. Setting this value will immediately invalidate
    /// existing rprim transforms.
    USDIMAGING_API
    void SetRootTransform(GfMatrix4d const& xf);

    /// Returns the root transform for the entire delegate.
    const GfMatrix4d &GetRootTransform() const { return _rootXf; }

    /// Sets the root visibility for the entire delegate, which is applied to
    /// all render prims generated. Setting this value will immediately
    /// invalidate existing rprim visibility.
    USDIMAGING_API
    void SetRootVisibility(bool isVisible);

    /// Returns the root visibility for the entire delegate.
    bool GetRootVisibility() const { return _rootIsVisible; }

    /// Sets the root instancer id for the entire delegate, which is used as a 
    /// fallback value for GetInstancerId.
    USDIMAGING_API
    void SetRootInstancerId(SdfPath const& instancerId);

    /// Returns the root instancer id for the entire delegate.
    SdfPath GetRootInstancerId() const { return _rootInstancerId; }

    /// Set the list of paths that must be invised.
    USDIMAGING_API
    void SetInvisedPrimPaths(SdfPathVector const &invisedPaths);

    /// Set transform value overrides on a set of paths.
    USDIMAGING_API
    void SetRigidXformOverrides(RigidXformOverridesMap const &overrides);

    /// Sets display of prims with purpose "render"
    USDIMAGING_API
    void SetDisplayRender(const bool displayRender);
    bool GetDisplayRender() const { return _displayRender; }

    /// Sets display of prims with purpose "proxy"
    USDIMAGING_API
    void SetDisplayProxy(const bool displayProxy);
    bool GetDisplayProxy() const { return _displayProxy; }

    /// Sets display of prims with purpose "guide"
    USDIMAGING_API
    void SetDisplayGuides(const bool displayGuides);
    bool GetDisplayGuides() const { return _displayGuides; }

    /// Returns whether draw modes are enabled.
    USDIMAGING_API
    void SetUsdDrawModesEnabled(bool enableUsdDrawModes);
    bool GetUsdDrawModesEnabled() const { return _enableUsdDrawModes; }

    /// Enables custom shading on prims.
    USDIMAGING_API
    void SetSceneMaterialsEnabled(bool enable);

    /// Enables lights found in the usdscene.
    USDIMAGING_API
    void SetSceneLightsEnabled(bool enable);

    /// Set the window policy on all scene cameras. This comes from
    /// the application.
    USDIMAGING_API
    void SetWindowPolicy(CameraUtilConformWindowPolicy policy);

    /// Sets display of unloaded prims as bounding boxes.
    /// Unloaded prims will need to satisfy one of the following set of
    /// conditions in order to see them:
    ///   1. The prim is a UsdGeomBoundable with an authored 'extent' attribute.
    ///   2. The prim is a UsdPrim.IsModel() and has an authored 'extentsHint'
    ///      attribute (see UsdGeomModelAPI::GetExtentsHint).
    /// Effective only for delegates that support draw modes (see
    /// GetUsdDrawModesEnabled()).
    USDIMAGING_API
    void SetDisplayUnloadedPrimsWithBounds(bool displayUnloaded);
    bool GetDisplayUnloadedPrimsWithBounds() const {
        return _displayUnloadedPrimsWithBounds;
    }

    /// Setup for the shutter open and close to be used for motion sampling.
    USDIMAGING_API
    void SetCameraForSampling(SdfPath const& id);

    /// Returns the current interval that will be used when using the 
    /// sample* API in the scene delegate.
    USDIMAGING_API
    GfInterval GetCurrentTimeSamplingInterval();

    // ---------------------------------------------------------------------- //
    // See HdSceneDelegate for documentation of the following virtual methods.
    // ---------------------------------------------------------------------- //
    USDIMAGING_API
    virtual TfToken GetRenderTag(SdfPath const& id) override;
    USDIMAGING_API
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id) override;
    USDIMAGING_API
    virtual HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const& id) 
        override;
    typedef PxOsdSubdivTags SubdivTags;

    // XXX: animated subdiv tags are not currently supported
    // XXX: subdiv tags currently fetched on-demand 
    USDIMAGING_API
    virtual SubdivTags GetSubdivTags(SdfPath const& id) override;

    USDIMAGING_API
    virtual GfRange3d GetExtent(SdfPath const & id) override;
    USDIMAGING_API
    virtual GfMatrix4d GetTransform(SdfPath const & id) override;
    USDIMAGING_API
    virtual bool GetVisible(SdfPath const & id) override;
    USDIMAGING_API
    virtual bool GetDoubleSided(SdfPath const & id) override;
    USDIMAGING_API
    virtual HdCullStyle GetCullStyle(SdfPath const &id) override;

    /// Gets the explicit display style for the given prim, if no refine level
    /// is explicitly set, the fallback is returned; also see 
    /// GetRefineLevelFallback().
    USDIMAGING_API
    virtual HdDisplayStyle GetDisplayStyle(SdfPath const& id) override;

    USDIMAGING_API
    HdModelDrawMode GetModelDrawMode(SdfPath const& id) override;

    USDIMAGING_API
    virtual VtValue Get(SdfPath const& id, TfToken const& key) override;
    USDIMAGING_API
    virtual VtValue GetIndexedPrimvar(SdfPath const& id, 
                                      TfToken const& key, 
                                      VtIntArray *outIndices) override;
    USDIMAGING_API
    HdIdVectorSharedPtr
    virtual GetCoordSysBindings(SdfPath const& id) override;
    USDIMAGING_API
    virtual HdReprSelector GetReprSelector(SdfPath const &id) override;
    USDIMAGING_API
    virtual VtArray<TfToken> GetCategories(SdfPath const &id) override;
    USDIMAGING_API
    virtual std::vector<VtArray<TfToken>>
    GetInstanceCategories(SdfPath const &instancerId) override;
    USDIMAGING_API
    virtual HdPrimvarDescriptorVector
    GetPrimvarDescriptors(SdfPath const& id,
                          HdInterpolation interpolation) override;
    USDIMAGING_API
    virtual VtIntArray GetInstanceIndices(SdfPath const &instancerId,
                                          SdfPath const &prototypeId) override;
    USDIMAGING_API
    virtual GfMatrix4d GetInstancerTransform(SdfPath const &instancerId) 
        override;

    USDIMAGING_API
    virtual SdfPath GetInstancerId(SdfPath const &primId) override;

    USDIMAGING_API
    virtual SdfPathVector GetInstancerPrototypes(SdfPath const &instancerId) override;

    // Motion samples
    USDIMAGING_API
    virtual size_t
    SampleTransform(SdfPath const & id, size_t maxNumSamples,
                    float *times, GfMatrix4d *samples) override;
    USDIMAGING_API
    virtual size_t
    SampleInstancerTransform(SdfPath const &instancerId,
                             size_t maxSampleCount, float *times,
                             GfMatrix4d *samples) override;
    USDIMAGING_API
    virtual size_t
    SamplePrimvar(SdfPath const& id, TfToken const& key,
                  size_t maxNumSamples, float *times,
                  VtValue *samples) override;

    USDIMAGING_API
    virtual size_t
    SampleIndexedPrimvar(SdfPath const& id, TfToken const& key,
                         size_t maxNumSamples, float *times,
                         VtValue *samples, VtIntArray *indices) override;

    // Material Support
    USDIMAGING_API
    virtual SdfPath GetMaterialId(SdfPath const &rprimId) override;

    USDIMAGING_API 
    virtual VtValue GetMaterialResource(SdfPath const &materialId) override;

    // Light Support
    USDIMAGING_API
    virtual VtValue GetLightParamValue(SdfPath const &id, 
                                       TfToken const &paramName) override;
    // Camera Support
    USDIMAGING_API
    virtual VtValue GetCameraParamValue(SdfPath const &id, 
                                        TfToken const &paramName) override;

    // Volume Support
    USDIMAGING_API
    virtual HdVolumeFieldDescriptorVector
    GetVolumeFieldDescriptors(SdfPath const &volumeId) override;

    // Picking path resolution
    // Resolves a \p rprimId and \p instanceIndex back to the original USD
    // gprim and instance index.  For point-instanced prims, \p instanceContext
    // returns extra information about which instance this is of which level of
    // point-instancer.  For example:
    //   /World/PI instances /World/PI/proto/PI
    //   /World/PI/proto/PI instances /World/PI/proto/PI/proto/Gprim
    //   instancerContext = [/World/PI, 0], [/World/PI/proto/PI, 1] means that
    //   this instance represents "protoIndex = 0" of /World/PI, etc.

    USDIMAGING_API
    virtual SdfPath
    GetScenePrimPath(SdfPath const& rprimId,
                     int instanceIndex,
                     HdInstancerContext *instancerContext = nullptr) override;

    USDIMAGING_API
    virtual SdfPathVector
    GetScenePrimPaths(SdfPath const& rprimId,
                      std::vector<int> instanceIndices,
                      std::vector<HdInstancerContext> *instancerContexts = nullptr) override;

    // ExtComputation support
    USDIMAGING_API
    TfTokenVector
    GetExtComputationSceneInputNames(SdfPath const& computationId) override;

    USDIMAGING_API
    HdExtComputationInputDescriptorVector
    GetExtComputationInputDescriptors(SdfPath const& computationId) override;

    USDIMAGING_API
    HdExtComputationOutputDescriptorVector
    GetExtComputationOutputDescriptors(SdfPath const& computationId) override;

    USDIMAGING_API
    HdExtComputationPrimvarDescriptorVector
    GetExtComputationPrimvarDescriptors(SdfPath const& computationId,
                                        HdInterpolation interpolation) override;

    USDIMAGING_API
    VtValue GetExtComputationInput(SdfPath const& computationId,
                                   TfToken const& input) override;

    USDIMAGING_API
    size_t SampleExtComputationInput(SdfPath const& computationId,
                                     TfToken const& input,
                                     size_t maxSampleCount,
                                     float *sampleTimes,
                                     VtValue *sampleValues) override;

    USDIMAGING_API
    std::string GetExtComputationKernel(SdfPath const& computationId) override;

    USDIMAGING_API
    void InvokeExtComputation(SdfPath const& computationId,
                              HdExtComputationContext *context) override;

public:
    // Converts a cache path to a path in the render index.
    USDIMAGING_API
    SdfPath ConvertCachePathToIndexPath(SdfPath const& cachePath) {
        SdfPathMap::const_iterator it = _cache2indexPath.find(cachePath);
        if (it != _cache2indexPath.end()) {
            return it->second;
        }

        // For pure/plain usdImaging, there is no prefix to replace
        SdfPath const &delegateID = GetDelegateID();
        if (delegateID == SdfPath::AbsoluteRootPath()) {
            return cachePath;
        }
        if (cachePath.IsEmpty()) {
            return cachePath;
        }

        return cachePath.ReplacePrefix(SdfPath::AbsoluteRootPath(), delegateID);
    }

    /// Convert the given Hydra ID to a UsdImaging cache path,
    /// by stripping the scene delegate prefix.
    ///
    /// The UsdImaging cache path is the same as a USD prim path,
    /// except for instanced prims, which get a name-mangled encoding.
    USDIMAGING_API
    SdfPath ConvertIndexPathToCachePath(SdfPath const& indexPath) {
        SdfPathMap::const_iterator it = _index2cachePath.find(indexPath);
        if (it != _index2cachePath.end()) {
            return it->second;
        }

        // For pure/plain usdImaging, there is no prefix to replace
        SdfPath const &delegateID = GetDelegateID();
        if (delegateID == SdfPath::AbsoluteRootPath()) {
            return indexPath;
        }

        return indexPath.ReplacePrefix(delegateID, SdfPath::AbsoluteRootPath());
    }

    /// Populate HdxSelection for given \p path (root) and \p instanceIndex.
    /// If indexPath is an instancer and instanceIndex is ALL_INSTANCES (-1),
    /// all instances will be selected.
    ///
    /// Note: if usdPath points to a gprim, "instanceIndex" (if provided)
    /// is assumed to be the hydra-computed instance index returned from
    /// picking code.
    ///
    /// If usdPath points to a point instancer, "instanceIndex" is assumed to
    /// be the instance of the point instancer to selection highlight (e.g.
    /// instance N of the protoIndices array).  This would correspond to
    /// returning one of the tuples from GetScenePrimPath's "instancerContext".
    ///
    /// In any other case, the interpretation of instanceIndex is undefined.
    static constexpr int ALL_INSTANCES = -1;
    USDIMAGING_API
    bool PopulateSelection(HdSelection::HighlightMode const& highlightMode,
                           const SdfPath &usdPath,
                           int instanceIndex,
                           HdSelectionSharedPtr const &result);

    /// Returns true if \p usdPath is included in invised path list.
    USDIMAGING_API
    bool IsInInvisedPaths(const SdfPath &usdPath) const;

private:
    // Internal Get and SamplePrimvar
    VtValue _Get(SdfPath const& id, TfToken const& key, VtIntArray *outIndices);

    size_t _SamplePrimvar(SdfPath const& id, TfToken const& key,
                          size_t maxNumSamples, float *times, VtValue *samples, 
                          VtIntArray *indices);

    // Internal friend class.
    class _Worker;
    friend class UsdImagingIndexProxy;
    friend class UsdImagingPrimAdapter;

    bool _ValidateRefineLevel(int level) {
        if (!(0 <= level && level <= 8)) {
            TF_CODING_ERROR("Invalid refinement level(%d), "
                            "expected range is [0,8]",
                            level);
            return false;
        }
        return true;
    }

    void _AddTask(UsdImagingDelegate::_Worker *worker, SdfPath const& usdPath);

    // ---------------------------------------------------------------------- //
    // Draw mode support
    // ---------------------------------------------------------------------- //
    // Determine whether to assign a draw mode adapter to the given prim.
    bool _IsDrawModeApplied(UsdPrim const& prim);
    // Get the inherited model:drawMode attribute of the given prim.
    TfToken _GetModelDrawMode(UsdPrim const& prim);

    // ---------------------------------------------------------------------- //
    // Usd Change Processing / Notice Handlers 
    // ---------------------------------------------------------------------- //
    void _OnUsdObjectsChanged(UsdNotice::ObjectsChanged const&,
                              UsdStageWeakPtr const& sender);

    // Map holding USD subtree path keys mapped to associated hydra prim cache
    // paths. This may be prepopulated and provided to the Refresh and Resync
    // methods below to speed up dependency gathering.
    typedef TfHashMap<SdfPath, SdfPathVector, SdfPath::Hash>
        _FlattenedDependenciesCacheMap;

    // The lightest-weight update, it does fine-grained invalidation of
    // individual properties at the given path (prim or property).
    //
    // If \p path is a prim path, changedPrimInfoFields will be populated
    // with the list of scene description fields that caused this prim to
    // be refreshed.
    //
    // Returns whether the prim or the subtree rooted at `usdPath` needed to
    // be resync'd (i.e., removed and repopulated).
    //
    bool _RefreshUsdObject(SdfPath const& usdPath, 
                           TfTokenVector const& changedPrimInfoFields,
                           _FlattenedDependenciesCacheMap const &cache,
                           UsdImagingIndexProxy* proxy,
                           SdfPathSet* allTrackedVariabilityPaths); 

    // Heavy-weight invalidation of an entire prim subtree. All cached data is
    // reconstructed for all prims below \p rootPath.
    //
    // By default, _ResyncPrim will remove each affected prim and call
    // Repopulate() on those prims individually. If repopulateFromRoot is
    // true, Repopulate() will be called on \p rootPath instead. This is slower,
    // but handles changes in tree topology.
    void _ResyncUsdPrim(SdfPath const& usdRootPath,
                        _FlattenedDependenciesCacheMap const &cache,
                        UsdImagingIndexProxy* proxy,
                        bool repopulateFromRoot = false);

    // ---------------------------------------------------------------------- //
    // Usd Data-Access Helper Methods
    // ---------------------------------------------------------------------- //
    UsdPrim _GetUsdPrim(SdfPath const& usdPath) {
        UsdPrim const& p = 
                    _stage->GetPrimAtPath(usdPath.GetAbsoluteRootOrPrimPath());
        TF_VERIFY(p, "No prim found for id: %s",
                  usdPath.GetAbsoluteRootOrPrimPath().GetText());
        return p;
    }

    void _UpdateSingleValue(SdfPath const& cachePath, int dirtyFlags);

    // ---------------------------------------------------------------------- //
    // Cache structures and related methods for population. 
    // ---------------------------------------------------------------------- //

    // Returns true if this delegate can be populated, false otherwise.
    bool _CanPopulate(UsdPrim const& rootPrim) const;

    // Set the delegate's state to reflect that it will be populated from
    // the given root prim with the given excluded paths.
    void _SetStateForPopulation(UsdPrim const& rootPrim,
                                SdfPathVector const& excludedPaths,
                                SdfPathVector const& invisedPaths);

    // Populates this delegate's render index from the paths specified
    // in the given index proxy.
    void _Populate(class UsdImagingIndexProxy* proxy);

    // Execute all variability update tasks that have been added to the given
    // worker.
    static void _ExecuteWorkForVariabilityUpdate(_Worker* worker);

    /// Returns true if the given prim is visible, taking into account inherited
    /// visibility values. Inherited values are strongest, Usd has no notion of
    /// "super vis/invis".
    bool _GetVisible(UsdPrim const& prim);

    /// Helper method for filtering discovered primvar names.
    TfTokenVector _GetPrimvarNames(SdfPath const& usdPath,
                                   TfToken const& interpolation);

    // ---------------------------------------------------------------------- //
    // Helper methods for updating the delegate on time changes
    // ---------------------------------------------------------------------- //

    // Execute all time update tasks that have been added to the given worker.
    static void _ExecuteWorkForTimeUpdate(_Worker* worker);

    // ---------------------------------------------------------------------- //
    // Core Delegate state
    // ---------------------------------------------------------------------- //

    // Usd Prim Type to Adapter lookup table.
    typedef TfHashMap<TfToken, UsdImagingPrimAdapterSharedPtr, 
                TfToken::HashFunctor> _AdapterMap;
    _AdapterMap _adapterMap;

    // Per-Hydra-Primitive tracking data
    struct _HdPrimInfo {
        UsdImagingPrimAdapterSharedPtr adapter; // The adapter to use for the 
                                                // prim
        UsdPrim           usdPrim;          // Reference to the Usd prim
        HdDirtyBits       timeVaryingBits;  // Dirty Bits to set when
                                            // time changes
        HdDirtyBits       dirtyBits;        // Current dirty state of the prim.
        TfDenseHashSet<SdfPath, SdfPath::Hash>
                          extraDependencies;// Dependencies that aren't usdPrim.
    };

    typedef TfHashMap<SdfPath, _HdPrimInfo, SdfPath::Hash> _HdPrimInfoMap;

    // Map from cache path to Hydra prim info
    _HdPrimInfoMap _hdPrimInfoMap;

    typedef std::multimap<SdfPath, SdfPath> _DependencyMap;

    // Map from USD path to Hydra path, for tracking USD->hydra dependencies.
    _DependencyMap _dependencyInfo;

    // Appends hydra prim cache paths corresponding to the USD subtree
    // provided by looking up the dependency map (above).
    void _GatherDependencies(SdfPath const& subtree,
                             SdfPathVector *affectedCachePaths);

    // Overload that takes an additional cache argument to help speed up the
    // dependency gathering operation. The onus is on the client to prepopulate
    // the cache.
    void
    _GatherDependencies(SdfPath const &subtree,
                        _FlattenedDependenciesCacheMap const &cache,
                        SdfPathVector *affectedCachePaths);

    // SdfPath::ReplacePrefix() is used frequently to convert between
    // cache path and Hydra render index path and is a performance bottleneck.
    // These maps pre-computes these conversion.
    typedef TfHashMap<SdfPath, SdfPath, SdfPath::Hash> SdfPathMap;
    SdfPathMap _cache2indexPath;
    SdfPathMap _index2cachePath;

    // Only use this method when we think no existing adapter has been
    // established. For example, during initial Population.
    UsdImagingPrimAdapterSharedPtr const& _AdapterLookup(
                                                UsdPrim const& prim, 
                                                bool ignoreInstancing = false);
    UsdImagingPrimAdapterSharedPtr const& _AdapterLookup(
                                                TfToken const& adapterKey);

    // Obtain the prim tracking data for the given cache path.
    _HdPrimInfo *_GetHdPrimInfo(const SdfPath &cachePath);

    Usd_PrimFlagsConjunction _GetDisplayPredicate() const;
    Usd_PrimFlagsConjunction _GetDisplayPredicateForPrototypes() const;

    // Mark render tags dirty for all prims.
    // This is done in response to toggling the purpose-based display settings.
    void _MarkRenderTagsDirty();

    typedef TfHashSet<SdfPath, SdfPath::Hash> _DirtySet;

    // Set of cache paths that are due a Sync()
    _DirtySet _dirtyCachePaths;

    /// Refinement level per-USD-prim and fallback.
    typedef TfHashMap<SdfPath, int, SdfPath::Hash> _RefineLevelMap;
    /// Map from USD prim path to refine level.
    _RefineLevelMap _refineLevelMap;

    /// Cached/pre-fetched primvar descriptors.
    UsdImagingPrimvarDescCache _primvarDescCache;

    /// Usd binding.
    UsdStageRefPtr _stage;
    SdfPath _rootPrimPath;
    SdfPathVector _excludedPrimPaths;
    SdfPathVector _invisedPrimPaths;

    RigidXformOverridesMap _rigidXformOverrides;

    // Aspects of the delegate root that apply to all items in the index.
    SdfPath _compensationPath;

    GfMatrix4d _rootXf;
    bool _rootIsVisible;
    SdfPath _rootInstancerId;

    /// The current time from which the delegate will read data.
    UsdTimeCode _time;

    /// Path to the camera that its shutter will be used for time samples.
    SdfPath _cameraPathForSampling;

    int _refineLevelFallback;
    HdReprSelector _reprFallback;
    HdCullStyle _cullStyleFallback;

    // Cache of which prims are time-varying.
    SdfPathVector _timeVaryingPrimCache;
    bool _timeVaryingPrimCacheValid;

    // Change processing
    TfNotice::Key _objectsChangedNoticeKey;
    SdfPathVector _usdPathsToResync;

    // Map from path of Usd object to update to list of changed scene 
    // description fields for that object. This list of fields is only
    // populated for prim paths.
    typedef std::unordered_map<SdfPath, TfTokenVector, SdfPath::Hash> 
        _PathsToUpdateMap;
    _PathsToUpdateMap _usdPathsToUpdate;

    UsdImaging_XformCache _xformCache;
    UsdImaging_MaterialBindingImplData _materialBindingImplData;
    UsdImaging_MaterialBindingCache _materialBindingCache;
    UsdImaging_CoordSysBindingCache _coordSysBindingCache;
    UsdImaging_VisCache _visCache;
    UsdImaging_PurposeCache _purposeCache;
    UsdImaging_DrawModeCache _drawModeCache;
    UsdImaging_CollectionCache _collectionCache;
    UsdImaging_InheritedPrimvarCache _inheritedPrimvarCache;
    UsdImaging_PointInstancerIndicesCache _pointInstancerIndicesCache;
    UsdImaging_NonlinearSampleCountCache _nonlinearSampleCountCache;
    UsdImaging_BlurScaleCache _blurScaleCache;

    // Purpose-based rendering toggles
    bool _displayRender;
    bool _displayProxy;
    bool _displayGuides;
    bool _enableUsdDrawModes;

    const bool _hasDrawModeAdapter;

    /// Enable custom shading of prims
    bool _sceneMaterialsEnabled;

    /// Enable lights found in the usdscene
    bool _sceneLightsEnabled;

    CameraUtilConformWindowPolicy _appWindowPolicy;

    // Enable HdCoordSys tracking
    const bool _coordSysEnabled;

    // Display unloaded prims with Bounds adapter
    bool _displayUnloadedPrimsWithBounds;

    UsdImagingDelegate() = delete;
    UsdImagingDelegate(UsdImagingDelegate const &) = delete;
    UsdImagingDelegate &operator =(UsdImagingDelegate const &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_IMAGING_USD_IMAGING_DELEGATE_H
