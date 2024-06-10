//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INSTANCER_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INSTANCER_H

#include "pxr/pxr.h"

#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"

#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/timeSampleArray.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"

#include "tbb/concurrent_unordered_map.h"
#include "tbb/spin_rw_mutex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanInstancer : public HdInstancer
{

public:

    HdPrmanInstancer(HdSceneDelegate* delegate, SdfPath const& id);

    /// Destructor.
    ~HdPrmanInstancer();
    
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    void Finalize(HdRenderParam *renderParam) override;

    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /**
     * @brief Instructs the instancer to generate riley instances for the given 
     * prototypes. Caller is responsible for the lifecycle of the riley
     * prototypes, while the instancer will own the riley instances. This should
     * only be called with all of the riley prototypes associated with a given
     * hydra prototype path.
     * 
     * @param renderParam
     * @param dirtyBits The hydra prototype's dirty bits.
     * @param hydraPrototypeId The path of the hydra prototype prim.
     * @param rileyPrototypeIds The riley geometry prototype ids associated with
     *        this hydra prototype prim. There may be more than one, in the case
     *        of geomSubsets, or when a child instancer has more than one
     *        prototype group. If this is empty, all previously-populated
     *        instances associated with this hydraPrototypeId will be destroyed.
     *        It should not contain invalid prototype ids unless the hydra
     *        prototype is an analytic light, in which case it must contain
     *        exactly one invalid geometry prototype id.
     * @param coordSysList The coordinate system list for the hydra prototype.
     * @param protoParams The riley instance params derived from the hydra
     *        prototype. These will be applied to every riley instance except
     *        where they are overridden by riley instance params derived from
     *        the instancer. This collection may include visibility params, but
     *        should not include params used for light linking. These latter
     *        params will be derived by direct query of the scene delegate
     *        using hydraPrototypeId (or the appropriate prototypePrimPath, see
     *        below), and will be ignored and overwritten if they are present
     *        here. For a full list, see _GetLightLinkParams
     * @param protoXform The transform of the hydra prototype prim relative to
     *        the parent of the prototype root. This will be applied to the
     *        riley instances first, before the transform derived from the
     *        instancer's instancing mechanism or the instancer's own transform.
     * @param rileyMaterialIds The riley material ids to be assigned to the
     *        instances of each of the supplied riley prototypes; this should
     *        match rileyPrototypeIds in length and indexing.
     * @param prototypePaths The stage paths of the (sub)prims each riley 
     *        prototype id represents, e.g., the stage paths to the geomSubsets;
     *        this should always match prototypeIds in length and indexing.
     *        These are used for identification purposes and, when they are
     *        different from hydraPrototypeId, for retrieving light-linking
     *        categories, so they should (ideally) not be proxy paths.
     * @param lightShaderId (optional) The riley light shader id associated with
     *        this hydra prototype. When this is provided, we assume the hydra
     *        prototype prim is a light. When this is provided,
     *        rileyPrototypeIds must either have the geometry prototype id(s)
     *        for a mesh light or have a single invalid id for an analytic light.
     */
    void Populate(
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits,
        const SdfPath& hydraPrototypeId,
        const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds,
        const riley::CoordinateSystemList& coordSysList,
        const RtParamList protoParams,
        const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> protoXform,
        const std::vector<riley::MaterialId>& rileyMaterialIds,
        const SdfPathVector& prototypePaths,
        const riley::LightShaderId& lightShaderId = riley::LightShaderId::InvalidId());

    /**
     * @brief Instructs the instancer to destroy any riley instances for the
     * given hydra prototype prim path, optionally preserving those instances of
     * a given list of prototype ids.
     * 
     * @param renderParam 
     * @param prototypePrimPath The path of the hydra prototype.
     * @param excludedPrototypeIds List of riley prototype ids whose instances
     *        should be preserved. When empty or not provided, all instances of
     *        all prototypes for the given prototypePrimPath will be destroyed.
     *        HdPrmanInstancer itself uses this list to preserve instances of
     *        its own prototype groups when depopulating some instances from a
     *        parent instancer.
     */
    void Depopulate(
        HdRenderParam* renderParam,
        const SdfPath& prototypePrimPath,
        const std::vector<riley::GeometryPrototypeId>& excludedPrototypeIds = {});

private:

    // **********************************************
    // **              Private Types               **
    // **********************************************

    using _GfMatrixSA = HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES>;
    using _RtMatrixSA = HdTimeSampleArray<RtMatrix4x4, HDPRMAN_MAX_TIME_SAMPLES>;
    
    struct _RtParamListHashFunctor
    {
        size_t operator()(const RtParamList& params) const noexcept
        {
            // Wow this sucks, but RtParamList::Hash() is not const!
            RtParamList copy = params;
            return std::hash<uint32_t>()(copy.Hash());
        }
    };

    struct _RtParamListEqualToFunctor
    {
        bool operator()(const RtParamList& lhs, const RtParamList& rhs) const noexcept
        {
            return _RtParamListHashFunctor()(lhs) == _RtParamListHashFunctor()(rhs);
        }
    };

    struct _PrimvarValue
    {
        HdPrimvarDescriptor desc;
        VtValue value;
    };

    struct _FlattenData
    {
        
        // The set of light linking categories
        std::unordered_set<TfToken, TfToken::HashFunctor> categories;

        // We store visibility in an RtParamList to take advantage of that
        // structure's Inherit and Update methods, and because simply storing
        // a single boolean would clobber any renderer-specific params that might
        // have been authored on a given (native) instance.
        RtParamList params;

        _FlattenData() { }
        _FlattenData(const VtTokenArray& cats) 
            : categories(cats.begin(), cats.end()) { }
        _FlattenData(const VtTokenArray& cats, bool vis)
            : categories(cats.begin(), cats.end())
        {
            SetVisibility(vis);
        }
        // Copy constructor
        _FlattenData(const _FlattenData& other) 
        : categories(other.categories.cbegin(), other.categories.cend())
        {
            params.Update(other.params);
        }

        // Params that already exist here will not be changed;
        // categories will be merged
        void Inherit(const _FlattenData& rhs)
        {
            categories.insert(rhs.categories.cbegin(), rhs.categories.cend());
            params.Inherit(rhs.params);
        }

        // Params that already exist here will be changed;
        // categories will be merged
        void Update(const _FlattenData& rhs)
        {
            categories.insert(rhs.categories.cbegin(), rhs.categories.cend());
            params.Update(rhs.params);
        }

        // Update this FlattenData's visibility from an RtParamList. Visibility
        // params that already exist here will be changed; visibility and
        // light linking params on the RtParamList will be removed from it.
        void UpdateVisAndFilterParamList(RtParamList& other) {
            // Move visibility params from the RtParamList to the FlattenData
            for (const RtUString& param : _GetVisibilityParams()) {
                int val;
                if (other.GetInteger(param, val)) {
                    if (val == 1) {
                        params.Remove(param);
                    } else {
                        params.SetInteger(param, val);
                    }
                    other.Remove(param);
                }
            }

            // Copy any existing value for grouping:membership into the
            // flatten data. For lights, this gets a value during light sync,
            // and ConvertCategoriesToAttributes specifically handles preserving
            // it. We need to capture the value from light sync here so we can 
            // and flatten against it. It won't be captured by the categories
            // because the value set in light sync comes from a different
            // source. It has to be handled separately from categories.
            RtUString groupingMembership;
            if (other.GetString(RixStr.k_grouping_membership, groupingMembership)) {
                params.SetString(RixStr.k_grouping_membership, groupingMembership);
            }

            // Remove the light linking params from the RtParamList. Not going
            // to parse them back out to individual tokens to add to
            // the FlattenData categories, as they will be captured elsewhere.
            for (const RtUString& param : _GetLightLinkParams()) {
                other.Remove(param);
            }
        }

        // Sets all visibility params, overwriting current values.)
        void SetVisibility(bool visible) {
            if (visible) {
                for (const RtUString& param : _GetVisibilityParams()) {
                    params.Remove(param);
                }
            } else {
                for (const RtUString& param : _GetVisibilityParams()) {
                    params.SetInteger(param, 0);
                }
            }
        }

        // equals operator
        bool operator==(const _FlattenData& rhs) const noexcept
        {
            return categories == rhs.categories &&
                _RtParamListEqualToFunctor()(params, rhs.params);
        }

        struct HashFunctor {
            size_t operator()(const _FlattenData& fd) const noexcept
            {
                size_t hash = 0ul;

                // simple order-independent XOR hash aggregation 
                for (const TfToken& tok : fd.categories) {
                    hash ^= tok.Hash();
                }
                return hash ^ _RtParamListHashFunctor()(fd.params);
            }
        };

    private:
        static std::vector<RtUString> _GetLightLinkParams()
        {
            // List of riley instance params pertaining to light-linking that are
            // not supported on instances inside geometry prototype groups
            static const std::vector<RtUString> LightLinkParams = {
                RixStr.k_lightfilter_subset,
                RixStr.k_lighting_subset,
                RixStr.k_grouping_membership,
                RixStr.k_lighting_excludesubset
            };
            return LightLinkParams;
        }

        static std::vector<RtUString> _GetVisibilityParams()
        {
            // List of rile instance params pertaining to visibility that are
            // not supported on instances inside geometry prototype groups
            static const std::vector<RtUString> VisParams = {
                RixStr.k_visibility_camera,
                RixStr.k_visibility_indirect,
                RixStr.k_visibility_transmission
            };
            return VisParams;
        }

    };

    struct _InstanceData
    {
        _FlattenData flattenData;
        RtParamList params;
        _GfMatrixSA transform;

        _InstanceData() { }
        _InstanceData(
            const VtTokenArray& cats, 
            bool vis, 
            const RtParamList& p, 
            _GfMatrixSA& xform)
            : transform(xform)
        {
            params.Inherit(p);
        }
    };

    // A simple concurrent hashmap built from tbb::concurrent_unordered_map but
    // with a simpler interface. Thread-safe operations (insertion, retrieval,
    // const iteration) happen under a shared_lock, while unsafe operations
    // (erase, clear, non-const iteration) use an exclusive lock. This way, the
    // thread-safe operations can all run concurrently with one another, relying
    // on tbb::concurrent_unordered_map's thread safety, but will never run
    // while an unsafe operation is in progress, nor will the unsafe operations
    // start while a safe one is running.
    template<
        typename Key, 
        typename T, 
        typename Hash = std::hash<Key>, 
        typename KeyEqual = std::equal_to<Key>>
    class _LockingMap
    {
    public:
        // Check whether the map contains the given key; check this call before
        // calling get() if you want to avoid get's auto-insertion.
        bool has(const Key& key) const
        {
            tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
            if (_map.size() == 0) { return false; }
            return _map.find(key) != _map.end();
        }

        // Retrieve the value for the given key. If the key is not present in
        // the map, a default-constructed value will be inserted and returned.
        // T must have default constructor
        T& get(const Key& key)
        {
            static_assert(std::is_default_constructible<T>::value, 
                          "T must be default constructible");

            tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
            auto it = _map.find(key);
            if (it == _map.end()) {
                it = _map.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::tuple<>{}).first;
            }
            return it->second;
        }

        // Set key to value, returns true if the key was newly inserted
        // T must have copy assignment operator
        bool set(const Key& key, T& val)
        {
            static_assert(std::is_copy_assignable<T>::value, 
                          "T must be copy-assignable");

            tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
            if (_map.size() > 0) {
                auto it = _map.find(key);
                if (it != _map.end()) {
                    it->second = val;
                    return false;
                }
            }
            _map.insert({key, val});
            return true;
        }

        // Iterate the map with a non-const value reference under exclusive lock
        void iterate(std::function<void(const Key&, T&)> fn)
        {
            // exclusive lock
            tbb::spin_rw_mutex::scoped_lock lock(_mutex, true);
            for (std::pair<const Key, T>& p : _map) {
                fn(p.first, p.second);
            }
        }

        // Iterate the map with a const value reference under shared lock
        void citerate(std::function<void(const Key&, const T&)> fn) const
        {
            tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
            for (const std::pair<const Key, const T>& p : _map) {
                fn(p.first, p.second);
            }
        }

        // Gives the count of keys currently in the map
        size_t size() const
        {
            tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
            return _map.size();
        }

        // Erase the given key from the map under exclusive lock
        void erase(const Key& key)
        {
            // exclusive lock
            tbb::spin_rw_mutex::scoped_lock lock(_mutex, true);
            _map.unsafe_erase(key);
        }

        // Clear all map entries under exclusive lock
        void clear()
        {
            // exclusive lock
            tbb::spin_rw_mutex::scoped_lock lock(_mutex, true);
            _map.clear();
        }
    private:
        tbb::concurrent_unordered_map<Key, T, Hash, KeyEqual> _map;
        mutable tbb::spin_rw_mutex _mutex;
    };

    using _LockingFlattenGroupMap = _LockingMap<
        _FlattenData,
        riley::GeometryPrototypeId,
        _FlattenData::HashFunctor>;

    struct _RileyInstanceId
    {
        riley::GeometryPrototypeId groupId;
        riley::GeometryInstanceId geoInstanceId;
        riley::LightInstanceId lightInstanceId;
    };

    using _InstanceIdVec = std::vector<_RileyInstanceId>;
    
    struct _ProtoIdHash
    {
        size_t operator()(const riley::GeometryPrototypeId& id) const noexcept
        { 
            return std::hash<uint32_t>()(id.AsUInt32());
        }
    };

    using _ProtoInstMap = std::unordered_map<
        riley::GeometryPrototypeId,
        _InstanceIdVec,
        _ProtoIdHash>;
    
    using _LockingProtoGroupCounterMap = _LockingMap<
        riley::GeometryPrototypeId,
        std::atomic<int>,
        _ProtoIdHash>;

    struct _ProtoMapEntry
    {
        _ProtoInstMap map;
        bool dirty;
    };

    using _LockingProtoMap = _LockingMap<SdfPath, _ProtoMapEntry, SdfPath::Hash>;

    // **********************************************
    // **             Private Methods              **
    // **********************************************    

    // Sync helper; caches instance-rate primvars
    void _SyncPrimvars(HdDirtyBits* dirtyBits);

    // Sync helper; caches the instancer and instance transforms
    void _SyncTransforms(HdDirtyBits* dirtyBits);

    // Sync helper; caches instance or instancer categories as appropriate
    void _SyncCategories(HdDirtyBits* dirtyBits);

    // Sync helper; caches instancer visibility
    void _SyncVisibility(HdDirtyBits* dirtyBits);

    // Generates InstanceData structures for this instancer's instances;
    // will multiply those by any supplied subInstances
    void _ComposeInstances(
        const SdfPath& protoId,
        const std::vector<_InstanceData>& subInstances,
        std::vector<_InstanceData>& instances);

    // Generates FlattenData from a set of instance params by looking for
    // incompatible params and moving them from the RtParamList to the
    // FlattenData. Called by _ComposeInstances().
    void _ComposeInstanceFlattenData(
        const size_t instanceId,
        RtParamList& instanceParams,
        _FlattenData& fd,
        const _FlattenData& fromBelow = _FlattenData());

    // Generates param sets and flatten data for the given prototype
    // prim(s). Starts with copies of the prototype params provided to
    // Populate, and additionally captures constant/uniform params inherited
    // by the prototype, prototype- and subset-level light linking, and subset
    // visibility.
    void _ComposePrototypeData(
        const SdfPath& protoPath,
        const RtParamList& globalProtoParams,
        const bool isLight,
        const std::vector<riley::GeometryPrototypeId>& protoIds,
        const SdfPathVector& subProtoPaths,
        const std::vector<_FlattenData>& subProtoFlats,
        std::vector<RtParamList>& protoParams,
        std::vector<_FlattenData>& protoFlats,
        std::vector<TfToken>& protoRenderTags);

    // Deletes riley instances owned by this instancer that are of riley
    // geometry prototypes that are no longer associated with the given
    // prototype prim. Returns true if there are any new riley geometry
    // prototype ids to associate with this prototype prim path.
    bool _RemoveDeadInstances(
        riley::Riley* riley,
        const SdfPath& prototypePrimPath,
        const std::vector<riley::GeometryPrototypeId>& protoIds);

    // Flags all previously seen prototype prim paths as needing their instances
    // updated the next time they show up in a Populate call.
    void _SetPrototypesDirty();

    // Generates instances of the given prototypes according to the instancer's
    // instancing configuration. If the instancer is too deep, they get passed
    // up to this method on the parent instancer. The given prototypes may be
    // prims (when called through the public Populate method) or may be
    // child instancers represented by riley geometry prototype groups. In
    // either case, the caller owns the riley prototypes.
    void _PopulateInstances(
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits,
        const SdfPath& hydraPrototypeId,
        const SdfPath& prototypePrimPath,
        const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds,
        const riley::CoordinateSystemList& coordSysList,
        const RtParamList protoParams,
        const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> protoXform,
        const std::vector<riley::MaterialId>& rileyMaterialIds,
        const SdfPathVector& prototypePaths,
        const riley::LightShaderId& lightShaderId,
        const std::vector<_InstanceData>& subInstances,
        const std::vector<_FlattenData>& prototypeFlats);

    // Locks before calling _PopulateInstances() to prevent duplicated Riley
    // calls that may arise when Populate() has been called from multiple
    // threads producing identical population requests from the same child
    // instancer. Locks are segregated by prototypePrimPath to avoid
    // over-locking.
    void _PopulateInstancesFromChild(
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits,
        const SdfPath& hydraPrototypeId,
        const SdfPath& prototypePrimPath,
        const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds,
        const riley::CoordinateSystemList& coordSysList,
        const RtParamList protoParams,
        const HdTimeSampleArray<GfMatrix4d, HDPRMAN_MAX_TIME_SAMPLES> protoXform,
        const std::vector<riley::MaterialId>& rileyMaterialIds,
        const SdfPathVector& prototypePaths,
        const riley::LightShaderId& lightShaderId,
        const std::vector<_InstanceData>& subInstances,
        const std::vector<_FlattenData>& prototypeFlats);

    // Get pointer to parent instancer, if one exists
    HdPrmanInstancer* _GetParentInstancer();
    
    // Resize the instancer's interal state store for tracking riley instances.
    // Shrinking the number of instances for a given prototype path and id will
    // delete excess instances from riley. Call with newSize = 0 to kill 'em all.
    void _ResizeProtoMap(
        riley::Riley* riley,
        const SdfPath& prototypePrimPath, 
        const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds, 
        const size_t newSize);

    // Deletes any riley geometry prototype groups that are no longer needed.
    // Returns true if any groups were deleted.
    bool _CleanDisusedGroupIds(HdPrman_RenderParam* param);

    // Obtain the riley geometry prototype group id for a given FlattenData.
    // Returns true if the group had to be created. Gives InvalidId when this
    // instancer has no parent instancer.
    bool _AcquireGroupId(
        HdPrman_RenderParam* param,
        const _FlattenData& flattenGroup,
        riley::GeometryPrototypeId& groupId);

    // Retrieves instance-rate params for the given instance index from
    // the instancer's cache.
    void _GetInstanceParams(
        const size_t instanceIndex,
        RtParamList& params);

    // Gets constant and uniform params for the prototype
    void _GetPrototypeParams(
        const SdfPath& protoPath,
        RtParamList& params
    );

    // Retrieves the instance transform for the given index from the
    // instancer's cache.
    void _GetInstanceTransform(
        const size_t instanceIndex,
        _GfMatrixSA& xform,
        const _GfMatrixSA& left = _GfMatrixSA());

    // Calculates this instancer's depth in the nested instancing hierarchy.
    // An uninstanced instancer has depth 0. Instancers with depth > 4 cannot
    // use riley nested instancing and must flatten their instances into their
    // parents.
    int _Depth();


    // **********************************************
    // **             Private Members              **
    // **********************************************   

    // This instancer's cached instance transforms
    HdTimeSampleArray<VtMatrix4dArray, HDPRMAN_MAX_TIME_SAMPLES> _sa;

    // This instancer's cached coordinate system list
    riley::CoordinateSystemList _coordSysList = { 0, nullptr };

    // This instancer's cached instance categories; will be empty under point
    // instancing, so all indexing must be bounds-checked!
    std::vector<VtTokenArray> _instanceCategories;

    // This instancer's cached visibility and categories
    _FlattenData _instancerFlat;

    // This instancer's cached USD primvars
    TfHashMap<TfToken, _PrimvarValue, TfToken::HashFunctor> _primvarMap;

    // Map of FlattenData to GeometryProtoypeId
    // We use this map to put instances that share values for instance params
    // that are incompatible with riley nesting into shared prototype groups so
    // that the incompatible params may be set on the outermost riley
    // instances of those groups where they are supported. This map may be
    // written to during Populate, so access must be gated behind a mutex
    // lock (built into LockingMap).
    _LockingFlattenGroupMap _groupMap;

    // Counters for tracking number of instances in each prototype group. Used
    // to speed up empty prototype group removal.
    _LockingProtoGroupCounterMap _groupCounters;

    // riley geometry prototype groups are created during Populate; these must
    // be serialized to prevent creating two different groups for the same set
    // of flatten data.
    tbb::spin_rw_mutex _groupIdAcquisitionLock;
    
    // Main storage for tracking riley instances owned by this instancer.
    // Instance ids are paired with their containing group id (RileyInstanceId),
    // then grouped by their riley geometry prototype id (ProtoInstMap). These
    // are then grouped by id of the prototype prim they represent (which may be
    // the invalid id in the case of analytic lights). The top level of this
    // nested structure may be written to during Populate, therefore access to
    // the top level is gated behind a mutex lock (built into LockingMap).
    // Deeper levels are only ever written to from within a single call to
    // Populate, so they do not have gated access.
    _LockingProtoMap _protoMap;

    // Locks used by _PopulateInstancesFromChild() to serialize (and dedupe)
    // parallel calls from the same child instancer, which occur when the child
    // has multiple prototype prims and would otherwise lead to duplicated
    // Riley calls to Create or Remove instances, both of which are problematic.
    _LockingMap<SdfPath, tbb::spin_rw_mutex, SdfPath::Hash> _childPopulateLocks;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INSTANCER_H
