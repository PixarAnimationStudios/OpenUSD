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
#ifndef USD_STAGECACHE_H
#define USD_STAGECACHE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/base/tf/declarePtrs.h"

#include <boost/lexical_cast.hpp>
#include <boost/operators.hpp>

#include <string>
#include <memory>
#include <mutex>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


SDF_DECLARE_HANDLES(SdfLayer);
TF_DECLARE_REF_PTRS(UsdStage);

class ArResolverContext;

class UsdStageCacheRequest;

/// \class UsdStageCache
///
/// A strongly concurrency safe collection of UsdStageRefPtr s, enabling
/// sharing across multiple clients and threads.  See UsdStageCacheContext for
/// typcial use cases finding UsdStage s in a cache and publishing UsdStage s to
/// a cache.
///
/// UsdStageCache is strongly thread safe: all operations other than
/// construction and destruction may be performed concurrently.
///
/// Clients typically populate and fetch UsdStage s in caches by binding a
/// UsdStageCacheContext object to a cache, then using the UsdStage::Open() API.
/// See UsdStageCacheContext for more details.  Clients may also populate and
/// fetch directly via UsdStageCache::Insert(), UsdStageCache::Find(),
/// UsdStageCache::FindOneMatching(), and UsdStageCache::FindAllMatching()
/// API.
///
/// Caches provide a mechanism that associates a lightweight key,
/// UsdStageCache::Id, with a cached stage.  A UsdStageCache::Id can be
/// converted to and from long int and string.  This can be useful for
/// communicating within a third party application that cannot transmit
/// arbitrary C++ objects.  See UsdStageCache::GetId().
///
/// Clients may iterate all cache elements using UsdStageCache::GetAllStages()
/// and remove elements with UsdStageCache::Erase(),
/// UsdStageCache::EraseAll(), and UsdStageCache::Clear().
///
/// Note that this class is a regular type: it can be copied and assigned at
/// will.  It is not a singleton.  Also, since it holds a collection of
/// UsdStageRefPtr objects, copying it does not create new UsdStage instances,
/// it merely copies the RefPtrs.
///
/// Enabling the USD_STAGE_CACHE TF_DEBUG code will issue debug output for
/// UsdStageCache Find/Insert/Erase/Clear operations.  Also see
/// UsdStageCache::SetDebugName() and UsdStageCache::GetDebugName().
///
class UsdStageCache
{
public:
    /// \class Id
    ///
    /// A lightweight identifier that may be used to identify a
    /// particular cached stage within a UsdStageCache.  An identifer may be
    /// converted to and from long int and string, to facilitate use within
    /// restricted contexts.
    ///
    /// Id objects are only valid with the stage from which they were obtained.
    /// It never makes sense to use an Id with a stage other than the one it was
    /// obtained from.
    ///
    struct Id : private boost::totally_ordered<Id> {
        /// Default construct an invalid id.
        Id() : _value(-1) {}

        /// Create an Id from an integral value.  The supplied \p val must have
        /// been obtained by calling ToLongInt() previously.
        static Id FromLongInt(long int val) { return Id(val); }

        /// Create an Id from a string value.  The supplied \p val must have
        /// been obtained by calling ToString() previously.
        static Id FromString(const std::string &s) {
            return FromLongInt(boost::lexical_cast<long int>(s));
        }

        /// Convert this Id to an integral representation.
        long int ToLongInt() const { return _value; }

        /// Convert this Id to a string representation.
        std::string ToString() const {
            return boost::lexical_cast<std::string>(ToLongInt());
        }

        /// Return true if this Id is valid.
        bool IsValid() const { return _value != -1; }

        /// Return true if this Id is valid.
        explicit operator bool() const { return IsValid(); }

    private:
        /// Equality comparison.
        friend bool operator==(const Id &lhs, const Id &rhs) {
            return lhs.ToLongInt() == rhs.ToLongInt();
        }
        /// Less-than comparison.
        friend bool operator<(const Id &lhs, const Id &rhs) {
            return lhs.ToLongInt() < rhs.ToLongInt();
        }
        /// Hash.
        friend size_t hash_value(Id id) {
            return ~size_t(id.ToLongInt());
        }
        
        explicit Id(long int val) : _value(val) {}

        long int _value;
    };

    /// Default construct an empty cache.
    USD_API
    UsdStageCache();
    
    /// Construct a new cache as a copy of \p other.
    USD_API
    UsdStageCache(const UsdStageCache &other);

    /// Destructor.
    USD_API
    ~UsdStageCache();

    /// Replace the contents of this cache with a copy of \p other.
    USD_API
    UsdStageCache &operator=(const UsdStageCache &other);

    /// Swap the contents of this cache with \p other.
    USD_API
    void swap(UsdStageCache &other);

