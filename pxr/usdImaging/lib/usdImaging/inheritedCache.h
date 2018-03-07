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
#ifndef USDIMAGING_INHERITEDCACHE_H
#define USDIMAGING_INHERITEDCACHE_H

/// \file usdImaging/inheritedCache.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/sdf/path.h"

#include <boost/functional/hash.hpp>
#include <tbb/concurrent_unordered_map.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImaging_InheritedCache
///
/// A general caching mechanism for attributes inherited up or down the ancestor
/// chain.
///
/// This class is thread safe following the basic guarantee that calling const
/// methods are thread safe, non-const methods are not.
///
/// This cache is generalized based on a strategy object which dictates what
/// value type it will hold along with a "query" object, which can be as simple
/// as a UsdObject or in the case of Xform cache, we use something more fancy, a
/// UsdGeomXformable::XformQuery. This cache is thread safe and lock free. It is
/// not wait free, however waits are expected to be extremely short (a small
/// number of cycles).
///
/// An optional supplemental cache may be used for computing the values to be 
/// cached, if necessary. This cache is passed along to the MakeQuery() method 
/// of the strategy object for use in computations. If MakeQuery() is expected 
/// to modify the supplemental-cache object in any way, care must be taken to 
/// ensure that the modifications are thread-safe. The fallback supplemental 
/// cache type is bool, which it's not used.
/// 
template<typename Strategy, typename SupplementalCache=bool>
class UsdImaging_InheritedCache
{
    friend Strategy;
    struct _Entry;
    typedef tbb::concurrent_unordered_map<UsdPrim,
                                          _Entry,
                                          boost::hash<UsdPrim> > _CacheMap;
public:
    typedef typename Strategy::value_type value_type;
    typedef typename Strategy::query_type query_type;

    typedef TfHashMap<UsdPrim, value_type, boost::hash<UsdPrim> > 
        ValueOverridesMap;

    /// Construct a new for the specified \p time and \p rootPath, at which
    /// all traversals will terminate. Note that values authored on the 
    /// \p rootPath itself are not inherited.
    explicit UsdImaging_InheritedCache(
        const UsdTimeCode time, 
        const SdfPath& rootPath,
        SupplementalCache *supplCache=nullptr,
        const ValueOverridesMap valueOverrides=ValueOverridesMap())
        : _time(time)
        , _rootPath(rootPath)
        , _cacheVersion(_GetInitialCacheVersion())
        , _valueOverrides(valueOverrides)
        , _supplCache(supplCache)
    {
    }

    /// Construct a new cache for UsdTimeCode::Default() and the absolute root
    /// path.
    UsdImaging_InheritedCache()
        : _time(UsdTimeCode::Default())
        , _rootPath(SdfPath::AbsoluteRootPath())
        , _cacheVersion(1)
    {
    }

    /// Compute the inherited value for the given \p prim, including the value
    /// authored on the Prim itself, if present.
    value_type GetValue(const UsdPrim& prim) const
    {
        TRACE_FUNCTION();
        if (!prim.GetPath().HasPrefix(_rootPath) 
            && !prim.IsInMaster()) {
            TF_CODING_ERROR("Attempt to get value for: %s "
                            "which is not within the specified root: %s",
                            prim.GetPath().GetString().c_str(),
                            _rootPath.GetString().c_str());
            return Strategy::MakeDefault();
        }

        return *_GetValue(prim);
    }

    /// Returns the underlying query object for the given prim. If the prim has
    /// no cache entry, calling this method will trigger the entry to be
    /// populated in an invalid state, but will return a valid query object.
    query_type const*
    GetQuery(const UsdPrim& prim) const {
        return &_GetCacheEntryForPrim(prim)->query;
    }

    /// Clears all pre-cached values.
    void Clear() {
        _cache.clear();
        _cacheVersion = _GetInitialCacheVersion();
    }

