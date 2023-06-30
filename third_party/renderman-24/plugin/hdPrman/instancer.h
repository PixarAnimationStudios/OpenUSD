//
// Copyright 2023 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INSTANCER_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INSTANCER_H

#include "pxr/pxr.h"

#include "hdPrman/debugCodes.h"
#include "hdPrman/debugUtil.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/rixStrings.h"

#include "pxr/imaging/hd/instancer.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/timeSampleArray.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"

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
     * @param hydraPrototypeId The path of the hydra prototype.
     * @param rileyPrototypeIds The riley prototype ids associated with this
     *        hydra prim. There may be more than one, in the case of geomSubsets.
     * @param coordSysList The coordinate system list for the hydra prototype.
     * @param rileyPrimId The riley prim id; used for identification.
     * @param rileyMaterialIds The riley material ids to be assigned to each
     *        riley prototype; this should match rileyPrototypeIds in length.
     * @param prototypePaths The stage paths of the (sub)prims each prototype
     *        id represents, e.g., the stage paths to the geomSubsets; this 
     *        should always match prototypeIds in length. These are used for
     *        identification purposes and, when different from hydraPrototypeId,
     *        for retrieving prototype-level attributes and light-linking
     *        categories, so they should (ideally) not be proxy paths.
     */
    void Populate(
        HdRenderParam* renderParam,
        HdDirtyBits* dirtyBits,
        const SdfPath& hydraPrototypeId,
        const std::vector<riley::GeometryPrototypeId>& rileyPrototypeIds,
        const riley::CoordinateSystemList& coordSysList,
        const int32_t rileyPrimId,
        const std::vector<riley::MaterialId>& rileyMaterialIds,
        const SdfPathVector& prototypePaths
    );

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
        // a single boolean would clobber any renderer-specific attrs that might
        // have been authored on a given (native) instance.
        RtParamList visibility;
        
        _FlattenData() {}
        _FlattenData(const VtTokenArray& cats) 
            : categories(cats.begin(), cats.end())
        { }
        _FlattenData(const VtTokenArray& cats, bool vis)
            : categories(cats.begin(), cats.end())
        {
            if (!vis) {
                visibility.SetInteger(RixStr.k_visibility_camera, 0);
                visibility.SetInteger(RixStr.k_visibility_indirect, 0);
                visibility.SetInteger(RixStr.k_visibility_transmission, 0);
            }
        };
        // Copy constructor
        _FlattenData(const _FlattenData& other) 
        : categories(other.categories.begin(), other.categories.end()) {
            visibility.Update(other.visibility);
        }

        // Visibility params that already exist here will not be changed
        void Inherit(const _FlattenData& rhs)
        {
            categories.insert(rhs.categories.begin(), rhs.categories.end());
            visibility.Inherit(rhs.visibility);
        }

        // Visibility params that already exist here will be changed
        void Update(const _FlattenData& rhs)
        {
            categories.insert(rhs.categories.begin(), rhs.categories.end());
            visibility.Update(rhs.visibility);
        }

        // equals operator
        const bool operator==(const _FlattenData& rhs) const noexcept
        {
            return categories == rhs.categories &&
                   _RtParamListEqualToFunctor()(visibility, rhs.visibility);
        }

        struct HashFunctor {
            size_t operator()(const _FlattenData& fd) const noexcept
            {
                // simple order-independent XOR hash aggregation 
                size_t hash = 0;
                for (const TfToken& tok : fd.categories) {
                    hash ^= tok.Hash();
                }
                return hash ^ _RtParamListHashFunctor()(fd.visibility);
            }
        };
    };

    struct _InstanceData
    {
        _FlattenData flattenData;
        RtParamList params;
        _GfMatrixSA transform;

        _InstanceData() {}
        _InstanceData(
            const VtTokenArray& cats, 
            bool vis, 
            const RtParamList& p, 
            _GfMatrixSA& xform)
            : flattenData(cats, vis),
              transform(xform)
        {
            params.Inherit(p);
        }
    };

    // A simple concurrent hashmap built from std::unordered_map with mutex
    // locking on read and write. Using this instead of tbb's
    // concurrent_hash_map because we need thread-safe erase and clear.
    template<
        typename Key, 
        typename T, 
        typename Hash = std::hash<Key>, 
        typename KeyEqual = std::equal_to<Key>>
    class _LockingMap
    {
    public:
        bool has(const Key& key)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            return _map.find(key) != _map.end();
        }
        // T must have default constructor
        template<class = std::enable_if_t<
            std::is_default_constructible<std::remove_reference_t<T>>::value>>
        T& get(const Key& key)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _map.find(key);
            if (it == _map.end()) {
                it = _map.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::tuple<>{}).first;
            }
            return it->second;
        }
        // T must have copy assignment operator
        template<class = std::enable_if_t<
            std::is_copy_assignable<std::remove_reference_t<T>>::value>>
        bool set(const Key& key, T& val)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _map.find(key);
            if (it == _map.end()) {
                _map.insert({key, val});
                return true;
            }
            it->second = val;
            return false;
        }
        void iterate(std::function<void(const Key&, T&)> fn)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            for (std::pair<const Key, T>& p : _map) {
                fn(p.first, p.second);
            }
        }
        void erase(const Key& key)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _map.erase(key);
        }
        void clear()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _map.clear();
        }
    private:
        std::unordered_map<Key, T, Hash, KeyEqual> _map;
        // XXX: A shared_mutex (C++17) would be preferable here
        std::mutex _mutex;
    };

    using _LockingFlattenGroupMap = _LockingMap<
        _FlattenData,
        riley::GeometryPrototypeId,
        _FlattenData::HashFunctor>;

    struct _RileyInstanceId
    {
        riley::GeometryPrototypeId groupId;
        riley::GeometryInstanceId instanceId;
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

    struct _ProtoMapEntry
    {
        _ProtoInstMap map;
        bool dirty;
    };

    using _LockingProtoMap = _LockingMap<SdfPath, _ProtoMapEntry, SdfPath::Hash>;

    // **********************************************
    // **             Private Methods              **
    // **********************************************    

    // Helper for multiplying transform matrix arrays; this does not really *need*
    // to be a static class method, but it is so it can use the (private)
    // shortened type names.
    static void _MultiplyTransforms(
        const _GfMatrixSA& lhs,
        const _GfMatrixSA& rhs,
        _RtMatrixSA& dest);

    // List of instance attributes pertaining to light-linking that are not
    // supported on instances inside geometry prototype groups
    static std::vector<RtUString> _GetLightLinkAttrs()
    {
        static const std::vector<RtUString> LightLinkAttrs = {
            RixStr.k_lightfilter_subset,
            RixStr.k_lighting_subset,
            RixStr.k_grouping_membership,
            RixStr.k_lighting_excludesubset
        };
        return LightLinkAttrs;
    }

    // List of instance attributes pertaining to visibility that are not
    // supported on instances inside geometry prototype groups
    static std::vector<RtUString> _GetVisAttrs()
    {
        static const std::vector<RtUString> VisAttrs = {
            RixStr.k_visibility_camera,
            RixStr.k_visibility_indirect,
            RixStr.k_visibility_transmission
        };
        return VisAttrs;
    }

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
        const int primId,
        const std::vector<_InstanceData> subInstances,
        std::vector<_InstanceData>& instances);

    // Generates FlattenData from a set of instance attributes by looking for
    // incompatible attributes and moving them from the RtParamList to the
    // FlattenData. Called by _ComposeInstances().
    void _ComposeInstanceFlattenData(
        const size_t instanceId,
        RtParamList& instanceParams,
        _FlattenData& fd,
        const _FlattenData& fromBelow = _FlattenData());

    // Generates transform and relevant instance attribute sets for the given
    // prototype prim(s). Captures prototype-level primvars, constant/uniform
    // primvars inherited by the prototype, and prototype-level light linking.
    void _ComposePrototypeData(
        HdPrman_RenderParam* param,
        const SdfPath& protoPath,
        const std::vector<riley::GeometryPrototypeId>& protoIds,
        const SdfPathVector& subProtoPaths,
        const std::vector<_FlattenData>& subProtoFlats,
        std::vector<RtParamList>& protoAttrs,
        std::vector<_FlattenData>& protoFlats,
        _GfMatrixSA& protoXform
    );

    // Deletes riley instances owned by this instancer that are of riley
    // geometry prototypes that are no longer associated with the given
    // prototype prim. Returns true if there are any new riley geometry
    // prototype ids to associate with this prototype prim path.
    bool _RemoveDeadInstances(
        riley::Riley* riley,
        const SdfPath& prototypePrimPath,
        const std::vector<riley::GeometryPrototypeId>& protoIds
    );

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
        const int32_t rileyPrimId,
        const std::vector<riley::MaterialId>& rileyMaterialIds,
        const SdfPathVector& prototypePaths,
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

    // Obtain the riley geometry prototype group for a given set of FlattenData.
    // Returns true if the group had to be created. Gives InvalidId when this
    // instancer has no parent instancer.
    bool _AcquireGroupId(
        HdPrman_RenderParam* param,
        const _FlattenData& flattenGroup,
        riley::GeometryPrototypeId& groupId);

    // Retrieves instance-rate primvars for the given instance index from
    // the instancer's cache.
    void _GetInstancePrimvars(
        const size_t instanceIndex,
        RtParamList& attrs);

    // Gets constant and uniform primvars for the prototype
    void _GetPrototypePrimvars(
        const SdfPath& protoPath,
        RtParamList& attrs
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

    // This instancer's cached instance-rate primvars
    TfHashMap<TfToken, _PrimvarValue, TfToken::HashFunctor> _primvarMap;

    // Map of FlattenData to GeometryProtoypeId
    // We use this map to put instances that share values for instance attributes
    // that are incompatible with riley nesting into shared prototype groups so
    // that the incompatible attributes may be set on the outermost riley
    // instances of those groups where they are supported. This map may be
    // written to during Populate, so access must be gated behind a mutex
    // lock (built into LockingMap).
    _LockingFlattenGroupMap _groupMap;

    // riley geometry prototype groups are created during Populate; these must
    // be serialized to prevent creating two different groups for the same set
    // of flatten data.
    std::mutex _groupIdAcquisitionLock;
    
    // Main storage for tracking riley instances owned by this instancer.
    // Instance ids are paired with their containing group id (RileyInstanceId),
    // then grouped by their riley geometry prototype id (ProtoInstMap). These
    // are then grouped by id of the prototype prim they represent. The top
    // level of this nested structure may be written to during Populate,
    // therefore access to the top level is gated behind a mutex lock (built 
    // into LockingMap). Deeper levels are only ever written to from within a
    // single call to Populate, so they do not have gated access.
    _LockingProtoMap _protoMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_INSTANCER_H
