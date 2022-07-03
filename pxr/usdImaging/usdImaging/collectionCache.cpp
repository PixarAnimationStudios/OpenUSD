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
#include "pxr/usdImaging/usdImaging/collectionCache.h"
#include "pxr/usdImaging/usdImaging/debugCodes.h"

PXR_NAMESPACE_OPEN_SCOPE

// A query is trivial if it includes everything.
static bool
_IsQueryTrivial(UsdCollectionAPI::MembershipQuery const& query)
{
    // XXX Should be a faster way to do this!
    UsdCollectionAPI::MembershipQuery::PathExpansionRuleMap ruleMap =
        query.GetAsPathExpansionRuleMap();
    return ruleMap.size() == 1 &&
        ruleMap.begin()->first == SdfPath::AbsoluteRootPath() &&
        ruleMap.begin()->second == UsdTokens->expandPrims;
}

void
UsdImaging_CollectionCache::_MarkCollectionContentDirty(UsdStageWeakPtr const& stage, 
    UsdCollectionAPI::MembershipQuery const& query)
{
    SdfPathSet linkedPaths = UsdComputeIncludedPathsFromCollection(query, stage);
    std::merge(_dirtyPaths.begin(), _dirtyPaths.end(), linkedPaths.begin(), linkedPaths.end(),
        std::inserter(_dirtyPaths, _dirtyPaths.begin()));
}

bool
UsdImaging_CollectionCache::UpdateCollection(UsdCollectionAPI const& c)
{
    const UsdStageWeakPtr& stage = c.GetPrim().GetStage();
    const size_t removedHash = RemoveCollection(stage, c.GetCollectionPath());

    std::lock_guard<std::mutex> lock(_mutex);

    SdfPath path = c.GetCollectionPath();
    UsdCollectionAPI::MembershipQuery query = c.ComputeMembershipQuery();
    bool changed = removedHash != query.GetHash();

    if (_IsQueryTrivial(query)) {
        TF_DEBUG(USDIMAGING_COLLECTIONS)
            .Msg("UsdImaging_CollectionCache: trivial for <%s>\n",
                 path.GetText());
        return changed;
    }

    // Establish Id <=> Query mapping.
    TfToken id;
    auto const& idForQueryEntry = _idForQuery.find(query);
    if (idForQueryEntry == _idForQuery.end()) {
        // Assign new id.  Use token form of collection path.
        id = path.GetToken();
        _idForQuery[query] = id;
        _queryForId[id] = query;
        TF_DEBUG(USDIMAGING_COLLECTIONS)
            .Msg("UsdImaging_CollectionCache: Assigned new id '%s'\n",
                 id.GetText());
    } else {
        // Share an existing query id.
        id = idForQueryEntry->second;
        TF_DEBUG(USDIMAGING_COLLECTIONS)
            .Msg("UsdImaging_CollectionCache: Shared id '%s' for <%s>\n",
                 id.GetText(), path.GetText());
    }

    // Establish Path <=> Id mapping.
    _pathsForQuery[query].insert(path);
    _idForPath[path] = id;

    _MarkCollectionContentDirty(stage, query);
    // Also add the light in the dirty set for it to be marked as collection dirty
    _dirtyPaths.insert(c.GetPrim().GetPath());

    return changed;
}

size_t
UsdImaging_CollectionCache::RemoveCollection(UsdStageWeakPtr const& stage, SdfPath const& collectionPath)
{
    std::lock_guard<std::mutex> lock(_mutex);

    auto const& pathEntry = _idForPath.find(collectionPath);
    if (pathEntry == _idForPath.end()) {
        // No pathEntry -- bail.  This can happen if the collection was
        // trivial; see _IsQueryTrivial().
        return 0;
    }
    TfToken id = pathEntry->second;
    TF_VERIFY(!id.IsEmpty());
    _idForPath.erase(pathEntry);

    auto const& queryEntry = _queryForId.find(id);
    if (!TF_VERIFY(queryEntry != _queryForId.end())) {
        return 0;
    }
    UsdCollectionAPI::MembershipQuery const& queryRef = queryEntry->second;
    size_t hash = queryRef.GetHash();
    _idForQuery.erase(queryRef);

    _MarkCollectionContentDirty(stage, queryRef);
    _dirtyPaths.insert(collectionPath.GetPrimPath());

    auto const& pathsForQueryEntry = _pathsForQuery.find(queryRef);
    pathsForQueryEntry->second.erase(collectionPath);
    TF_DEBUG(USDIMAGING_COLLECTIONS)
        .Msg("UsdImaging_CollectionCache: Id '%s' disused <%s>\n",
             id.GetText(), collectionPath.GetText());

    // Reap _pathsForQuery entries when the last path is removed.
    // This also reaps the associated identifier.
    if (pathsForQueryEntry->second.empty()) {
        _pathsForQuery.erase(pathsForQueryEntry);
        _queryForId.erase(queryEntry);
        TF_DEBUG(USDIMAGING_COLLECTIONS)
            .Msg("UsdImaging_CollectionCache: Dropped id '%s'\n", id.GetText());
    }
    return hash;
};

SdfPathSet const&
UsdImaging_CollectionCache::GetDirtyPaths() const
{
    return _dirtyPaths;
}

void
UsdImaging_CollectionCache::ClearDirtyPaths()
{
    _dirtyPaths.clear();
}

TfToken
UsdImaging_CollectionCache::GetIdForCollection(UsdCollectionAPI const& c)
{
    SdfPath path = c.GetCollectionPath();
    auto const& pathEntry = _idForPath.find(path);
    if (pathEntry == _idForPath.end()) {
        // No entry, so assume this was cached as the trivial default.
        return TfToken();
    }
    return pathEntry->second;
}

VtArray<TfToken>
UsdImaging_CollectionCache::ComputeCollectionsContainingPath(
    SdfPath const& path) const
{
    TRACE_FUNCTION();
    VtArray<TfToken> result;
    for (auto const& entry: _queryForId) {
        if (entry.second.IsPathIncluded(path)) {
            result.push_back(entry.first);
        }
    }
    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