    /// Use the new \p time when computing values and may clear any existing
    /// values cached for the previous time. Setting \p time to the current time
    /// is a no-op.
    void SetTime(UsdTimeCode time) {
        if (time == _time) 
            return;

        // Mark all cached entries as invalid, but leave the queries behind.
        // We increment by 2 here and always keep the version an odd number,
        // this enables the use of even versions as a per-entry spin lock.
        _cacheVersion += 2;

        // Update to correct time.
        _time = time;
    }

    /// Get the current time from which this cache is reading values.
    UsdTimeCode GetTime() const { return _time; }

    /// Set the root ancestor path at which to stop inheritance.
    /// Note that values on the root are not inherited.
    void SetRootPath(const SdfPath& rootPath) {
        if (!rootPath.IsAbsolutePath()) {
            TF_CODING_ERROR("Invalid root path: %s", 
                            rootPath.GetString().c_str());
            return;
        }

        if (rootPath == _rootPath) 
            return;

        Clear(); 
        _rootPath = rootPath;
    }

    /// Return the root ancestor path at which to stop inheritance.
    /// Note that values on the root are not inherited.
    const SdfPath & GetRootPath() const { return _rootPath; }

    /// Helper function used to append, update or remove overrides from the 
    /// internal value overrides map. By doing the updates to the map in a 
    /// single pass, we can optimize the dirtying of the cache entries.
    /// 
    /// \p valueOverrides contains the set of value overrides to be appended 
    /// or updated in the internal value overrides map. 
    /// \p overriesToRemove contains the list of prims for which overrides 
    /// must be removed. 
    /// \p dirtySubtreeRoots is populated with the list of paths to the roots 
    /// of the subtrees that must be recomputed.
    void UpdateValueOverrides(const ValueOverridesMap &valueOverrides,
                              const std::vector<UsdPrim> &overridesToRemove,
                              std::vector<SdfPath> *dirtySubtreeRoots) 
    {
        TRACE_FUNCTION();

        if (valueOverrides.empty() && overridesToRemove.empty())
            return;

        ValueOverridesMap valueOverridesToProcess;
        SdfPathVector processedOverridePaths;
        TF_FOR_ALL(it, valueOverrides) {
            const UsdPrim &prim = it->first;
            const value_type &value = it->second;

            // If the existing value matches the incoming value, skip 
            // the update and dirtying.
            if (*_GetValue(prim) == value)
                continue;

            valueOverridesToProcess[prim] = value;
        }

        TF_FOR_ALL(it, valueOverridesToProcess) {
            const UsdPrim &prim = it->first;
            const value_type &value = it->second;

            // XXX: performance
            // We could probably make this faster by using a hash table of 
            // prefixes. This hasn't showed up in traces much though as it's not
            // common to update value overrides for more than one path at a 
            // time.
            bool isDescendantOfProcessedOverride = false;
            for (const SdfPath &processedPath : processedOverridePaths) {
                if (prim.GetPath().HasPrefix(processedPath)) {
                    isDescendantOfProcessedOverride = true;
                    break;
                }
            }

            // Invalidate cache entries if the prim is not a descendant of a 
            // path that has already been processed.
            if (!isDescendantOfProcessedOverride) {
                for (UsdPrim descendant: UsdPrimRange(prim)) {
                    if (_Entry* entry = _GetCacheEntryForPrim(descendant)) {
                        entry->version = _GetInvalidVersion();
                    }
                }
                processedOverridePaths.push_back(prim.GetPath());
                dirtySubtreeRoots->push_back(prim.GetPath());
            }

            // Update overrides in the internal value overrides map.
            _valueOverrides[prim] = value;
        }

        for (const UsdPrim &prim : overridesToRemove) {

            // Erase the entry from the map of overrides.
            size_t numErased = _valueOverrides.erase(prim);

            // If the override doesn't exist, then there's nothing to do.
            if (numErased == 0) {
                continue; 
            }

            bool isDescendantOfProcessedOverride = false;
            for (const SdfPath &processedPath : processedOverridePaths) {
                if (prim.GetPath().HasPrefix(processedPath)) {
                    isDescendantOfProcessedOverride = true;
                    break;
                }
            }

            // Invalidate cache entries if the prim is not a descendant of a 
            // path that has already been processed.
            if (!isDescendantOfProcessedOverride) {
                for (UsdPrim descendant: UsdPrimRange(prim)) {
                    if (_Entry* entry = _GetCacheEntryForPrim(descendant)) {
                        entry->version = _GetInvalidVersion();
                    }
                }
                dirtySubtreeRoots->push_back(prim.GetPath());
                processedOverridePaths.push_back(prim.GetPath());
            }
        }        
    }

private:
    // Cached value entries. Note that because query objects may be caching
    // non-time varying data, entries may exist in the cache with invalid
    // values. The version is used to determine validity.
    struct _Entry {
        _Entry()
            : value(Strategy::MakeDefault())
            , version(_GetInitialEntryVersion()) 
        { }