    /// Return a vector containing the stages present in this cache.
    USD_API
    std::vector<UsdStageRefPtr> GetAllStages() const;

    /// Return the number of stages present in this cache.
    USD_API
    size_t Size() const;

    /// Return true if this cache holds no stages, false otherwise.
    bool IsEmpty() const { return Size() == 0; }

    /// Find an existing stage in the cache that satisfies \p request, or invoke
    /// request.Manufacture() to create one and insert it into the cache.
    /// Return the resulting stage and a bool indicating whether or not this
    /// call manufactured the stage.
    ///
    /// This avoids race conditions in concurrent code that can occur using the
    /// other public methods.  Consider this racy example:
    ///
    /// \code
    /// if (!cache.FindOneMatching(rootLayer)) {
    ///     auto stage = UsdStage::Open(rootLayer);
    ///     cache.Insert(stage);
    /// }
    /// \endcode
    ///
    /// This will race with another thread doing the same thing, resulting in
    /// two stages with the same root layer inserted in the cache.  This is
    /// potentially rather inefficient since stage creation can be expensive,
    /// depending on how many objects and how many prims & layers the stage
    /// contains.  RequestStage() avoids this by ensuring that there is no race
    /// and the stage is created only once.
    ///
    /// Note that request should not be retained and must not be reused.
    USD_API
    std::pair<UsdStageRefPtr, bool>
    RequestStage(UsdStageCacheRequest &&request);

    /// Find the stage in this cache corresponding to \p id in this cache.  If
    /// \p id is not valid (see Id::IsValid()) or if this cache does not have a
    /// stage corresponding to \p id, return null.
    USD_API
    UsdStageRefPtr Find(Id id) const;

    /// Find a stage in this cache with \p rootLayer.  If there is no matching
    /// stage in this cache, return null.  If there is more than one matching
    /// stage in this cache, return an arbitrary matching one.  See also
    /// FindAllMatching().
    USD_API
    UsdStageRefPtr FindOneMatching(const SdfLayerHandle &rootLayer) const;

    /// Find a stage in this cache with \p rootLayer and \p sessionLayer.  If
    /// there is no matching stage in this cache, return null.  If there is more
    /// than one matching stage in this cache, return an arbitrary matching one.
    /// See also FindAllMatching().
    USD_API
    UsdStageRefPtr FindOneMatching(const SdfLayerHandle &rootLayer,
                                   const SdfLayerHandle &sessionLayer) const;

    /// Find a stage in this cache with \p rootLayer and \p pathResolverContext.
    /// If there is no matching stage in this cache, return null.  If there is
    /// more than one matching stage in this cache, return an arbitrary matching
    /// one.
    /// \sa FindAllMatching()
    USD_API
    UsdStageRefPtr FindOneMatching(
        const SdfLayerHandle &rootLayer,
        const ArResolverContext &pathResolverContext) const;

    /// Find a stage in this cache with \p rootLayer, \p sessionLayer, and
    /// \p pathResolverContext.  If there is no matching stage in this cache,
    /// return null.  If there is more than one matching stage in this cache,
    /// return an arbitrary matching one.
    /// \sa FindAllMatching()
    USD_API
    UsdStageRefPtr FindOneMatching(
        const SdfLayerHandle &rootLayer,
        const SdfLayerHandle &sessionLayer,
        const ArResolverContext &pathResolverContext) const;

    /// Find all stages in this cache with \p rootLayer.  If there is no
    /// matching stage in this cache, return an empty vector.
    USD_API
    std::vector<UsdStageRefPtr>
    FindAllMatching(const SdfLayerHandle &rootLayer) const;

    /// Find all stages in this cache with \p rootLayer and \p sessionLayer.
    /// If there is no matching stage in this cache, return an empty vector.
    USD_API
    std::vector<UsdStageRefPtr>
    FindAllMatching(const SdfLayerHandle &rootLayer,
                    const SdfLayerHandle &sessionLayer) const;

    /// Find all stages in this cache with \p rootLayer and
    /// \p pathResolverContext.  If there is no matching stage in this cache,
    /// return an empty vector.
    USD_API
    std::vector<UsdStageRefPtr>
    FindAllMatching(const SdfLayerHandle &rootLayer,
                    const ArResolverContext &pathResolverContext) const;

    /// Find all stages in this cache with \p rootLayer, \p sessionLayer, and
    /// \p pathResolverContext.  If there is no matching stage in this cache,
    /// return an empty vector.  If there is more than one matching stage in
    /// this cache, return an arbitrary matching one. 
    USD_API
    std::vector<UsdStageRefPtr>
    FindAllMatching(const SdfLayerHandle &rootLayer,
                    const SdfLayerHandle &sessionLayer,
                    const ArResolverContext &pathResolverContext) const;

