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
#ifndef USDIMAGING_VALUE_CACHE_H
#define USDIMAGING_VALUE_CACHE_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/pxOsd/subdivTags.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/token.h"

#include <boost/noncopyable.hpp>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>


/// \class UsdImagingValueCache
///
/// A heterogeneous value container without type erasure.
///
class UsdImagingValueCache : boost::noncopyable {
public:
    typedef PxOsdSubdivTags SubdivTags;

    struct PrimvarInfo {
        TfToken name;
        TfToken interpolation;
        bool operator==(PrimvarInfo const& rhs) {
            return rhs.name == name and
                   rhs.interpolation == interpolation;
        }
    };
    typedef std::vector<PrimvarInfo> PrimvarInfoVector;

    class Key {
        friend class UsdImagingValueCache;
    private:
        SdfPath _path;
        TfToken _attribute;
    public:
        Key(SdfPath const& path, TfToken const& attr)
            : _path(path)
            , _attribute(attr)
        {}

        inline bool operator==(Key const& rhs) const {
            return _path == rhs._path and _attribute == rhs._attribute;
        }
        inline bool operator!=(Key const& rhs) const {
            return not (*this == rhs);
        }

        struct Hash {
            inline size_t operator()(Key const& key) const {
                size_t hash = key._path.GetHash();
                boost::hash_combine(hash, key._attribute.Hash());
                return hash;
            }
        };

    private:
        static Key Color(SdfPath const& path) {
            static TfToken attr("color");
            return Key(path, attr);
        }
        static Key DoubleSided(SdfPath const& path) {
            static TfToken attr("doubleSided");
            return Key(path, attr);
        }
        static Key Extent(SdfPath const& path) {
            static TfToken attr("extent");
            return Key(path, attr);
        }
        static Key InstancerTransform(SdfPath const& path) {
            static TfToken attr("instancerTransform");
            return Key(path, attr);
        }
        static Key InstanceIndices(SdfPath const& path) {
            static TfToken attr("instanceIndices");
            return Key(path, attr);
        }
        static Key Points(SdfPath const& path) {
            static TfToken attr("points");
            return Key(path, attr);
        }
        static Key Purpose(SdfPath const& path) {
            static TfToken attr("purpose");
            return Key(path, attr);
        }
        static Key Primvars(SdfPath const& path) {
            static TfToken attr("primvars");
            return Key(path, attr);
        }
        static Key SubdivTags(SdfPath const& path) {
            static TfToken attr("subdivTags");
            return Key(path, attr);
        }
        static Key Topology(SdfPath const& path) {
            static TfToken attr("topology");
            return Key(path, attr);
        }
        static Key Transform(SdfPath const& path) {
            static TfToken attr("transform");
            return Key(path, attr);
        }
        static Key Visible(SdfPath const& path) {
            static TfToken attr("visible");
            return Key(path, attr);
        }
        static Key Widths(SdfPath const& path) {
            static TfToken attr("widths");
            return Key(path, attr);
        }
        static Key Normals(SdfPath const& path) {
            static TfToken attr("normals");
            return Key(path, attr);
        }
        static Key SurfaceShader(SdfPath const& path) {
            static TfToken attr("surfaceShader");
            return Key(path, attr);
        }
    };

    UsdImagingValueCache()
        : _locked(false)
    { }

private:
    template <typename Element>
    struct _TypedCache
    {
        typedef tbb::concurrent_unordered_map<Key, Element, Key::Hash> _MapType;
        typedef typename _MapType::iterator                            _MapIt;
        typedef typename _MapType::const_iterator                      _MapConstIt;
        typedef tbb::concurrent_queue<_MapIt>                          _QueueType;

        _MapType   _map;
        _QueueType _deferredDeleteQueue;
    };


    /// Locates the requested \p key then populates \p value and returns true if
    /// found.
    template <typename T>
    bool _Find(Key const& key, T* value) const {
        typedef _TypedCache<T> Cache_t;

        Cache_t *cache = nullptr;

        _GetCache(&cache);
        typename Cache_t::_MapConstIt it = cache->_map.find(key);
        if (it == cache->_map.end()) {
            return false;
        }
        *value = it->second;
        return true;
    }