        _Entry(const query_type & query_,
               const value_type& value_,
               unsigned version_)
            : query(query_)
            , value(value_)
            , version(version_)
        { }

        query_type query;
        value_type value;
        tbb::atomic<unsigned> version;
    };

    // Returns the version number for a valid cache entry
    unsigned _GetValidVersion() const { return _cacheVersion + 1; }

    // Returns the version number for an invalid cache entry
    unsigned _GetInvalidVersion() const { return _cacheVersion - 1; }

    // Initial version numbers
    static unsigned _GetInitialCacheVersion() { return 1; }
    static unsigned _GetInitialEntryVersion() { 
        return _GetInitialCacheVersion()-1;
    }

    // Traverse the hierarchy (order is strategy dependent) and compute the
    // inherited value. 
    value_type const* _GetValue(const UsdPrim& prim) const;

    // Helper function to get or create a new entry for a prim in the cache.
    _Entry* _GetCacheEntryForPrim(const UsdPrim &prim) const;

    // Sets the value of the given cache entry. If multiple threads attempt to
    // set the same entry, the first in wins and other threads spin until the
    // new value is set.
    void _SetCacheEntryForPrim(const UsdPrim &prim, 
                               value_type const& value,
                               _Entry* entry) const;

    // Mutable is required here to allow const methods to update the cache when
    // it is thread safe, however not all mutations of this map are thread safe.
    // See underlying map documentation for details.
    mutable _CacheMap _cache;
    
    // The time at which this stack is querying and caching attribute values.
    UsdTimeCode _time;
    SdfPath _rootPath;

    // A serial number indicating the valid state of entries in the cache. When
    // an entry has an equal or greater value, the entry is valid.
    tbb::atomic<unsigned> _cacheVersion;

    // Value overrides for a set of descendents.
    ValueOverridesMap _valueOverrides;

    // Supplemental cache if used by this inherited cache.
    SupplementalCache *_supplCache;
};

template<typename Strategy, typename SupplementalCache>
void
UsdImaging_InheritedCache<Strategy,SupplementalCache>::_SetCacheEntryForPrim(
    const UsdPrim &prim,
    value_type const& value,
    _Entry* entry) const
{
    // Note: _cacheVersion is not allowed to change during cache access.
    unsigned v = entry->version;
    if (v < _cacheVersion 
        && entry->version.compare_and_swap(_cacheVersion, v) == v)
    {
        entry->value = value;
        entry->version = _GetValidVersion();
    } else {
        while (entry->version != _GetValidVersion()) {
            // Future work: A suggestion is that rather than literally spinning 
            // here, we should use the pause instruction, which sleeps for one
            // cycle while allowing hyper threads to continue. Folly has a nice
            // implementation of this packaged up as "sleeper", which we could
            // also implement in Work and Arch.
        }
    }
}

