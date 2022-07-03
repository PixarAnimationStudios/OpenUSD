//
// Copyright 2018 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_COLLECTION_CACHE_H
#define PXR_USD_IMAGING_USD_IMAGING_COLLECTION_CACHE_H

/// \file usdImaging/collectionCache.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usd/usd/collectionAPI.h"

#include <boost/noncopyable.hpp>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImaging_CollectionCache
///
/// UsdImaging_CollectionCache provides a cache of Usd collections
/// discovered in the scene.  It associates the collection paths
/// (UsdCollectionAPI::GetCollectionPath) with the computed
/// membership query (UsdCollectionAPI::ComputeMembershipQuery).
///
/// For efficiency, it groups collections into equivalence
/// classes based on the computed query.  Collections that yield
/// equivalent queries are merged.  Each unique query is assigned
/// an identifier token.
///
/// This cache is used to track the collections used for linking
/// UsdLux lights/shadows/filters to geometry.
///
/// As an optimization, the query that includes everything is
/// treated as a special case and given the empty id, TfToken().
///
class UsdImaging_CollectionCache : boost::noncopyable {
public:
    /// Query is the MembershipQuery computed from a collection's state.
    typedef UsdCollectionAPI::MembershipQuery Query;

    /// Computes the membership query from the current state of the
    /// given collection, and establishes a cache entry.  If a
    /// prior entry existed for the collection at this path,
    /// it is removed first.
    /// Returns true for newly created collection or
    /// if the hash of the collection is different from the previous collection
    bool
    UpdateCollection(UsdCollectionAPI const& collection);
 
    /// Returns the hash of the removed collection, or 0 if no collection existed
    size_t
    RemoveCollection(UsdStageWeakPtr const& stage, SdfPath const& path);

    /// Return the cached entry for the given collection.
    TfToken
    GetIdForCollection(UsdCollectionAPI const& collection);

    /// Return a list of identifiers of all collections that contain
    // the given path.
    VtArray<TfToken>
    ComputeCollectionsContainingPath(SdfPath const& path) const;

    /// Return the cached MembershipQuery for a given id
    void
    GetMembershipQuery(TfToken const& id, const Query** query) const;

    /// Returns a set of dirty paths
    /// Should only be used if AreAllPathsDirty returned false
    SdfPathSet const&
    GetDirtyPaths() const;

    /// Clears the internal dirty flags
    void
    ClearDirtyPaths();

private:
    // The cache boils down to tracking the correspondence of
    // collection paths, their computed queries, and the id
    // assigned to each unique query:
    //
    // CollectionPath <=> MembershipQuery <=> AssignedId
    //
    // In this scheme, the assigned id provides a compact but
    // potentially human-meaningful reference to the query,
    // which we can pass to the renderer. 
    //
    std::unordered_map<Query, TfToken, Query::Hash> _idForQuery;
    std::unordered_map<TfToken, Query, TfToken::HashFunctor> _queryForId;
    std::unordered_map<SdfPath, TfToken, SdfPath::Hash> _idForPath;
    std::unordered_map<Query, SdfPathSet, Query::Hash> _pathsForQuery;

    void
    _MarkCollectionContentDirty(UsdStageWeakPtr const& stage, UsdCollectionAPI::MembershipQuery const& query);

    SdfPathSet _dirtyPaths;

    std::mutex _mutex;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_COLLECTION_CACHE_H