    /// Locates the requested \p key then populates \p value, swap the value
    /// from the entry and queues the entry up for deletion.
    /// Returns true if found.
    /// This function is thread-safe, but Garbage collection must be called
    /// to perform the actual deletion.
    /// Note: second hit on same key will be sucessful, but return whatever
    /// value was passed into the first _Extract.
    template <typename T>
    bool _Extract(Key const& key, T* value) {
        if (not TF_VERIFY(not _locked)) {
            return false;
        }
      
        typedef _TypedCache<T> Cache_t;
        Cache_t *cache = nullptr;

        _GetCache(&cache);
        typename Cache_t::_MapIt it = cache->_map.find(key);

        if (it == cache->_map.end()) {
            return false;
        }

        // If we're going to erase the old value, swap to avoid a copy.
        std::swap(it->second, *value);
        cache->_deferredDeleteQueue.push(it);
        return true;
    }



    /// Erases the given key from the value cache.
    /// Not thread safe
    template <typename T>
    void _Erase(Key const& key) {
        if (not TF_VERIFY(not _locked)) {
            return;
        }

        typedef _TypedCache<T> Cache_t;

        Cache_t *cache = nullptr;
        _GetCache(&cache);
        cache->_map.unsafe_erase(key);
    }

    /// Returns a reference to the held value for \p key. Note that the entry
    /// for \p key will created with a default-constructed instance of T if
    /// there was no pre-existing entry.
    template <typename T>
    T& _Get(Key const& key) const {
        typedef _TypedCache<T> Cache_t;

        Cache_t *cache = nullptr;
        _GetCache(&cache);

        // With concurrent_unordered_map, multi-threaded insertion is safe.
        std::pair<typename Cache_t::_MapIt, bool> res =
                                cache->_map.insert(std::make_pair(key, T()));

        return res.first->second;
    }

    /// Removes items from the cache that are marked for deletion.
    /// This is not thread-safe and designed to be called after
    /// all the worker threads have been joined.
    template <typename T>
    void _GarbageCollect(_TypedCache<T> &cache) {
        typedef _TypedCache<T> Cache_t;

        typename Cache_t::_MapIt it;

        while (cache._deferredDeleteQueue.try_pop(it)) {
            cache._map.unsafe_erase(it);
        }
    }

public:

    void EnableMutation() { _locked = false; }
    void DisableMutation() { _locked = true; }

    /// Clear all data associated with a specific path.
    void Clear(SdfPath const& path) {
        _Erase<VtValue>(Key::Color(path));
        _Erase<bool>(Key::DoubleSided(path));
        _Erase<GfRange3d>(Key::Extent(path));
        _Erase<VtValue>(Key::InstanceIndices(path));
        _Erase<TfToken>(Key::Purpose(path));
        _Erase<SubdivTags>(Key::SubdivTags(path));
        _Erase<VtValue>(Key::Topology(path));
        _Erase<GfMatrix4d>(Key::Transform(path));
        _Erase<bool>(Key::Visible(path));
        _Erase<VtValue>(Key::Points(path));
        _Erase<VtValue>(Key::Widths(path));
        _Erase<VtValue>(Key::Normals(path));
        _Erase<VtValue>(Key::SurfaceShader(path));

        // PERFORMANCE: We're copying the primvar vector here, but we could
        // access the map directly, if we need to for performance reasons.
        PrimvarInfoVector vars;
        if (FindPrimvars(path, &vars)) {
            TF_FOR_ALL(pvIt, vars) {
                _Erase<VtValue>(Key(path, pvIt->name));
            }
            _Erase<PrimvarInfoVector>(Key::Primvars(path));
        }
    }

