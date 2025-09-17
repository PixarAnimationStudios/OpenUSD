//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compiledLeafNodeCache.h"

#include "pxr/exec/ef/leafNode.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_CompiledLeafNodeCache::Exec_CompiledLeafNodeCache() = default;

const EfLeafNode *
Exec_CompiledLeafNodeCache::Find(const ExecValueKey &valueKey) const
{
    const _CacheKey cacheKey(valueKey);
    const _Cache::const_iterator cacheIter = _cache.find(cacheKey);
    return cacheIter != _cache.end() ? cacheIter->second : nullptr;
}

void
Exec_CompiledLeafNodeCache::Insert(
    const ExecValueKey &valueKey,
    const EfLeafNode *const leafNode)
{
    _CacheKey cacheKey(valueKey);
    auto [emplacedCacheIter, emplacedCache] =
        _cache.emplace(cacheKey, leafNode);
    
    // The CompiledLeafNodeCache only tracks the first leaf node created for a
    // value key. If other leaf nodes are created for the same value key, they
    // do not replace the existing leaf node in the cache or in the reverse
    // table. This is ok, because all of these leaf nodes will connect to the
    // same masked output, so there is no need to distinguish between them. Also
    // note, the only way to create duplicate leaf nodes is for a request to
    // contain duplicate value keys.
    if (!emplacedCache) {
        return;
    }

    auto [emplacedReverseTableIter, emplacedReverseTable] =
        _reverseTable.emplace(leafNode, std::move(cacheKey));
    
    TF_VERIFY(emplacedReverseTable);
}

void
Exec_CompiledLeafNodeCache::WillDeleteNode(const EfLeafNode *const node)
{
    const _ReverseTable::iterator reverseTableIter = _reverseTable.find(node);
    if (reverseTableIter == _reverseTable.end()) {
        return;
    }

    _cache.unsafe_erase(reverseTableIter->second);
    _reverseTable.unsafe_erase(reverseTableIter);
}

PXR_NAMESPACE_CLOSE_SCOPE
