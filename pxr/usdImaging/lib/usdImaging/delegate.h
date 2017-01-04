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

#include "pxr/usdImaging/usdImaging/valueCache.h"
#include "pxr/usdImaging/usdImaging/inheritedCache.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/shaderAdapter.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/surfaceShader.h"
#include "pxr/imaging/hd/texture.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdx/selectionTracker.h"

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

TF_DECLARE_WEAK_PTRS(UsdImagingDelegate);
typedef std::vector<UsdPrim> UsdPrimVector;

class UsdImagingPrimAdapter;
class UsdImagingIndexProxy;
class UsdImagingInstancerContext;
class UsdImagingDefaultShaderAdapter;

typedef boost::shared_ptr<UsdImagingPrimAdapter> UsdImagingPrimAdapterSharedPtr;

typedef boost::shared_ptr<UsdImagingShaderAdapter> UsdImagingShaderAdapterSharedPtr ;

/// \class UsdImagingDelegate
///
/// The primary translation layer between the Hydra (Hd) core and the Usd
/// scene graph.
///
class UsdImagingDelegate : public HdSceneDelegate, public TfWeakBase {
    typedef UsdImagingDelegate This;
public:

    typedef TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> RigidXformOverridesMap;
    typedef boost::container::flat_map<SdfPath, int /*purposeMask*/>
        CollectionMembershipMap;
    typedef TfHashMap<TfToken, CollectionMembershipMap, TfToken::HashFunctor>
        CollectionMap;

    UsdImagingDelegate();

    /// Constructor used for nested delegate objects which share a RenderIndex.
    UsdImagingDelegate(HdRenderIndexSharedPtr const& parentIndex, 
                    SdfPath const& delegateID);

    virtual ~UsdImagingDelegate();

    virtual void Sync(HdSyncRequestVector* request);

    // Helper for clients who don't want to drive the sync behavior (unit
    // tests). Note this method is not virtual.
    void SyncAll(bool includeUnvarying);

    /// Opportunity for the delegate to clean itself up after
    /// performing parrellel work during sync phase
    virtual void PostSyncCleanup();

    // TODO: Populate implies that multiple stages can be loaded at once, though
    // this is not currently supported.

    /// Populates the rootPrim in the HdRenderIndex in each delegate, 
    /// excluding all paths in the \p excludedPrimPaths, as well as their
    /// prim children.
    ///
    /// This is equivalent to calling Populate on each delegate individually.
    /// However, this method will try to parallelize certain operations,
    /// making it potentially more efficient.
    static void Populate(std::vector<UsdImagingDelegate*> const& delegates,
                         UsdPrimVector const& rootPrims,
                         std::vector<SdfPathVector> const& excludedPrimPaths,
                         std::vector<SdfPathVector> const& invisedPrimPaths);
        
    /// Populates the rootPrim in the HdRenderIndex.
    void Populate(UsdPrim const& rootPrim);

    /// Populates the rootPrim in the HdRenderIndex, excluding all paths in the
    /// \p excludedPrimPaths, as well as their prim children.
    void Populate(UsdPrim const& rootPrim,
                  SdfPathVector const& excludedPrimPaths,
                  SdfPathVector const &invisedPrimPaths=SdfPathVector());

    /// For each delegate in \p delegates, sets the current time from
    /// which data wil be read to the corresponding time in \p times.
    ///
    /// This is equivalent to calling SetTime on each delegate individually.
    /// However, this method will try to parallelize certain operations,
    /// making it potentially more efficient.
    static void SetTimes(const std::vector<UsdImagingDelegate*>& delegates,
                         const std::vector<UsdTimeCode>& times);

    /// Sets the current time from which data will be read by the delegate.
    ///
    /// Changing the current time immediately triggers invalidation in the
    /// HdChangeTracker. Redundantly setting the time to its existing value is a
    /// no-op and will not trigger invalidation.
    void SetTime(UsdTimeCode time);

    /// Returns the current time.
    UsdTimeCode GetTime() const { return _time; }

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
    void SetRefineLevelFallback(int level);

    /// Removes any explicit refine level set for the given prim, marks dirty if
    /// a change in level occurs.
    void ClearRefineLevel(SdfPath const& usdPath);

    /// Sets an explicit refinement level for the given prim, if no level is
    /// explicitly set, the fallback is used, see GetRefineLevelFallback().
    /// If setting an explicit level does not change the effective level, no
    /// dirty bit is set.
    void SetRefineLevel(SdfPath const& usdPath, int level);

    /// Returns the fallback repr name.
    TfToken GetReprFallback() const { return _reprFallback; }

