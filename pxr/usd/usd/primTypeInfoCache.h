//
// Copyright 2020 Pixar
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
#ifndef PXR_USD_USD_PRIM_TYPE_INFO_CACHE_H
#define PXR_USD_USD_PRIM_TYPE_INFO_CACHE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/primTypeInfo.h"
#include "pxr/base/tf/token.h"

#include <tbb/concurrent_hash_map.h>

PXR_NAMESPACE_OPEN_SCOPE

// Private class to be used as a singleton by UsdStage to cache the type info
// structures for all distinct prim types used by any prim data.
class Usd_PrimTypeInfoCache {
public:
    using TypeId = UsdPrimTypeInfo::_TypeId;

    Usd_PrimTypeInfoCache() 
        : _emptyPrimTypeInfo(&UsdPrimTypeInfo::GetEmptyPrimType()) {}

    // Non-copyable
    Usd_PrimTypeInfoCache(const Usd_PrimTypeInfoCache&) = delete;
    Usd_PrimTypeInfoCache &operator=(const Usd_PrimTypeInfoCache&) = delete;

    // Finds the cached prim type info for the given full prim type ID, 
    // creating and caching a new one if it doesn't exist.
    const UsdPrimTypeInfo *FindOrCreatePrimTypeInfo(TypeId &&primTypeId)
    {
        if (primTypeId.IsEmpty()) {
            return GetEmptyPrimTypeInfo();
        }

        if (auto primTypeInfo = _primTypeInfoMap.Find(primTypeId)) {
            return primTypeInfo;
        }

        // If it's not, create the new type info first and then try to insert 
        // it. We always return the value found in the cache which may not be
        // the type info we created if another thread happened to create the
        // same type info and managed to insert it first. In that case ours just
        // gets deleted since the hash map didn't take ownership.
        std::unique_ptr<UsdPrimTypeInfo> newPrimTypeInfo(
            new UsdPrimTypeInfo(std::move(primTypeId)));
        return _primTypeInfoMap.Insert(std::move(newPrimTypeInfo));
    }

    // Return the single empty prim type info
    const UsdPrimTypeInfo *GetEmptyPrimTypeInfo() const 
    {
        return _emptyPrimTypeInfo;
    }

    // Computes a mapping of invalid prim type name to its valid fallback
    // type name from the provided fallback prim types dictionary.
    void ComputeInvalidPrimTypeToFallbackMap(
        const VtDictionary &fallbackPrimTypesDict,
        TfHashMap<TfToken, TfToken, TfHash> *typeToFallbackTypeMap);

private:
    // Wrapper around the thread safe hash map implementation used by the 
    // Usd_PrimTypeInfoCache to cache prim type info
    class _ThreadSafeHashMapImpl {
    public:
        _ThreadSafeHashMapImpl() = default;
        _ThreadSafeHashMapImpl(const _ThreadSafeHashMapImpl&) = delete;

        using KeyType = TypeId;
        using ValueTypePtr = std::unique_ptr<UsdPrimTypeInfo>;

        // Find and return a pointer to the prim type info if it already exists.
        const UsdPrimTypeInfo *Find(const KeyType &key) const 
        {
            _HashMap::const_accessor accessor;
            if (_hashMap.find(accessor, key)) {
                return accessor->second.get();
            }
            return nullptr;
        }

        // Inserts and takes ownership of the prim type info only if it isn't 
        // already in the hash map. Returns the pointer to the value in the map
        // after insertion regardless.
        const UsdPrimTypeInfo *Insert(ValueTypePtr &&valuePtr)
        {
            const KeyType &key = valuePtr->_GetTypeId();
            _HashMap::accessor accessor;
            if (_hashMap.insert(accessor, key)) {
                accessor->second = std::move(valuePtr);
            }
            return accessor->second.get();
        }

    private:
        struct _TbbHashFunc {
            inline bool equal(const KeyType &l, const KeyType &r) const {
                return l == r;
            }
            inline size_t hash(const KeyType &t) const {
                return t.Hash();
            }
        };

        using _HashMap = 
            tbb::concurrent_hash_map<KeyType, ValueTypePtr, _TbbHashFunc>;
        _HashMap _hashMap;
    };

    _ThreadSafeHashMapImpl _primTypeInfoMap;
    const UsdPrimTypeInfo *_emptyPrimTypeInfo;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_PRIM_TYPE_INFO_CACHE_H
