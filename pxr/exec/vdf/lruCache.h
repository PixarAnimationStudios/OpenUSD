//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_LRU_CACHE_H
#define PXR_EXEC_VDF_LRU_CACHE_H

/// \file

#include "pxr/pxr.h"

#include <cstddef>
#include <list>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfLRUCache
/// 
/// A simple cache with a fixed capacity and a least-recently-used eviction
/// policy.
/// 
template < typename Key, typename Value, typename Hash >
class VdfLRUCache
{
public:
    // Non-copyable
    VdfLRUCache(const VdfLRUCache &) = delete;
    VdfLRUCache &operator=(const VdfLRUCache &) = delete;

    // Non-movable
    VdfLRUCache(VdfLRUCache &&) = delete;
    VdfLRUCache &operator=(VdfLRUCache &&) = delete;

    /// Constructs a new cache with a fixed \p capacity.
    ///
    explicit VdfLRUCache(size_t capacity) : _capacity(capacity) {}

    /// Performs a lookup into the cache and returns \c true if the cache
    /// contains an entry for the given \p key. If the cache does not contain
    /// and entry for \p key, a new entry will be constructed as long as the
    /// cache is below capacity. If the cache has reached capacity an existing
    /// entry will be repurposed for \p key. In this case, \p value will point
    /// at the evicted entry. and the client will be resonpsible for resetting
    /// \p value. In all cases, \p value will always point at a valid instance
    /// of \p Value.
    ///
    bool Lookup(const Key &key, Value **value);

    /// Removes all entries from the cache.
    ///
    void Clear();

private:
    // The cache entry stores a hash to accelerate equality comparison, as well
    // as the key and value for each entry.
    struct _Entry {
        _Entry(size_t h, const Key &k) : hash(h), key(k) {}

        size_t hash;
        Key key;
        Value value;
    };

    // The sorted list of cache entries. The most recently used entry will
    // always be at the head of the list.
    using _List = std::list<_Entry>;
    _List _list;

    // The fixed cache capacity. The cache will never grow beyond this size.
    const size_t _capacity;
};

////////////////////////////////////////////////////////////////////////////////

template < typename Key, typename Value, typename Hash >
bool
VdfLRUCache<Key, Value, Hash>::Lookup(const Key &key, Value **value)
{
    // Hash the key. We will use this hash as an early out for
    // equality comparison.
    const size_t hash = Hash()(key);

    // Iterate over all the recently used entries.
    typename _List::iterator it = _list.begin();
    for (; it != _list.end(); ++it) {
        // If the hash and key compare equal, we have found a matching entry.
        if (it->hash == hash && it->key == key) {
            // If this entry isn't already at the head of the list, move it
            // there. This way, the list always stays sorted in order of most
            // recent usage.
            if (it != _list.begin()) {
                _list.splice(_list.begin(), _list, it);
            }

            // Return a pointer to the value at the current entry.
            *value = &it->value;
            return true;
        }
    }

    // If we were unable to find a matching entry and the list is below
    // capacity, let's insert a new entry.
    if (_list.size() < _capacity) {
        _list.emplace_front(hash, key);
    }

    // If the list has reached capacity, reuse the last entry by first moving
    // it to the front.
    else {
        _list.splice(_list.begin(), _list, --_list.end());
        _list.front().hash = hash;
        _list.front().key = key;
    }

    // Return a pointer to the new value entry.
    *value = &_list.front().value;
    return false;
}

template < typename Key, typename Value, typename Hash >
void
VdfLRUCache<Key, Value, Hash>::Clear()
{
    _list.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