template<typename Strategy, typename SupplementalCache>
typename UsdImaging_InheritedCache<Strategy, SupplementalCache>::_Entry*
UsdImaging_InheritedCache<Strategy, SupplementalCache>::_GetCacheEntryForPrim(
    const UsdPrim &prim) const
{
    typename _CacheMap::const_iterator it = _cache.find(prim);
    if (it != _cache.end()) {
        return &it->second;
    }
     
    _Entry e;
    e.query = Strategy::MakeQuery(prim, _supplCache);
    e.value = Strategy::MakeDefault();
    e.version = _GetInvalidVersion();
    return &(_cache.insert(
                        typename _CacheMap::value_type(prim, e)).first->second);
}

template<typename Strategy, typename SupplementalCache>
typename UsdImaging_InheritedCache<Strategy, SupplementalCache>::value_type const*
UsdImaging_InheritedCache<Strategy, SupplementalCache>::_GetValue(
    const UsdPrim& prim) const
{
    static value_type const default_ = Strategy::MakeDefault();

    // Base case.
    if (!prim || prim.IsMaster() || prim.GetPath() == _rootPath)
        return &default_;

    _Entry* entry = _GetCacheEntryForPrim(prim);
    if (entry->version == _GetValidVersion()) {
        // Cache hit
        return &entry->value;
    }

    // Future work: Suggestion is that when multiple threads are computing the
    // same value, we could block all but one thread here, possibly rescheduling
    // blocked threads as continuations, rather than allowing all threads to
    // continue to race until a cache hit is encountered.

    // Future work: A suggestion is that we make this iterative instead of
    // recursive.
    typename ValueOverridesMap::const_iterator it =
        _valueOverrides.find(prim);
    if (it != _valueOverrides.end()) {
        _SetCacheEntryForPrim(prim, it->second, entry);
    } else {
        _SetCacheEntryForPrim(prim,
                              Strategy::Inherit(this, prim, &entry->query), 
                              entry);
    }
    return &entry->value;
}

// -------------------------------------------------------------------------- //
// Xform Cache
// -------------------------------------------------------------------------- //

PXR_NAMESPACE_CLOSE_SCOPE

#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_XfStrategy;
typedef UsdImaging_InheritedCache<UsdImaging_XfStrategy> UsdImaging_XformCache;

struct UsdImaging_XfStrategy {
    typedef GfMatrix4d value_type;
    typedef UsdGeomXformable::XformQuery query_type;

    static
    value_type MakeDefault() { return GfMatrix4d(1); }

    static
    query_type MakeQuery(UsdPrim prim, bool *) {
        if (UsdGeomXformable xf = UsdGeomXformable(prim))
            return query_type(xf);
        return query_type();
    }

    static 
    value_type
    Inherit(UsdImaging_XformCache const* owner, 
            UsdPrim prim,
            query_type const* query)
    { 
        value_type xform = MakeDefault();
        // No need to check query validity here because XformQuery doesn't
        // support it.
        query->GetLocalTransformation(&xform, owner->GetTime());

        return !query->GetResetXformStack() 
                ? (xform * (*owner->_GetValue(prim.GetParent())))
                : xform;
    }

    // Compute the full transform, this is not part of the interface required by
    // the cache.
    static
    value_type
    ComputeTransform(UsdPrim const& prim, 
            SdfPath const& rootPath, 
            UsdTimeCode time, 
            const TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> &ctmOverrides)
    {
        bool reset = false;
        GfMatrix4d ctm(1.0);
        GfMatrix4d localXf(1.0);
        UsdPrim p = prim;
        while (p && p.GetPath() != rootPath) {
            const auto &overIt = ctmOverrides.find(p.GetPath());
            // If there's a ctm override, use it and break out of the loop.
            if (overIt != ctmOverrides.end()) {
                ctm *= overIt->second;
                break;
            } else if (UsdGeomXformable xf = UsdGeomXformable(p)) {
                if (xf.GetLocalTransformation(&localXf, &reset, time))
                    ctm *= localXf;
                if (reset)
                    break;
            }
            p = p.GetParent();
        }
        return ctm;
    }
};