    /// Sets the fallback repr name. Note that currently UsdImagingDelegate
    /// doesn't support per-prim repr.
    void SetReprFallback(TfToken const &repr);

    /// Returns the fallback cull style.
    HdCullStyle GetCullStyleFallback() const { return _cullStyleFallback; }

    /// Sets the fallback cull style.
    void SetCullStyleFallback(HdCullStyle cullStyle);

    /// Sets the root transform for the entire delegate, which is applied to all
    /// render prims generated. Settting this value will immediately invalidate
    /// existing rprim transforms.
    void SetRootTransform(GfMatrix4d const& xf);

    /// Returns the root transform for the entire delegate.
    const GfMatrix4d &GetRootTransform() const { return _rootXf; }

    /// Sets a compensating root transformation for the entire delegate
    /// which cancels out the effects of any transformation accumulated
    /// from the root from which the delegate was populated to the descendent
    /// at /a usdPath.
    void SetRootCompensation(SdfPath const &usdPath);
    const SdfPath & GetRootCompensation() const { return _compensationPath; }

    /// Sets the root visibility for the entire delegate, which is applied to
    /// all render prims generated. Settting this value will immediately
    /// invalidate existing rprim visibility.
    void SetRootVisibility(bool isVisible);

    /// Returns the root visibility for the entire delegate.
    bool GetRootVisibility() const { return _rootIsVisible; }

    /// Set the list of paths that must be invised.
    void SetInvisedPrimPaths(SdfPathVector const &invisedPaths);

    /// Set transform value overrides on a set of paths.
    void SetRigidXformOverrides(RigidXformOverridesMap const &overrides);

    enum PurposeMask {
        PurposeNone    = 0,
        PurposeDefault = (1<<0),
        PurposeRender  = (1<<1),
        PurposeProxy   = (1<<2),
        PurposeGuide   = (1<<3)
    };

    /// Sets the membership flag of user-defined \p collectionName for the
    /// entire delegate. IsInCollection responds based on this setting.
    void SetInCollection(TfToken const &collectionName, int purposeMask);

    /// Sets the membership of user-defined \p collectionName as determined
    /// by \p membershipMap, a SdfPathMap to purposeMask (int) from rprim
    /// memership (or not) can be determined.
    ///
    /// IsInCollection responds based on this setting.
    void TransferCollectionMembershipMap(
        TfToken const &collectionName,
        CollectionMembershipMap &&membershipMap);

    /// Sets the collection map
    /// (discard previously set existing map by SetInCollection)
    void SetCollectionMap(CollectionMap const &collectionMap);

    /// Returns the collection map
    CollectionMap GetCollectionMap() const { return _collectionMap; }

    // ---------------------------------------------------------------------- //
    // See HdSceneDelegate for documentation of the following virtual methods.
    // ---------------------------------------------------------------------- //
    virtual bool IsInCollection(SdfPath const& id, 
                                TfToken const& collectionName);
    virtual HdMeshTopology GetMeshTopology(SdfPath const& id);
    virtual HdBasisCurvesTopology GetBasisCurvesTopology(SdfPath const& id);
    typedef PxOsdSubdivTags SubdivTags;

    // XXX: animated subdiv tags are not currently supported
    // XXX: subdiv tags currently feteched on demand 
    virtual SubdivTags GetSubdivTags(SdfPath const& id);

    virtual GfRange3d GetExtent(SdfPath const & id);
    virtual GfMatrix4d GetTransform(SdfPath const & id);
    virtual bool GetVisible(SdfPath const & id);
    virtual GfVec4f GetColorAndOpacity(SdfPath const & id);
    virtual bool GetDoubleSided(SdfPath const & id);
    virtual HdCullStyle GetCullStyle(SdfPath const &id);

    /// Gets the explicit refinement level for the given prim, if no level is
    /// explicitly set, the fallback is returned; also see 
    /// GetRefineLevelFallback().
    virtual int GetRefineLevel(SdfPath const& id);

    /// Returns the ranges of instances.
    virtual VtVec2iArray GetInstances(SdfPath const& id);

    virtual VtValue Get(SdfPath const& id, TfToken const& key);
    virtual TfToken GetReprName(SdfPath const &id);
    virtual TfTokenVector GetPrimVarVertexNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarVaryingNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarFacevaryingNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarUniformNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarConstantNames(SdfPath const& id);
    virtual TfTokenVector GetPrimVarInstanceNames(SdfPath const& id);
    virtual int GetPrimVarDataType(SdfPath const& id, TfToken const& key);
    virtual int GetPrimVarComponents(SdfPath const& id, TfToken const& key);
    virtual VtIntArray GetInstanceIndices(SdfPath const &instancerId,
                                          SdfPath const &prototypeId);
    virtual GfMatrix4d GetInstancerTransform(SdfPath const &instancerId,
                                             SdfPath const &prototypeId);

