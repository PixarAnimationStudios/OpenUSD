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
#ifndef PXR_USD_IMAGING_USD_IMAGING_INHERITED_CACHE_H
#define PXR_USD_IMAGING_USD_IMAGING_INHERITED_CACHE_H

/// \file usdImaging/resolvedAttributeCache.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/work/utils.h"

#include <boost/functional/hash.hpp>
#include <tbb/concurrent_unordered_map.h>
#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImaging_ResolvedAttributeCache
///
/// A general caching mechanism for attributes that are nontrivial to resolve,
/// such as attributes inherited up or down the ancestor chain or attributes
/// with significant load-time processing involved.
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
/// An optional implementation data (ImplData) object may be used for computing 
/// the values to be cached, if necessary. This object is passed along to the 
/// MakeQuery() method of the strategy object, making it available for use in 
/// computations. If MakeQuery() is expected to modify the ImplData object in 
/// any way, care must be taken to ensure that the modifications are 
/// thread-safe. The fallback type for ImplData is bool, when it's not specified
/// by a cache.
/// 
template<typename Strategy, typename ImplData=bool>
class UsdImaging_ResolvedAttributeCache
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

    /// Construct a new for the specified \p time.
    explicit UsdImaging_ResolvedAttributeCache(
        const UsdTimeCode time,
        ImplData *implData=nullptr,
        const ValueOverridesMap valueOverrides=ValueOverridesMap())
        : _time(time)
        , _rootPath(SdfPath::AbsoluteRootPath())
        , _cacheVersion(_GetInitialCacheVersion())
        , _valueOverrides(valueOverrides)
        , _implData(implData)
    {
    }

    /// Construct a new cache for UsdTimeCode::Default().
    UsdImaging_ResolvedAttributeCache()
        : _time(UsdTimeCode::Default())
        , _rootPath(SdfPath::AbsoluteRootPath())
        , _cacheVersion(1)
    {
    }

    ~UsdImaging_ResolvedAttributeCache()
    {
        WorkSwapDestroyAsync(_cache);
    }


    /// Compute the inherited value for the given \p prim, including the value
    /// authored on the Prim itself, if present.
    value_type GetValue(const UsdPrim& prim) const
    {
        TRACE_FUNCTION();
        if (!prim.GetPath().HasPrefix(_rootPath) 
            && !prim.IsInPrototype()) {
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
        WorkSwapDestroyAsync(_cache);
        _cacheVersion = _GetInitialCacheVersion();
    }

    /// Use the new \p time when computing values and may clear any existing
    /// values cached for the previous time. Setting \p time to the current time
    /// is a no-op.
    void SetTime(UsdTimeCode time) {
        if (time == _time) 
            return;

        if (Strategy::ValueMightBeTimeVarying()) {
            // Mark all cached entries as invalid, but leave the queries behind.
            // We increment by 2 here and always keep the version an odd number,
            // this enables the use of even versions as a per-entry spin lock.
            _cacheVersion += 2;
        }

        // Update to correct time.
        _time = time;
    }

    /// Get the current time from which this cache is reading values.
    UsdTimeCode GetTime() const { return _time; }

    /// Set the root ancestor path at which to stop inheritance.
    /// Note that values on the root are not inherited.
    ///
    /// In general, you shouldn't use this function; USD inherited attribute
    /// resolution will traverse to the pseudo-root, and not doing that in the
    /// cache can introduce subtle bugs. This exists mainly for the benefit of
    /// the transform cache, since UsdImagingDelegate transform resolution
    /// semantics are complicated and special-cased.
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
    /// See notes on SetRootPath.
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
    ImplData *_implData;
};

template<typename Strategy, typename ImplData>
void
UsdImaging_ResolvedAttributeCache<Strategy,ImplData>::_SetCacheEntryForPrim(
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

template<typename Strategy, typename ImplData>
typename UsdImaging_ResolvedAttributeCache<Strategy, ImplData>::_Entry*
UsdImaging_ResolvedAttributeCache<Strategy, ImplData>::_GetCacheEntryForPrim(
    const UsdPrim &prim) const
{
    typename _CacheMap::const_iterator it = _cache.find(prim);
    if (it != _cache.end()) {
        return &it->second;
    }
     
    _Entry e;
    e.query = Strategy::MakeQuery(prim, _implData);
    e.value = Strategy::MakeDefault();
    e.version = _GetInvalidVersion();
    return &(_cache.insert(
                        typename _CacheMap::value_type(prim, e)).first->second);
}

template<typename Strategy, typename ImplData>
typename UsdImaging_ResolvedAttributeCache<Strategy, ImplData>::value_type const*
UsdImaging_ResolvedAttributeCache<Strategy, ImplData>::_GetValue(
    const UsdPrim& prim) const
{
    static value_type const default_ = Strategy::MakeDefault();

    // Base case.
    if (!prim || prim.IsPrototype() || prim.GetPath() == _rootPath)
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
                              Strategy::Compute(this, prim, &entry->query), 
                              entry);
    }
    return &entry->value;
}

