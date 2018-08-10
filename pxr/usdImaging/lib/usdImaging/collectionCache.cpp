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

TfToken
UsdImaging_CollectionCache::UpdateCollection(UsdCollectionAPI const& c)
{
    RemoveCollection(c);

    std::lock_guard<std::mutex> lock(_mutex);

    SdfPath path = c.GetCollectionPath();
    UsdCollectionAPI::MembershipQuery query = c.ComputeMembershipQuery();

    if (_IsQueryTrivial(query)) {
        return TfToken();
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

    return id;
}

void
UsdImaging_CollectionCache::RemoveCollection(UsdCollectionAPI const& c)
{
    std::lock_guard<std::mutex> lock(_mutex);

    SdfPath path = c.GetCollectionPath();
    auto const& pathEntry = _idForPath.find(path);
    if (pathEntry == _idForPath.end()) {
        // No pathEntry -- bail.  This can happen if the collection was
        // trivial; see _IsQueryTrivial().
        return;
    }
    TfToken id = pathEntry->second;
    TF_VERIFY(!id.IsEmpty());
    _idForPath.erase(pathEntry);

    auto const& queryEntry = _queryForId.find(id);
    if (!TF_VERIFY(queryEntry != _queryForId.end())) {
        return;
    }
    UsdCollectionAPI::MembershipQuery const& queryRef = queryEntry->second;

    auto const& pathsForQueryEntry = _pathsForQuery.find(queryRef);
    pathsForQueryEntry->second.erase(path);
    TF_DEBUG(USDIMAGING_COLLECTIONS)
        .Msg("UsdImaging_CollectionCache: Id '%s' disused <%s>\n",
             id.GetText(), path.GetText());

    // Reap _pathsForQuery entries when the last path is removed.
    // This also reaps the associated identifier.
    if (pathsForQueryEntry->second.empty()) {
        _pathsForQuery.erase(pathsForQueryEntry);
        _idForQuery.erase(queryRef);
        _queryForId.erase(queryEntry);
        TF_DEBUG(USDIMAGING_COLLECTIONS)
            .Msg("UsdImaging_CollectionCache: Dropped id '%s'", id.GetText());
    }
};

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