    VtValue& GetColor(SdfPath const& path) const {
        return _Get<VtValue>(Key::Color(path));
    }
    bool& GetDoubleSided(SdfPath const& path) const {
        return _Get<bool>(Key::DoubleSided(path));
    }
    GfRange3d& GetExtent(SdfPath const& path) const {
        return _Get<GfRange3d>(Key::Extent(path));
    }
    GfMatrix4d& GetInstancerTransform(SdfPath const& path) const {
        return _Get<GfMatrix4d>(Key::InstancerTransform(path));
    }
    VtValue& GetInstanceIndices(SdfPath const& path) const {
        return _Get<VtValue>(Key::InstanceIndices(path));
    }
    VtValue& GetPoints(SdfPath const& path) const {
        return _Get<VtValue>(Key::Points(path));
    }
    TfToken& GetPurpose(SdfPath const& path) const {
        return _Get<TfToken>(Key::Purpose(path));
    }
    PrimvarInfoVector& GetPrimvars(SdfPath const& path) const {
        return _Get<PrimvarInfoVector>(Key::Primvars(path));
    }
    SubdivTags& GetSubdivTags(SdfPath const& path) const {
        return _Get<SubdivTags>(Key::SubdivTags(path));
    }
    VtValue& GetTopology(SdfPath const& path) const {
        return _Get<VtValue>(Key::Topology(path));
    }
    GfMatrix4d& GetTransform(SdfPath const& path) const {
        return _Get<GfMatrix4d>(Key::Transform(path));
    }
    bool& GetVisible(SdfPath const& path) const {
        return _Get<bool>(Key::Visible(path));
    }
    VtValue& GetWidths(SdfPath const& path) const {
        return _Get<VtValue>(Key::Widths(path));
    }
    VtValue& GetNormals(SdfPath const& path) const {
        return _Get<VtValue>(Key::Normals(path));
    }
    VtValue& GetPrimvar(SdfPath const& path, TfToken const& name) const {
        return _Get<VtValue>(Key(path, name));
    }
    SdfPath& GetSurfaceShader(SdfPath const& path) const {
        return _Get<SdfPath>(Key::SurfaceShader(path));
    }

    bool FindPrimvar(SdfPath const& path, TfToken const& name, VtValue* value) const {
        return _Find(Key(path, name), value);
    }
    bool FindColor(SdfPath const& path, VtValue* value) const {
        return _Find(Key::Color(path), value);
    }
    bool FindDoubleSided(SdfPath const& path, bool* value) const {
        return _Find(Key::DoubleSided(path), value);
    }
    bool FindExtent(SdfPath const& path, GfRange3d* value) const {
        return _Find(Key::Extent(path), value);
    }
    bool FindInstancerTransform(SdfPath const& path, GfMatrix4d* value) const {
        return _Find(Key::InstancerTransform(path), value);
    }
    bool FindInstanceIndices(SdfPath const& path, VtValue* value) const {
        return _Find(Key::InstanceIndices(path), value);
    }
    bool FindPoints(SdfPath const& path, VtValue* value) const {
        return _Find(Key::Points(path), value);
    }
    bool FindPurpose(SdfPath const& path, TfToken* value) const {
        return _Find(Key::Purpose(path), value);
    }
    bool FindPrimvars(SdfPath const& path, PrimvarInfoVector* value) const {
        return _Find(Key::Primvars(path), value);
    }
    bool FindSubdivTags(SdfPath const& path, SubdivTags* value) const {
        return _Find(Key::SubdivTags(path), value);
    }
    bool FindTopology(SdfPath const& path, VtValue* value) const {
        return _Find(Key::Topology(path), value);
    }
    bool FindTransform(SdfPath const& path, GfMatrix4d* value) const {
        return _Find(Key::Transform(path), value);
    }
    bool FindVisible(SdfPath const& path, bool* value) const {
        return _Find(Key::Visible(path), value);
    }
    bool FindWidths(SdfPath const& path, VtValue* value) const {
        return _Find(Key::Widths(path), value);
    }
    bool FindNormals(SdfPath const& path, VtValue* value) const {
        return _Find(Key::Normals(path), value);
    }
    bool FindSurfaceShader(SdfPath const& path, SdfPath* value) const {
        return _Find(Key::SurfaceShader(path), value);
    }

