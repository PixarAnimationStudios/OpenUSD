//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_MASK_MEMOIZER_H
#define PXR_EXEC_VDF_MASK_MEMOIZER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"

#include "pxr/base/tf/hash.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Memoizes the results of mask append (union) operations.
///
/// MapType must provide find and insert methods compatible with the standard
/// library associative container API.  MapType must accept Key, Mapped, Hash
/// and Eq types as its first four template parameters with the same meaning
/// as the standard unordered associative containers.  References returned by
/// Append have that same invalidation policy as the MapType.  For example,
/// references from VdfMaskMemoizer<TfHashMap> are valid for the lifetime of
/// the memoizer but calling VdfMaskMemoizer<flat_map>::Append invalidates
/// *all* references.
template <template <typename...> class MapType>
class VdfMaskMemoizer
{
public:
    /// Append \p lhs and \p rhs and return the result.
    /// Returns a cached result, if available.
    const VdfMask &Append(const VdfMask &lhs, const VdfMask &rhs) {
        _Key key{lhs, rhs};

        typename _Cache::iterator it = _appended.find(key);
        if (it != _appended.end()) {
            return it->second;
        }

        return _appended.insert({std::move(key), lhs | rhs}).first->second;
    }

private:

    // Operations are keyed off of the lhs and rhs operands.
    using _Key = std::pair<VdfMask, VdfMask>;

    // Produce a combination of lhs and rhs hash values as the hash for the
    // key.
    struct _Hash {
        size_t operator()(const _Key &v) const {
            return TfHash::Combine(v.first.GetHash(), v.second.GetHash());
        }
    };

    // The cache for append operations.
    using _Cache = MapType<_Key, VdfMask, _Hash>;
    _Cache _appended;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