PXR_NAMESPACE_CLOSE_SCOPE

// -------------------------------------------------------------------------- //
// Xform Cache
// -------------------------------------------------------------------------- //

#include "pxr/usd/usdGeom/xformable.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_XfStrategy;
typedef UsdImaging_ResolvedAttributeCache<UsdImaging_XfStrategy> UsdImaging_XformCache;

struct UsdImaging_XfStrategy {
    typedef GfMatrix4d value_type;
    typedef UsdGeomXformable::XformQuery query_type;

    static
    bool ValueMightBeTimeVarying() { return true; }
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
    Compute(UsdImaging_XformCache const* owner, 
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

PXR_NAMESPACE_CLOSE_SCOPE

// -------------------------------------------------------------------------- //
// Visibility Cache
// -------------------------------------------------------------------------- //

#include "pxr/usd/usdGeom/imageable.h"
#include "pxr/base/tf/token.h"
#include "pxr/usdImaging/usdImaging/debugCodes.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_VisStrategy;
typedef UsdImaging_ResolvedAttributeCache<UsdImaging_VisStrategy> UsdImaging_VisCache;

struct UsdImaging_VisStrategy {
    typedef TfToken value_type; // invisible, inherited
    typedef UsdAttributeQuery query_type;

    static
    bool ValueMightBeTimeVarying() { return true; }
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
    Compute(UsdImaging_VisCache const* owner, 
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
    ComputeVisibility(UsdPrim const& prim, UsdTimeCode time)
    {
        return UsdGeomImageable(prim).ComputeVisibility(time);
    }
};

// -------------------------------------------------------------------------- //
// Purpose Cache
// -------------------------------------------------------------------------- //

struct UsdImaging_PurposeStrategy;
typedef UsdImaging_ResolvedAttributeCache<UsdImaging_PurposeStrategy> 
    UsdImaging_PurposeCache;

struct UsdImaging_PurposeStrategy {
    // For proper inheritance, we need to return the PurposeInfo struct which 
    // stores whether child prims can inherit the parent's computed purpose 
    // when they don't have an authored purpose of their own.
    typedef UsdGeomImageable::PurposeInfo value_type; // purpose, inherited
    typedef UsdAttributeQuery query_type;

    static
    value_type MakeDefault() { 
        // Return the fallback default instead of an empty purpose info.
        return value_type(UsdGeomTokens->default_, false); 
    }

    static
    query_type MakeQuery(UsdPrim prim, bool *) {
        UsdGeomImageable im = UsdGeomImageable(prim);
        return im ? query_type(im.GetPurposeAttr()) : query_type();
    }

    static 
    value_type
    Compute(UsdImaging_PurposeCache const* owner, 
            UsdPrim prim,
            query_type const* query)
    {
        // Fallback to parent if the prim isn't imageable or doesn't have a 
        // purpose attribute. Note that this returns the default purpose if
        // there's no parent prim.
        if (!*query) {
            return *(owner->_GetValue(prim.GetParent()));
        }

        // If the prim has an authored purpose value, we get and use that.
        if (query->HasAuthoredValue()) {
            value_type info;
            query->Get(&info.purpose);
            info.isInheritable = true;
            return info;
        }

        // Otherwise we inherit parent's purpose value, but only if the parent's
        // purpose is inheritable. An inherited purpose is itself inheritable
        // by child prims..
        const value_type *v = owner->_GetValue(prim.GetParent());
        if (v->isInheritable) {
            return *v;
        }

        // Otherwise, get the fallback value. The fallback purpose will not 
        // be inherited by descendants. 
        value_type info;
        query->Get(&info.purpose);
        return info;
    }

    static
    value_type
    ComputePurposeInfo(UsdPrim const& prim)
    {
        return UsdGeomImageable(prim).ComputePurposeInfo();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

// -------------------------------------------------------------------------- //
// Hydra MaterialBinding Cache
// -------------------------------------------------------------------------- //

#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_MaterialBindingImplData {
    /// Constructor takes the purpose for which material bindings are to be 
    /// evaluated.
    UsdImaging_MaterialBindingImplData(const TfToken &materialPurpose):
        _materialPurpose(materialPurpose)
    {   }

    /// Destructor invokes ClearCaches(), which does the cache deletion in 
    /// parallel.
    ~UsdImaging_MaterialBindingImplData() {
        ClearCaches();
    }

    /// Returns the material purpose for which bindings must be computed.
    const TfToken &GetMaterialPurpose() const { 
        return _materialPurpose; 
    }

    /// Returns the BindingsCache object to be used when computing resolved 
    /// material bindings.
    UsdShadeMaterialBindingAPI::BindingsCache & GetBindingsCache() 
    { return _bindingsCache; }

    /// Returns the BindingsCache object to be used when computing resolved 
    /// material bindings.
    UsdShadeMaterialBindingAPI::CollectionQueryCache & GetCollectionQueryCache()
    { return _collQueryCache; }

    /// Clears all of the held caches.
    void ClearCaches();

private:
    const TfToken _materialPurpose;
    UsdShadeMaterialBindingAPI::BindingsCache _bindingsCache;
    UsdShadeMaterialBindingAPI::CollectionQueryCache _collQueryCache;
};

struct UsdImaging_MaterialStrategy;
typedef UsdImaging_ResolvedAttributeCache<UsdImaging_MaterialStrategy,
    UsdImaging_MaterialBindingImplData> 
        UsdImaging_MaterialBindingCache;

struct UsdImaging_MaterialStrategy {
    typedef SdfPath value_type;         // inherited path to bound shader
    typedef UsdShadeMaterial query_type;

    using ImplData = UsdImaging_MaterialBindingImplData;

    static
    bool ValueMightBeTimeVarying() { return false; }
    static
    value_type MakeDefault() { return SdfPath(); }

    static
    query_type MakeQuery(
        UsdPrim prim, 
        ImplData *implData) 
    {
        return UsdShadeMaterialBindingAPI(prim).ComputeBoundMaterial(
                &implData->GetBindingsCache(), 
                &implData->GetCollectionQueryCache(),
                implData->GetMaterialPurpose());
    }
 
    static 
    value_type
    Compute(UsdImaging_MaterialBindingCache const* owner,
            UsdPrim prim,
            query_type const* query)
    { 
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Looking for \"preview\" material "
                "binding for %s\n", prim.GetPath().GetText());
        if (*query) {
            SdfPath binding = query->GetPath();
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
    ComputeMaterialPath(UsdPrim const& prim, ImplData *implData) {
        // We don't need to walk up the namespace here since 
        // ComputeBoundMaterial does it for us.
        if (UsdShadeMaterial mat = UsdShadeMaterialBindingAPI(prim).
                    ComputeBoundMaterial(&implData->GetBindingsCache(), 
                                         &implData->GetCollectionQueryCache(),
                                         implData->GetMaterialPurpose())) {
            return mat.GetPath();
        }
        return value_type();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

// -------------------------------------------------------------------------- //
// ModelDrawMode Cache
// -------------------------------------------------------------------------- //

#include "pxr/usd/usdGeom/modelAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_DrawModeStrategy;
typedef UsdImaging_ResolvedAttributeCache<UsdImaging_DrawModeStrategy>
    UsdImaging_DrawModeCache;

struct UsdImaging_DrawModeStrategy
{
    typedef TfToken value_type; // origin, bounds, cards, default, inherited
    typedef UsdAttributeQuery query_type;

    static
    bool ValueMightBeTimeVarying() { return false; }
    static
    value_type MakeDefault() { return UsdGeomTokens->default_; }

    static
    query_type MakeQuery(UsdPrim prim, bool *) {
        if (UsdAttribute a = UsdGeomModelAPI(prim).GetModelDrawModeAttr())
            return query_type(a);
        return query_type();
    }

    static
    value_type
    Compute(UsdImaging_DrawModeCache const* owner,
            UsdPrim prim,
            query_type const* query)
    {
        // No attribute defined means inherited, means refer to the parent.
        // Any defined attribute overrides parent opinion.
        // If the drawMode is inherited all the way to the root of the scene,
        // that means "default".
        value_type v = UsdGeomTokens->inherited;
        if (*query) {
            query->Get(&v);
        }
        if (v != UsdGeomTokens->inherited) {
            return v;
        }
        v = *owner->_GetValue(prim.GetParent());
        if (v == UsdGeomTokens->inherited) {
            return UsdGeomTokens->default_;
        }
        return v;
    }

    static
    value_type
    ComputeDrawMode(UsdPrim const& prim)
    {
        return UsdGeomModelAPI(prim).ComputeModelDrawMode();
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

// -------------------------------------------------------------------------- //
// UsdGeomPointInstancer indices cache
// -------------------------------------------------------------------------- //

#include "pxr/usd/usdGeom/pointInstancer.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_PointInstancerIndicesStrategy;
typedef UsdImaging_ResolvedAttributeCache<UsdImaging_PointInstancerIndicesStrategy>
    UsdImaging_PointInstancerIndicesCache;

struct UsdImaging_PointInstancerIndicesStrategy
{
    // map from protoIndex -> instanceIndices.
    typedef VtArray<VtIntArray> value_type;
    // We don't use query_type, but can't set it to void.
    typedef int query_type;

    // XXX: Most indices values will be static, but since they *can*
    // be animated, we need to return true here to get invalidation on
    // time-change.  It would be nice to add a per-entry time-varying bit
    // to the resolved cache, instead of having the global per-attribute
    // bit.
    //
    // In this particular case, instance indices are only recomputed when
    // we see "DirtyInstanceIndex" in UpdateForTime, so though we'll be
    // clearing cache entries out of the resolved cache on time-change,
    // we won't actually call out to the attribute cache on static indices.
    static
    bool ValueMightBeTimeVarying() { return true; }
    static
    value_type MakeDefault() { return value_type(); }

    static
    query_type MakeQuery(UsdPrim prim, bool *) {
        return 0;
    }

    static
    value_type
    Compute(UsdImaging_PointInstancerIndicesCache const* owner,
            UsdPrim prim,
            query_type const* query)
    {
        return ComputePerPrototypeIndices(prim, owner->GetTime());
    }

    static
    value_type
    ComputePerPrototypeIndices(UsdPrim const& prim, UsdTimeCode time)
    {
        value_type v;

        UsdGeomPointInstancer pi(prim);
        VtIntArray protoIndices;
        if (!pi.GetProtoIndicesAttr().Get(&protoIndices, time)) {
            TF_WARN("Failed to read point instancer protoIndices");
            return v;
        }

        std::vector<bool> mask = pi.ComputeMaskAtTime(time);

        for (size_t instanceId = 0; instanceId < protoIndices.size(); ++instanceId) {
            size_t protoIndex = protoIndices[instanceId];

            if (protoIndex >= v.size()) {
                v.resize(protoIndex + 1);
            }

            if (mask.size() == 0 || mask[instanceId]) {
                v[protoIndex].push_back(instanceId);
            }
        }

        return v;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

// -------------------------------------------------------------------------- //
// CoordSysBinding Cache
// -------------------------------------------------------------------------- //

#include "pxr/usd/usdShade/coordSysAPI.h"
#include "pxr/imaging/hd/coordSys.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_CoordSysBindingImplData {
    // Helper provided by the scene delegate to pre-convert
    // the binding paths to the equivalent Hydra ID.
    std::function<SdfPath(SdfPath)> usdToHydraPath;
};

struct UsdImaging_CoordSysBindingStrategy;

typedef UsdImaging_ResolvedAttributeCache<
    UsdImaging_CoordSysBindingStrategy,
    UsdImaging_CoordSysBindingImplData>
    UsdImaging_CoordSysBindingCache;

struct UsdImaging_CoordSysBindingStrategy
{
    using ImplData = UsdImaging_CoordSysBindingImplData;

    typedef std::vector<UsdShadeCoordSysAPI::Binding> UsdBindingVec;
    typedef std::shared_ptr<UsdBindingVec> UsdBindingVecPtr;
    typedef std::shared_ptr<SdfPathVector> IdVecPtr;

    struct value_type {
        IdVecPtr idVecPtr;
        UsdBindingVecPtr usdBindingVecPtr;
    };
    struct query_type {
        UsdShadeCoordSysAPI coordSysAPI;
        ImplData *implData;

        // Convert a USD binding relationship to a Hydra ID
        SdfPath
        _IdForBinding(UsdShadeCoordSysAPI::Binding const& binding) const {
            return implData->usdToHydraPath(binding.bindingRelPath);
        }
    };

    static
    bool ValueMightBeTimeVarying() { return false; }

    static
    value_type MakeDefault() {
        return value_type();
    }

    static
    query_type MakeQuery(UsdPrim prim, ImplData *implData) {
        return query_type({ UsdShadeCoordSysAPI(prim), implData });
    }

    static
    value_type
    Compute(UsdImaging_CoordSysBindingCache const* owner,
            UsdPrim prim,
            query_type const* query)
    {
        value_type v;
        if (query->coordSysAPI) {
            // Pull inherited bindings first.
            if (UsdPrim parentPrim = prim.GetParent()) {
                v = *owner->_GetValue(parentPrim);
            }
            // Merge any local bindings.
            if (query->coordSysAPI.HasLocalBindings()) {
                SdfPathVector hdIds;
                UsdBindingVec usdBindings;
                if (v.idVecPtr) {
                    hdIds = *v.idVecPtr;
                }
                if (v.usdBindingVecPtr) {
                    usdBindings = *v.usdBindingVecPtr;
                }
                for (auto const& binding:
                     query->coordSysAPI.GetLocalBindings()) {
                    if (!prim.GetStage()->GetPrimAtPath(
                        binding.coordSysPrimPath).IsValid()) {
                        // The target xform prim does not exist, so ignore
                        // this coord sys binding.
                        TF_WARN("UsdImaging: Ignoring coordinate system "
                                "binding to non-existent prim <%s>\n",
                                binding.coordSysPrimPath.GetText());
                        continue;
                    }
                    bool found = false;
                    for (size_t i=0, n=hdIds.size(); i<n; ++i) {
                        if (usdBindings[i].name == binding.name) {
                            // Found an override -- replace this binding.
                            usdBindings[i] = binding;
                            hdIds[i] = query->_IdForBinding(binding);
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        // New binding, so append.
                        usdBindings.push_back(binding);
                        hdIds.push_back(query->_IdForBinding(binding));
                    }
                }
                v.idVecPtr.reset(new SdfPathVector(hdIds));
                v.usdBindingVecPtr.reset(new UsdBindingVec(usdBindings));
            }
        }
        return v;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

// -------------------------------------------------------------------------- //
// Inherited Primvar Cache
// -------------------------------------------------------------------------- //

#include "pxr/usd/usdGeom/primvarsAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

struct UsdImaging_InheritedPrimvarStrategy;
typedef UsdImaging_ResolvedAttributeCache<UsdImaging_InheritedPrimvarStrategy>
    UsdImaging_InheritedPrimvarCache;

struct UsdImaging_InheritedPrimvarStrategy
{
    struct PrimvarRecord {
        std::vector<UsdGeomPrimvar> primvars;
        bool variable;
    };
    typedef std::shared_ptr<PrimvarRecord> value_type;
    typedef UsdGeomPrimvarsAPI query_type;

    // While primvar data might be time-varying, the set of primvars applying
    // to a prim will not.
    static
    bool ValueMightBeTimeVarying() { return false; }

    static
    value_type MakeDefault() {
        return value_type();
    }

    static
    query_type MakeQuery(UsdPrim prim, bool *) {
        return query_type(UsdGeomPrimvarsAPI(prim));
    }

    static
    value_type Compute(UsdImaging_InheritedPrimvarCache const* owner,
                       UsdPrim prim,
                       query_type const* query)
    {
        value_type v;
        if (*query) {
            // Pull inherited bindings first.
            if (UsdPrim parentPrim = prim.GetParent()) {
                v = *owner->_GetValue(parentPrim);
            }
            // Merge any local bindings.
            std::vector<UsdGeomPrimvar> primvars =
                query->FindIncrementallyInheritablePrimvars(
                    v ? v->primvars : std::vector<UsdGeomPrimvar>());
            if (!primvars.empty()) {
                v = std::make_shared<PrimvarRecord>();
                v->primvars = std::move(primvars);
                v->variable = false;
                for (UsdGeomPrimvar const& pv : v->primvars) {
                    if (pv.ValueMightBeTimeVarying()) {
                        v->variable = true;
                        break;
                    }
                }
            }
        }
        return v;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_INHERITED_CACHE_H