    /// Return the Id associated with \p stage in this cache.  If \p stage is
    /// not present in this cache, return an invalid Id.
    USD_API
    Id GetId(const UsdStageRefPtr &stage) const;

    /// Return true if \p stage is present in this cache, false otherwise.
    bool Contains(const UsdStageRefPtr &stage) const {
        return static_cast<bool>(GetId(stage));
    }

    /// Return true if \p id is present in this cache, false otherwise.
    bool Contains(Id id) const { return Find(id); }

    /// Insert \p stage into this cache and return its associated Id.  If the
    /// given \p stage is already present in this cache, simply return its
    /// associated Id.
    USD_API
    Id Insert(const UsdStageRefPtr &stage);
    
    /// Erase the stage identified by \p id from this cache and return true.  If
    /// \p id is invalid or there is no associated stage in this cache, do
    /// nothing and return false.  Since the cache contains UsdStageRefPtr,
    /// erasing a stage from the cache will only destroy the stage if no other
    /// UsdStageRefPtrs exist referring to it.
    USD_API
    bool Erase(Id id);

    /// Erase \p stage from this cache and return true.  If \p stage is not
    /// present in this cache, do nothing and return false.  Since the cache
    /// contains UsdStageRefPtr, erasing a stage from the cache will only
    /// destroy the stage if no other UsdStageRefPtrs exist referring to it.
    USD_API
    bool Erase(const UsdStageRefPtr &stage);

    /// Erase all stages present in the cache with \p rootLayer and return the
    /// number erased.  Since the cache contains UsdStageRefPtr, erasing a stage
    /// from the cache will only destroy the stage if no other UsdStageRefPtrs
    /// exist referring to it.
    USD_API
    size_t EraseAll(const SdfLayerHandle &rootLayer);

    /// Erase all stages present in the cache with \p rootLayer and
    /// \p sessionLayer and return the number erased.  Since the cache contains
    /// UsdStageRefPtr, erasing a stage from the cache will only destroy the
    /// stage if no other UsdStageRefPtrs exist referring to it.
    USD_API
    size_t EraseAll(const SdfLayerHandle &rootLayer,
                    const SdfLayerHandle &sessionLayer);

    /// Erase all stages present in the cache with \p rootLayer,
    /// \p sessionLayer, and \p pathResolverContext and return the number
    /// erased.  Since the cache contains UsdStageRefPtr, erasing a stage from
    /// the cache will only destroy the stage if no other UsdStageRefPtrs
    /// exist referring to it.
    USD_API
    size_t EraseAll(const SdfLayerHandle &rootLayer,
                    const SdfLayerHandle &sessionLayer,
                    const ArResolverContext &pathResolverContext);
    
    /// Remove all entries from this cache, leaving it empty and equivalent to a
    /// default-constructed cache.  Since the cache contains UsdStageRefPtr,
    /// erasing a stage from the cache will only destroy the stage if no other
    /// UsdStageRefPtrs exist referring to it.
    USD_API
    void Clear();

    /// Assign a debug name to this cache.  This will be emitted in debug output
    /// messages when the USD_STAGE_CACHES debug flag is enabled.  If set to the
    /// empty string, the cache's address will be used instead.
    USD_API
    void SetDebugName(const std::string &debugName);

    /// Retrieve this cache's debug name, set with SetDebugName().  If no debug
    /// name has been assigned, return the empty string.
    USD_API
    std::string GetDebugName() const;

private:
    friend void swap(UsdStageCache &lhs, UsdStageCache &rhs) {
        lhs.swap(rhs);
    }

    typedef struct Usd_StageCacheImpl _Impl;
    std::unique_ptr<_Impl> _impl;
    mutable std::mutex _mutex;
};

class UsdStageCacheRequest
{
public:
    USD_API
    virtual ~UsdStageCacheRequest();

    // Return true if the stage satisfies this request.
    virtual bool IsSatisfiedBy(UsdStageRefPtr const &stage) const = 0;

    // Return true if the pending request will satisfy this request, once
    // complete.
    virtual bool IsSatisfiedBy(UsdStageCacheRequest const &pending) const = 0;

    // Invoked to manufacture a stage to insert in the cache.  Postcondition:
    // IsSatisfiedBy() must return true for the resulting stage.
    virtual UsdStageRefPtr Manufacture() = 0;

private:
    friend class UsdStageCache;

    struct _Mailbox;
    void _Subscribe(_Mailbox *);

    struct _Data;
    struct _DataDeleter { void operator()(_Data *); };
    std::unique_ptr<_Data, _DataDeleter> _data;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_STAGECACHE_H
