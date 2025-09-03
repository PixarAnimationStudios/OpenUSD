//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILED_LEAF_NODE_CACHE_H
#define PXR_EXEC_EXEC_COMPILED_LEAF_NODE_CACHE_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/valueKey.h"

#include "pxr/base/tf/hash.h"
#include "pxr/usd/sdf/path.h"

#include <tbb/concurrent_unordered_map.h>

PXR_NAMESPACE_OPEN_SCOPE

class EfLeafNode;
class VdfNode;

/// Maps a compiled leaf node for each ExecValueKey.
class Exec_CompiledLeafNodeCache
{
public:
    Exec_CompiledLeafNodeCache();

    Exec_CompiledLeafNodeCache(const Exec_CompiledLeafNodeCache &) = delete;
    Exec_CompiledLeafNodeCache& operator=(
        const Exec_CompiledLeafNodeCache &) = delete;

    /// Returns a pointer to the leaf node compiled for \p valueKey.
    ///
    /// \note
    /// This method may be called concurrently with itself, and with Insert.
    ///
    const EfLeafNode *Find(const ExecValueKey &valueKey) const;

    /// Inserts a mapping from \p valueKey to a compiled \p leafNode.
    ///
    /// If a leaf node for \p valueKey already exists in the cache, the
    /// insertion will be ignored. This is not an error.
    ///
    /// \note
    /// This method may be called concurrently with itself, and with Find.
    ///
    void Insert(const ExecValueKey &valueKey, const EfLeafNode *leafNode);

    /// Notifies the cache that \p leafNode is being deleted.
    ///
    /// Entries mapping to \p leafNode will be removed from the cache.
    ///
    /// \note
    /// This method is not thread safe.
    ///
    void WillDeleteNode(const EfLeafNode *leafNode);

private:
    // ExecValueKey cannot be used as a key in tbb::concurrent_unordered_map
    // because it is not equality comparable, and because it contains an
    // EsfObject. Instead, this cache uses a different key that is similar to
    // ExecValueKey, except the provider object is stored as an SdfPath.
    struct _CacheKey
    {
        explicit _CacheKey(const ExecValueKey &valueKey)
            : providerPath(valueKey.GetProvider()->GetPath(nullptr))
            , computationName(valueKey.GetComputationName())
        {}

        bool operator==(const _CacheKey &rhs) const {
            return providerPath == rhs.providerPath &&
                computationName == rhs.computationName;
        }

        bool operator!=(const _CacheKey &rhs) const {
            return !(*this == rhs);
        }

        template <class HashState>
        friend void TfHashAppend(HashState &hashState, const _CacheKey &key) {
            hashState.Append(key.providerPath);
            hashState.Append(key.computationName);
        }

        SdfPath providerPath;
        TfToken computationName;
    };

private:
    // Stores leaf nodes for each ExecValueKey (represented by _CacheKey).
    //
    // TODO: The paths in each _CacheKey need to be updated in response to
    // namespace edits.
    using _Cache =
        tbb::concurrent_unordered_map<_CacheKey, const EfLeafNode *, TfHash>;
    _Cache _cache;

    // Maps leaf nodes back to their _CacheKeys. This is needed to clean up
    // entries when leaf nodes are deleted.
    using _ReverseTable =
        tbb::concurrent_unordered_map<const EfLeafNode *, _CacheKey, TfHash>;
    _ReverseTable _reverseTable;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif