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
#ifndef USDIMAGING_DELEGATE_H
#define USDIMAGING_DELEGATE_H

/// \file usdImaging/delegate.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/collectionCache.h"
#include "pxr/usdImaging/usdImaging/valueCache.h"
#include "pxr/usdImaging/usdImaging/inheritedCache.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/selection.h"
#include "pxr/imaging/hd/texture.h"
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
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"

#include <boost/container/flat_map.hpp>
#include <boost/scoped_ptr.hpp>
#include <tbb/spin_rw_mutex.h>
#include <map>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_PTRS(UsdImagingDelegate);
typedef std::vector<UsdPrim> UsdPrimVector;

class UsdImagingPrimAdapter;
class UsdImagingIndexProxy;
class UsdImagingInstancerContext;

typedef boost::container::flat_map<SdfPath, bool> PickabilityMap;
typedef boost::shared_ptr<UsdImagingPrimAdapter> UsdImagingPrimAdapterSharedPtr;

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

    /// Opportunity for the delegate to clean itself up after
    /// performing parrellel work during sync phase
    USDIMAGING_API
    virtual void PostSyncCleanup() override;

    // TODO: Populate implies that multiple stages can be loaded at once, though
    // this is not currently supported.

    /// Populates the rootPrim in the HdRenderIndex in each delegate, 
    /// excluding all paths in the \p excludedPrimPaths, as well as their
    /// prim children.
    ///
    /// This is equivalent to calling Populate on each delegate individually.
    /// However, this method will try to parallelize certain operations,
    /// making it potentially more efficient.
    USDIMAGING_API
    static void Populate(std::vector<UsdImagingDelegate*> const& delegates,
                         UsdPrimVector const& rootPrims,
                         std::vector<SdfPathVector> const& excludedPrimPaths,
                         std::vector<SdfPathVector> const& invisedPrimPaths);
        
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

    /// Removes any explicit refine level set for the given prim, marks dirty if
    /// a change in level occurs.
    USDIMAGING_API
    void ClearRefineLevel(SdfPath const& usdPath);

    /// Sets an explicit refinement level for the given prim, if no level is
    /// explicitly set, the fallback is used, see GetRefineLevelFallback().
    /// If setting an explicit level does not change the effective level, no
    /// dirty bit is set.
    USDIMAGING_API
    void SetRefineLevel(SdfPath const& usdPath, int level);

    /// Returns true is the prims refinement level > 0
    USDIMAGING_API
    bool IsRefined(SdfPath const& usdPath) const;

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
    /// render prims generated. Settting this value will immediately invalidate
    /// existing rprim transforms.
    USDIMAGING_API
    void SetRootTransform(GfMatrix4d const& xf);

    /// Returns the root transform for the entire delegate.
    const GfMatrix4d &GetRootTransform() const { return _rootXf; }

    /// Sets the root visibility for the entire delegate, which is applied to
    /// all render prims generated. Settting this value will immediately
    /// invalidate existing rprim visibility.
    USDIMAGING_API
    void SetRootVisibility(bool isVisible);

    /// Returns the root visibility for the entire delegate.
    bool GetRootVisibility() const { return _rootIsVisible; }

    /// Set the list of paths that must be invised.
    USDIMAGING_API
    void SetInvisedPrimPaths(SdfPathVector const &invisedPaths);

    /// Set transform value overrides on a set of paths.
    USDIMAGING_API
    void SetRigidXformOverrides(RigidXformOverridesMap const &overrides);

    /// Returns the root paths of pickable objects.
    USDIMAGING_API
    PickabilityMap GetPickabilityMap() const;

    /// Sets pickability for a specific path.
    USDIMAGING_API
    void SetPickability(SdfPath const& path, bool pickable);

    /// Clears any pickability opinions that this delegates might have.
    USDIMAGING_API
    void ClearPickabilityMap();

    /// Sets display guides rendering
    USDIMAGING_API
    void SetDisplayGuides(bool displayGuides);
    bool GetDisplayGuides() const { return _displayGuides; }

    /// Returns whether draw modes are enabled.
    USDIMAGING_API
    void SetUsdDrawModesEnabled(bool enableUsdDrawModes);
    bool GetUsdDrawModesEnabled() const { return _enableUsdDrawModes; }

    /// Enables custom shading on prims.
    USDIMAGING_API
    void SetSceneMaterialsEnabled(bool enable);

    // ---------------------------------------------------------------------- //
    // See HdSceneDelegate for documentation of the following virtual methods.
    // ---------------------------------------------------------------------- //
    USDIMAGING_API
    virtual TfToken GetRenderTag(SdfPath const& id) override;
    USDIMAGING_API
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id) override;
    USDIMAGING_API
    virtual HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const& id) override;
    typedef PxOsdSubdivTags SubdivTags;

    // XXX: animated subdiv tags are not currently supported
    // XXX: subdiv tags currently feteched on demand 
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
    virtual VtValue Get(SdfPath const& id, TfToken const& key) override;
    USDIMAGING_API
    virtual HdReprSelector GetReprSelector(SdfPath const &id) override;
    USDIMAGING_API
    virtual VtArray<TfToken> GetCategories(SdfPath const &id) override;
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

    // Material Support
    USDIMAGING_API
    virtual SdfPath GetMaterialId(SdfPath const &rprimId) override;
    USDIMAGING_API
    virtual std::string GetSurfaceShaderSource(SdfPath const &id) override;
    USDIMAGING_API
    virtual std::string GetDisplacementShaderSource(SdfPath const &id) override;
    USDIMAGING_API
    virtual VtValue GetMaterialParamValue(SdfPath const &id,
                                          TfToken const &paramName) override;
    USDIMAGING_API
    virtual HdMaterialParamVector GetMaterialParams(SdfPath const &id) override;

    // Texture Support
    USDIMAGING_API
    HdTextureResource::ID GetTextureResourceID(SdfPath const &id) override;
    USDIMAGING_API
    virtual HdTextureResourceSharedPtr GetTextureResource(SdfPath const &id) override;

    // Light Support
    USDIMAGING_API
    virtual VtValue GetLightParamValue(SdfPath const &id, 
                                       TfToken const &paramName) override;

    // Volume Support
    USDIMAGING_API
    virtual HdVolumeFieldDescriptorVector
    GetVolumeFieldDescriptors(SdfPath const &volumeId) override;

    // Material Support
    USDIMAGING_API 
    virtual VtValue GetMaterialResource(SdfPath const &materialId) override;
    USDIMAGING_API
    virtual TfTokenVector GetMaterialPrimvars(SdfPath const &materialId) override;

    USDIMAGING_API
    virtual VtDictionary
    GetMaterialMetadata(SdfPath const &materialId) override;

    // Instance path resolution

    /// Returns the path of the instance prim corresponding to the
    /// protoPrimPath and instance index generated by instancer.
    ///
    /// if the instancer instances heterogeneously, instanceIndex of the
    /// prototype rprim doesn't match the instanceIndex in the instancer.
    ///
    /// for example:
    ///   instancer = [ A, B, A, B, B ]
    ///        instanceIndex       absoluteInstanceIndex
    ///     A: [0, 1]              [0, 2]
    ///     B: [0, 1, 2]           [1, 3, 5]
    ///
    /// To track this mapping, absoluteInstanceIndex is returned which
    /// is an instanceIndex of the instancer for the given instanceIndex of
    /// the prototype.
    ///
    /// If \p instanceContext is not NULL, it is populated with the list of 
    /// instance roots that must be traversed to get to the rprim. The last prim
    /// in this list is always the forwarded rprim.
    /// 
    /// ALL_INSTANCES may be returned if the protoPrimPath isn't instanced.
    ///
    static constexpr int ALL_INSTANCES = -1;
    USDIMAGING_API
    virtual SdfPath GetPathForInstanceIndex(const SdfPath &protoPrimPath,
                                            int instanceIndex,
                                            int *absoluteInstanceIndex,
                                            SdfPath *rprimPath=NULL,
                                            SdfPathVector *instanceContext=NULL) override;

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
    std::string GetExtComputationKernel(SdfPath const& computationId) override;

    USDIMAGING_API
    void InvokeExtComputation(SdfPath const& computationId,
                              HdExtComputationContext *context) override;

public:
    // Converts a UsdStage path to a path in the render index.
    USDIMAGING_API
    SdfPath GetPathForIndex(SdfPath const& usdPath) {
        SdfPathMap::const_iterator it = _cache2indexPath.find(usdPath);
        if (it != _cache2indexPath.end()) {
            return it->second;
        }

        // For pure/plain usdImaging, there is no prefix to replace
        SdfPath const &delegateID = GetDelegateID();
        if (delegateID == SdfPath::AbsoluteRootPath()) {
            return usdPath;
        }
        if (usdPath.IsEmpty()) {
            return usdPath;
        }

        return usdPath.ReplacePrefix(SdfPath::AbsoluteRootPath(), delegateID);
    }

    /// Convert the given Hydra ID to a UsdImaging cache path,
    /// by stripping the scene delegate prefix.
    ///
    /// The UsdImaging cache path is the same as a USD prim path,
    /// except for instanced prims, which get a name-mangled encoding.
    USDIMAGING_API
    SdfPath GetPathForUsd(SdfPath const& indexPath) {
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

    /// Populate HdxSelection for given \p path (root) and \p instanceIndex
    /// if path is instancer and instanceIndex is -1, all instances will be
    /// selected.
    ///
    /// XXX: subtree highlighting with native instancing is not working
    /// correctly right now. Path needs to be a leaf prim or instancer.
    USDIMAGING_API
    bool PopulateSelection(HdSelection::HighlightMode const& highlightMode,
                           const SdfPath &path,
                           int instanceIndex,
                           HdSelectionSharedPtr const &result);

    /// Returns true if \p usdPath is included in invised path list.
    USDIMAGING_API
    bool IsInInvisedPaths(const SdfPath &usdPath) const;

private:
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
    void _OnObjectsChanged(UsdNotice::ObjectsChanged const&,
                           UsdStageWeakPtr const& sender);

    // The lightest-weight update, it does fine-grained invalidation of
    // individual properties at the given path (prim or property).
    //
    // If \p path is a prim path, changedPrimInfoFields will be populated
    // with the list of scene description fields that caused this prim to
    // be refreshed.
    void _RefreshObject(SdfPath const& path, 
                        TfTokenVector const& changedPrimInfoFields,
                        UsdImagingIndexProxy* proxy);

    // Heavy-weight invalidation of an entire prim subtree. All cached data is
    // reconstructed for all prims below \p rootPath.
    //
    // By default, _ResyncPrim will remove each affected prim and call
    // Repopulate() on those prims individually. If repopulateFromRoot is
    // true, Repopulate() will be called on \p rootPath instead. This is slower,
    // but handles changes in tree topology.
    void _ResyncPrim(SdfPath const& rootPath, UsdImagingIndexProxy* proxy,
                     bool repopulateFromRoot = false);

    // ---------------------------------------------------------------------- //
    // Usd Data-Access Helper Methods
    // ---------------------------------------------------------------------- //
    UsdPrim _GetPrim(SdfPath const& usdPath) {
        UsdPrim const& p = 
                    _stage->GetPrimAtPath(usdPath.GetAbsoluteRootOrPrimPath());
        TF_VERIFY(p, "No prim found for id: %s",
                  usdPath.GetAbsoluteRootOrPrimPath().GetText());
        return p;
    }

    void _UpdateSingleValue(SdfPath const& usdPath, int dirtyFlags);

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
    typedef boost::shared_ptr<UsdImagingPrimAdapter> _AdapterSharedPtr;
    typedef TfHashMap<TfToken, 
                         _AdapterSharedPtr, TfToken::HashFunctor> _AdapterMap;
    _AdapterMap _adapterMap;

    // Per-Hydra-Primitive tracking data
    struct _PrimInfo {
        _AdapterSharedPtr adapter;          // The adapter to use for the prim
        UsdPrim           usdPrim;          // Reference to the Usd prim
        HdDirtyBits       timeVaryingBits;  // Dirty Bits to set when
                                            // time changes
        HdDirtyBits       dirtyBits;        // Current dirty state of the prim.
    };

    typedef TfHashMap<SdfPath, _PrimInfo, SdfPath::Hash> _PrimInfoMap;

    _PrimInfoMap _primInfoMap;       // Indexed by "Cache Path"

    // SdfPath::ReplacePrefix() is used frequently to convert between
    // cache path and Hydra render index path and is a performance bottleneck.
    // These maps pre-computes these conversion.
    typedef TfHashMap<SdfPath, SdfPath, SdfPath::Hash> SdfPathMap;
    SdfPathMap _cache2indexPath;
    SdfPathMap _index2cachePath;

    // List of all prim Id's for sub-tree analysis
    Hd_SortedIds _usdIds;

    // Only use this method when we think no existing adapter has been
    // established. For example, during initial Population.
    _AdapterSharedPtr const& _AdapterLookup(UsdPrim const& prim, 
                                            bool ignoreInstancing = false);

    // Obtain the prim tracking data for the given cache path.
    _PrimInfo *GetPrimInfo(const SdfPath &cachePath);

    typedef TfHashSet<SdfPath, SdfPath::Hash> _InstancerSet;
    _InstancerSet _instancerPrimPaths;

    void _MarkSubtreeTransformDirty(SdfPath const &subtreeRoot);
    void _MarkSubtreeVisibilityDirty(SdfPath const &subtreeRoot);

    bool _IsChildPath(SdfPath const& path) const {
        return path.IsPropertyPath();
    }

    /// Refinement level per-prim and fallback.
    typedef TfHashMap<SdfPath, int, SdfPath::Hash> _RefineLevelMap;
    _RefineLevelMap _refineLevelMap;

    /// Cached/pre-fetched rprim data.
    UsdImagingValueCache _valueCache;

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

    /// The current time from which the delegate will read data.
    UsdTimeCode _time;

    /// The requested time offsets for time samples.
    std::vector<float> _timeSampleOffsets;

    int _refineLevelFallback;
    HdReprSelector _reprFallback;
    HdCullStyle _cullStyleFallback;

    // Change processing
    TfNotice::Key _objectsChangedNoticeKey;
    SdfPathVector _pathsToResync;

    // Map from path of Usd object to update to list of changed scene 
    // description fields for that object. This list of fields is only
    // populated for prim paths.
    typedef std::unordered_map<SdfPath, TfTokenVector, SdfPath::Hash> 
        _PathsToUpdateMap;
    _PathsToUpdateMap _pathsToUpdate;

    UsdImaging_XformCache _xformCache;
    UsdImaging_MaterialBindingImplData _materialBindingImplData;
    UsdImaging_MaterialBindingCache _materialBindingCache;
    UsdImaging_VisCache _visCache;
    UsdImaging_PurposeCache _purposeCache;
    UsdImaging_DrawModeCache _drawModeCache;
    UsdImaging_CollectionCache _collectionCache;

    // Pickability
    PickabilityMap _pickablesMap;

    // Display guides rendering
    bool _displayGuides;
    bool _enableUsdDrawModes;

    const bool _hasDrawModeAdapter;

    /// Enable custom shading of prims
    bool _sceneMaterialsEnabled;

    UsdImagingDelegate() = delete;
    UsdImagingDelegate(UsdImagingDelegate const &) = delete;
    UsdImagingDelegate &operator =(UsdImagingDelegate const &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //USDIMAGING_DELEGATE_H