    // Shader Support
    virtual bool GetSurfaceShaderIsTimeVarying(SdfPath const& id);
    virtual std::string GetSurfaceShaderSource(SdfPath const &id);
    virtual TfTokenVector GetSurfaceShaderParamNames(SdfPath const &id);
    virtual VtValue GetSurfaceShaderParamValue(SdfPath const &id, 
                                  TfToken const &paramName);
    virtual HdShaderParamVector GetSurfaceShaderParams(SdfPath const &id);
    virtual SdfPathVector GetSurfaceShaderTextures(SdfPath const &shaderId);

    // Texture Support
    HdTextureResource::ID GetTextureResourceID(SdfPath const &id);
    virtual HdTextureResourceSharedPtr GetTextureResource(SdfPath const &id);

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
    virtual SdfPath GetPathForInstanceIndex(const SdfPath &protoPrimPath,
                                            int instanceIndex,
                                            int *absoluteInstanceIndex,
                                            SdfPath * rprimPath=NULL,
                                            SdfPathVector *instanceContext=NULL);

private:
    typedef TfHashMap<SdfPath, SdfPath, SdfPath::Hash> _PathToPathMap;
public:
    // Converts a UsdStage path to a path in the render index.
    SdfPath GetPathForIndex(SdfPath const& usdPath) {
        // For pure/plain usdImaging, there is no prefix to replace
        SdfPath const &delegateID = GetDelegateID();
        if (delegateID == SdfPath::AbsoluteRootPath()) {
            return usdPath;
        }
        if (usdPath.IsEmpty()) {
            return usdPath;
        }

        tbb::spin_rw_mutex::scoped_lock 
            lock(_usdToIndexPathMapMutex, /* write = */ false);

        _PathToPathMap::const_iterator it = _usdToIndexPathMap.find(usdPath);
        if (it != _usdToIndexPathMap.end())
            return it->second;

        const SdfPath indexPath = 
            usdPath.ReplacePrefix(SdfPath::AbsoluteRootPath(), delegateID);

        lock.upgrade_to_writer();
        _usdToIndexPathMap[usdPath] = indexPath;
        return indexPath;
    }

    SdfPath GetPathForUsd(SdfPath const& indexPath) {
        // For pure/plain usdImaging, there is no prefix to replace
        SdfPath const &delegateID = GetDelegateID();
        if (delegateID == SdfPath::AbsoluteRootPath())
            return indexPath;

        tbb::spin_rw_mutex::scoped_lock 
            lock(_indexToUsdPathMapMutex, /* write = */ false);

        _PathToPathMap::const_iterator it = _indexToUsdPathMap.find(indexPath);
        if (it != _indexToUsdPathMap.end())
            return it->second;

        const SdfPath usdPath = 
            indexPath.ReplacePrefix(delegateID, SdfPath::AbsoluteRootPath());

        lock.upgrade_to_writer();
        _indexToUsdPathMap[indexPath] = usdPath;
        return usdPath;
    }

    /// Populate HdxSelection for given \p path (root) and \p instanceIndex
    /// if path is instancer and instanceIndex is -1, all instances will be
    /// selected.
    ///
    /// XXX: subtree highlighting with native instancing is not working
    /// correctly right now. Path needs to be a leaf prim or instancer.
    bool PopulateSelection(const SdfPath &path,
                           int instanceIndex,
                           HdxSelectionSharedPtr const &result);

    /// Returns true if \p usdPath is included in invised path list.
    bool IsInInvisedPaths(const SdfPath &usdPath) const;

private:
    // Internal friend class.
    class _Worker;
    friend class UsdImagingIndexProxy;
    friend class UsdImagingPrimAdapter;

    // UsdImagingShaderAdapter needs access to _GetPrim.  We should
    // consider making it public.
    friend class UsdImagingShaderAdapter;

    bool _ValidateRefineLevel(int level) {
        if (!(0 <= level && level <= 8)) {
            TF_CODING_ERROR("Invalid refinement level(%d), "
                            "expected range is [0,8]",
                            level);
            return false;
        }
        return true;
    }

    // ---------------------------------------------------------------------- //
    // Usd Change Processing / Notice Handlers 
    // ---------------------------------------------------------------------- //
    void _OnObjectsChanged(UsdNotice::ObjectsChanged const&,
                           UsdStageWeakPtr const& sender);