// -------------------------------------------------------------------------- //
// Visibility Cache
// -------------------------------------------------------------------------- //

PXR_NAMESPACE_CLOSE_SCOPE

#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/base/tf/token.h"
#include "pxr/usdImaging/usdImaging/debugCodes.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_VisStrategy;
typedef UsdImaging_InheritedCache<UsdImaging_VisStrategy> UsdImaging_VisCache;

struct UsdImaging_VisStrategy {
    typedef TfToken value_type; // invisible, inherited
    typedef UsdAttributeQuery query_type;

    static
    value_type MakeDefault() { return UsdGeomTokens->inherited; }

    static
    query_type MakeQuery(UsdPrim prim, bool *) {
        if (UsdGeomImageable xf = UsdGeomImageable(prim))
            return query_type(xf.GetVisibilityAttr());
        return query_type();
    }

    static 
    value_type
    Inherit(UsdImaging_VisCache const* owner, 
            UsdPrim prim,
            query_type const* query)
    { 
        value_type v = *owner->_GetValue(prim.GetParent());
        if (v == UsdGeomTokens->invisible)
            return v;
        if (*query)
            query->Get(&v, owner->GetTime());
        return v;
    }

    static
    value_type
    ComputeVisibility(UsdPrim const& prim, SdfPath const& rootPath, UsdTimeCode time)
    {
        return UsdGeomImageable(prim).ComputeVisibility(time);
    }
};

// -------------------------------------------------------------------------- //
// Hydra MaterialBinding Cache
// -------------------------------------------------------------------------- //

PXR_NAMESPACE_CLOSE_SCOPE

#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_MaterialBindingSupplementalCache {
    UsdShadeMaterialBindingAPI::BindingsCache bindingsCache;
    UsdShadeMaterialBindingAPI::CollectionQueryCache collQueryCache;

    void Clear() {
        bindingsCache.clear();
        collQueryCache.clear();
    }
};

struct UsdImaging_MaterialStrategy;
typedef UsdImaging_InheritedCache<UsdImaging_MaterialStrategy,
    UsdImaging_MaterialBindingSupplementalCache> 
        UsdImaging_MaterialBindingCache;

struct UsdImaging_MaterialStrategy {
    typedef SdfPath value_type;         // inherited path to bound shader
    typedef UsdShadeMaterial query_type;

    static
    value_type MakeDefault() { return SdfPath(); }

    static
    query_type MakeQuery(
        UsdPrim prim, 
        UsdImaging_MaterialBindingSupplementalCache *supplCache) 
    {
        return UsdShadeMaterialBindingAPI(prim).ComputeBoundMaterial(
                &supplCache->bindingsCache, &supplCache->collQueryCache,
                /*materialPurpose*/ UsdShadeTokens->preview);
    }

    static
    SdfPath GetBinding(UsdShadeMaterial const& material);

    static
    UsdPrim GetTargetedShader(UsdPrim const& materialPrim,
                              UsdRelationship const& materialRel);
 