    bool ExtractColor(SdfPath const& path, VtValue* value) {
        return _Extract(Key::Color(path), value);
    }
    bool ExtractDoubleSided(SdfPath const& path, bool* value) {
        return _Extract(Key::DoubleSided(path), value);
    }
    bool ExtractExtent(SdfPath const& path, GfRange3d* value) {
        return _Extract(Key::Extent(path), value);
    }
    bool ExtractInstancerTransform(SdfPath const& path, GfMatrix4d* value) {
        return _Extract(Key::InstancerTransform(path), value);
    }
    bool ExtractInstanceIndices(SdfPath const& path, VtValue* value) {
        return _Extract(Key::InstanceIndices(path), value);
    }
    bool ExtractPoints(SdfPath const& path, VtValue* value) {
        return _Extract(Key::Points(path), value);
    }
    bool ExtractPurpose(SdfPath const& path, TfToken* value) {
        return _Extract(Key::Purpose(path), value);
    }
    bool ExtractPrimvars(SdfPath const& path, PrimvarInfoVector* value) {
        return _Extract(Key::Primvars(path), value);
    }
    bool ExtractSubdivTags(SdfPath const& path, SubdivTags* value) {
        return _Extract(Key::SubdivTags(path), value);
    }
    bool ExtractTopology(SdfPath const& path, VtValue* value) {
        return _Extract(Key::Topology(path), value);
    }
    bool ExtractTransform(SdfPath const& path, GfMatrix4d* value) {
        return _Extract(Key::Transform(path), value);
    }
    bool ExtractVisible(SdfPath const& path, bool* value) {
        return _Extract(Key::Visible(path), value);
    }
    bool ExtractWidths(SdfPath const& path, VtValue* value) {
        return _Extract(Key::Widths(path), value);
    }
    bool ExtractNormals(SdfPath const& path, VtValue* value) {
        return _Extract(Key::Normals(path), value);
    }
    bool ExtractSurfaceShader(SdfPath const& path, SdfPath* value) {
        return _Extract(Key::SurfaceShader(path), value);
    }
    bool ExtractPrimvar(SdfPath const& path, TfToken const& name, VtValue* value) {
        return _Extract(Key(path, name), value);
    }

    /// Remove any items from the cache that are marked for defered deletion.
    void GarbageCollect()
    {
        _GarbageCollect(_boolCache);
        _GarbageCollect(_tokenCache);
        _GarbageCollect(_rangeCache);
        _GarbageCollect(_matrixCache);
        _GarbageCollect(_vec4Cache);
        _GarbageCollect(_valueCache);
        _GarbageCollect(_pviCache);
        _GarbageCollect(_subdivTagsCache);
        _GarbageCollect(_sdfPathCache);
    }

private:
    bool _locked;

    // visible, doubleSided
    typedef _TypedCache<bool> _BoolCache;
    mutable _BoolCache _boolCache;

    // purpose
    typedef _TypedCache<TfToken> _TokenCache;
    mutable _TokenCache _tokenCache;

    // extent
    typedef _TypedCache<GfRange3d> _RangeCache;
    mutable _RangeCache _rangeCache;

    // transform
    typedef _TypedCache<GfMatrix4d> _MatrixCache;
    mutable _MatrixCache _matrixCache;

    // color (will be VtValue)
    typedef _TypedCache<GfVec4f> _Vec4Cache;
    mutable _Vec4Cache _vec4Cache;

    // sdfPath
    typedef _TypedCache<SdfPath> _SdfPathCache;
    mutable _SdfPathCache _sdfPathCache;

    // primvars, topology
    typedef _TypedCache<VtValue> _ValueCache;
    mutable _ValueCache _valueCache;

    typedef _TypedCache<PrimvarInfoVector> _PviCache;
    mutable _PviCache _pviCache;

    typedef _TypedCache<SubdivTags> _SubdivTagsCache;
    mutable _SubdivTagsCache _subdivTagsCache;

    void _GetCache(_BoolCache **cache) const {
        *cache = &_boolCache;
    }
    void _GetCache(_TokenCache **cache) const {
        *cache = &_tokenCache;
    }
    void _GetCache(_RangeCache **cache) const {
        *cache = &_rangeCache;
    }
    void _GetCache(_MatrixCache **cache) const {
        *cache = &_matrixCache;
    }
    void _GetCache(_Vec4Cache **cache) const {
        *cache = &_vec4Cache;
    }
    void _GetCache(_ValueCache **cache) const {
        *cache = &_valueCache;
    }
    void _GetCache(_PviCache **cache) const {
        *cache = &_pviCache;
    }
    void _GetCache(_SubdivTagsCache **cache) const {
        *cache = &_subdivTagsCache;
    }
    void _GetCache(_SdfPathCache **cache) const {
        *cache = &_sdfPathCache;
    }
};

#endif // USDIMAGING_VALUE_CACHE_H