    // The lightest-weight update, it does fine-grained invalidation of
    // individual properties at the given path (prim or property).
    void _RefreshObject(SdfPath const& path, UsdImagingIndexProxy* proxy);

    // Heavy-weight invalidation of a single property (the property was added or
    // removed).
    void _ResyncProperty(SdfPath const& path, UsdImagingIndexProxy* proxy);

    // Heavy-weight invalidation of an entire prim subtree. All cached data is
    // reconstructed for all prims below \p rootPath.
    void _ResyncPrim(SdfPath const& rootPath, UsdImagingIndexProxy* proxy);

    // ---------------------------------------------------------------------- //
    // Usd Data-Access Helper Methods
    // ---------------------------------------------------------------------- //
    UsdPrim _GetPrim(SdfPath const& usdPath) {
        UsdPrim const& p = 
                    _stage->GetPrimAtPath(usdPath.GetAbsoluteRootOrPrimPath());
        if (!TF_VERIFY(p))
            TF_CODING_ERROR("No prim found for id: %s", 
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

    bool _ComputeRootCompensation(SdfPath const & usdPath);
    void _UpdateRootTransform();
    GfMatrix4d _GetTransform(UsdPrim prim) const;

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

    // Set the delegate's current time to the given time and process
    // any object changes that have occurred in the interim.
    bool _ProcessChangesForTimeUpdate(UsdTimeCode time);

    // Add tasks to update prims with time-varying attributes to the
    // given worker.
    void _PrepareWorkerForTimeUpdate(_Worker* worker);

    // Execute all time update tasks that have been added to the given worker.
    static void _ExecuteWorkForTimeUpdate(_Worker* worker, 
                                          bool updateDelegates = true);

    // ---------------------------------------------------------------------- //
    // Core Delegate state
    // ---------------------------------------------------------------------- //
    typedef boost::shared_ptr<UsdImagingPrimAdapter> _AdapterSharedPtr;
    typedef TfHashMap<TfToken, 
                         _AdapterSharedPtr, TfToken::HashFunctor> _AdapterMap;
    _AdapterMap _adapterMap;

    // Only use this method when we think no existing adapter has been
    // established. For example, during initial Population.
    _AdapterSharedPtr const& _AdapterLookup(UsdPrim const& prim, 
                                            bool ignoreInstancing = false);

    // This method should be used for all cases in which we expect an adapter to
    // have been registered for a prim. Note that usdPath here should always
    // match some "cachePath" from the adapters perspective. This also allows
    // for a fan-out of multiple adapters to handle a single underlying UsdPrim.
    _AdapterSharedPtr const& _AdapterLookupByPath(SdfPath const& usdPath);

    // A mapping from usd scene graph path to prim adapter. Note that this path
    // can be different from the path in the dirty map, which is intentionally
    // the same as the render index. This map facillitates usd-to-adapter
    // lookups, which are required during change processing (usd reports a usd
    // path, which may not match the render index path, particularly for child
    // rprims). 
    typedef SdfPathTable<_AdapterSharedPtr> _PathAdapterMap;
    _PathAdapterMap _pathAdapterMap;

    typedef UsdImagingShaderAdapterSharedPtr _ShaderAdapterSharedPtr;

    // This method looks up a shader adapter based on the \p shaderId.
    // Currently, it's hard coded to return _shaderAdapter but could be
    // extended.
    //
    // This will never return a nullptr.  
    _ShaderAdapterSharedPtr  _ShaderAdapterLookup(SdfPath const& shaderId) const;

    // XXX: These maps could be store as individual member paths on the Rprim
    // itself, which seems like a much nicer way of maintaining the mapping.
    tbb::spin_rw_mutex _indexToUsdPathMapMutex;
    _PathToPathMap _indexToUsdPathMap;
    tbb::spin_rw_mutex _usdToIndexPathMapMutex;
    _PathToPathMap _usdToIndexPathMap;

    /// Tracks a set of dirty flags for each prim, these flags get sent to the
    /// render index as time changes to trigger invalidation. All prims exist in
    /// this map, but some will have flags set to HdChangeTracker::Clean.
    typedef TfHashMap<SdfPath, int, SdfPath::Hash> _DirtyMap;
    _DirtyMap _dirtyMap;

    typedef TfHashMap<SdfPath, bool, SdfPath::Hash> _ShaderMap;
    _ShaderMap _shaderMap;

    typedef TfHashSet<SdfPath, SdfPath::Hash> _TextureSet;
    typedef TfHashSet<SdfPath, SdfPath::Hash> _InstancerSet;
    _TextureSet _texturePaths;
    _InstancerSet _instancerPrimPaths;

    // Retrieves the dirty bits for a given usdPath and allows mutation of the
    // held value, but requires that the entry already exists in the map.
    int* _GetDirtyBits(SdfPath const& usdPath);

    void _MarkRprimOrInstancerDirty(SdfPath const& usdPath, int dirtyFlags);

    void _MarkSubtreeDirty(SdfPath const &subtreeRoot,
                           int rprimDirtyFlag,
                           int instancerDirtyFlag);

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

    int _refineLevelFallback;
    TfToken _reprFallback;
    HdCullStyle _cullStyleFallback;

    // Change processing
    TfNotice::Key _objectsChangedNoticeKey;
    SdfPathVector _pathsToResync;
    SdfPathVector _pathsToUpdate;

    UsdImaging_XformCache _xformCache;
    UsdImaging_MaterialBindingCache _materialBindingCache;
    UsdImaging_VisCache _visCache;

    // Collection
    CollectionMap _collectionMap;

    UsdImagingShaderAdapterSharedPtr _shaderAdapter;
};

/// \class UsdImagingIndexProxy
///
/// This proxy class exposes a subset of the private Delegate API to
/// PrimAdapters.
///
class UsdImagingIndexProxy {
public: 
    // Create a dependency on usdPath for the specified prim adapter. When no
    // prim adapter is specified, the Usd prim will be fetched from the current
    // stage and the typename will be used to find the associated adapter. If no
    // adapter exists for the type name, an error will be issued.
    void AddDependency(SdfPath const& usdPath, 
                        UsdImagingPrimAdapterSharedPtr const& adapter =
                                    UsdImagingPrimAdapterSharedPtr());

    SdfPath InsertMesh(SdfPath const& usdPath,
                       SdfPath const& shaderBinding,
                       UsdImagingInstancerContext const* instancerContext);

    SdfPath InsertBasisCurves(SdfPath const& usdPath,
                       SdfPath const& shaderBinding,
                       UsdImagingInstancerContext const* instancerContext);

    SdfPath InsertPoints(SdfPath const& usdPath,
                       SdfPath const& shaderBinding,
                       UsdImagingInstancerContext const* instancerContext);

    // Inserts an instancer into the HdRenderIndex and schedules it for updates
    // from the delegate.
    void InsertInstancer(SdfPath const& usdPath,
                UsdImagingInstancerContext const* instancerContext);

    // Refresh the HdRprim at the specified render index path.
    void Refresh(SdfPath const& cachePath);

    // Refresh the HdInstancer at the specified render index path.
    void RefreshInstancer(SdfPath const& instancerPath);

    //
    // All removals are deferred to avoid surprises during change processing.
    //
    
    // Removes the dependency on the specified Usd path. Notice that this is the
    // path in the Usd scene graph, not the path in the RenderIndex.
    void RemoveDependency(SdfPath const& usdPath) {
        _depsToRemove.push_back(usdPath);
    }

    // Removes the HdRprim at the specified render index path. 
    void RemoveRprim(SdfPath const& cachePath) { 
        _rprimsToRemove.push_back(cachePath);
    }

    // Removes the HdInstancer at the specified render index path.
    void RemoveInstancer(SdfPath const& instancerPath) { 
        _instancersToRemove.push_back(instancerPath);
    }

    // Recursively repopulate the specified usdPath into the render index.
    void Repopulate(SdfPath const& usdPath);

private:
    friend class UsdImagingDelegate;
    UsdImagingIndexProxy(UsdImagingDelegate* delegate,
                            UsdImagingDelegate::_Worker* worker) 
        : _delegate(delegate)
        , _worker(worker)
    {}

    SdfPathVector const& _GetPathsToRepopulate() { return _pathsToRepopulate; }
    UsdImagingDelegate::_Worker* _GetWorker() { return _worker; }
    void _ProcessRemovals();

    // Insert a child of the given path; the full child path is returned if
    // successful, the empty path is returned on failure.
    //
    // Note that this method does not implicitly add a dependency because the
    // child is likely to represent a different prim in the Usd scene graph.
    template <typename T>
    SdfPath _InsertRprim(SdfPath const& usdPath,
                        SdfPath const& shaderBinding,
                        UsdImagingInstancerContext const* instancerContext);

    void _AddTask(SdfPath const& usdPath);   

    UsdImagingDelegate* _delegate;
    UsdImagingDelegate::_Worker* _worker;
    SdfPathVector _pathsToRepopulate;
    SdfPathVector _rprimsToRemove;
    SdfPathVector _instancersToRemove;
    SdfPathVector _depsToRemove;
};

#endif //USDIMAGING_DELEGATE_H