    static 
    value_type
    Inherit(UsdImaging_MaterialBindingCache const* owner,
            UsdPrim prim,
            query_type const* query)
    { 
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Looking for \"preview\" material "
                "binding for %s\n", prim.GetPath().GetText());
        if (*query) {
            SdfPath binding = GetBinding(*query);
            if (!binding.IsEmpty()) {
                return binding;
            }
        }
        // query already contains the resolved material binding for the prim. 
        // Hence, we don't need to inherit the binding from the parent here. 
        // Futhermore, it may be wrong to inherit the binding from the parent,
        // because in the new scheme, a child of a bound prim can be unbound.
        return value_type();
    }

    static 
    value_type 
    ComputeBoundMaterialForPurpose(UsdPrim const& prim,
                                   TfToken const& materialPurpose) {          
        // We don't need to walk up the namespace here since 
        // ComputeBoundMaterial does it for us.
        if (UsdShadeMaterial mat = UsdShadeMaterialBindingAPI(prim).
                    ComputeBoundMaterial(materialPurpose)) {
            return GetBinding(mat);
        }

        return value_type();
    }

    static
    value_type
    ComputeMaterialPath(UsdPrim const& prim) {
        return ComputeBoundMaterialForPurpose(prim, UsdShadeTokens->preview);
    }
};

// -------------------------------------------------------------------------- //
// MaterialBinding Cache for full material networks
// -------------------------------------------------------------------------- //

struct UsdImaging_MaterialNetworkStrategy;
typedef UsdImaging_InheritedCache<UsdImaging_MaterialNetworkStrategy,
        UsdImaging_MaterialBindingSupplementalCache> 
    UsdImaging_MaterialNetworkBindingCache;

struct UsdImaging_MaterialNetworkStrategy {
    typedef SdfPath value_type;         // inherited path to bound shader
    typedef UsdShadeMaterial query_type;

    static
    value_type MakeDefault() { return SdfPath(); }

    static
    query_type MakeQuery(
        UsdPrim prim, 
        UsdImaging_MaterialBindingSupplementalCache *supplCache) 
    {
        return UsdShadeMaterialBindingAPI(prim).ComputeBoundMaterial(
            &supplCache->bindingsCache, &supplCache->collQueryCache,
            /*materialPurpose*/ UsdShadeTokens->full);
    }

    static
    SdfPath GetBinding(UsdShadeMaterial const& material);

    static
    value_type
    Inherit(UsdImaging_MaterialNetworkBindingCache const* owner,
            UsdPrim prim,
            query_type const* query)
    { 
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Looking for \"full\" material binding"
                " for %s\n", prim.GetPath().GetText());
        if (*query) {
            SdfPath binding = GetBinding(*query);
            if (!binding.IsEmpty()) {
                return binding;
            }
        }
        return *owner->_GetValue(prim.GetParent());
    }

    static
    value_type
    ComputeMaterialPath(UsdPrim const& prim) {
        return UsdImaging_MaterialStrategy::ComputeBoundMaterialForPurpose(
                prim, UsdShadeTokens->full);
    }
};

// -------------------------------------------------------------------------- //
// ModelDrawMode Cache
// -------------------------------------------------------------------------- //

PXR_NAMESPACE_CLOSE_SCOPE

#include "pxr/usd/usdGeom/modelAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_DrawModeStrategy;
typedef UsdImaging_InheritedCache<UsdImaging_DrawModeStrategy>
    UsdImaging_DrawModeCache;

struct UsdImaging_DrawModeStrategy
{
    typedef TfToken value_type; // origin, bounds, cards, default
    typedef UsdAttributeQuery query_type;

    static
    value_type MakeDefault() { return UsdGeomTokens->default_; }

    static
    query_type MakeQuery(UsdPrim prim, bool *) {
        if (UsdGeomModelAPI m = UsdGeomModelAPI(prim))
            if (UsdAttribute a = m.GetModelDrawModeAttr())
                return query_type(a);
        return query_type();
    }

    static
    value_type
    Inherit(UsdImaging_DrawModeCache const* owner,
            UsdPrim prim,
            query_type const* query)
    {
        value_type v = UsdGeomTokens->default_;
        if (*query) {
            query->Get(&v);
            return v;
        }
        return *owner->_GetValue(prim.GetParent());
    }

    static
    value_type
    ComputeDrawMode(UsdPrim const& prim)
    {
        return UsdGeomModelAPI(prim).ComputeModelDrawMode();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDIMAGING_INHERITEDCACHE_H
