//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_CONNECTOR_MAP_H
#define PXR_EXEC_VDF_CONNECTOR_MAP_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"

#include <iterator>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Maps names to inputs or outputs (a.k.a. connectors.)
///
/// VdfConnectorMap is intended to map TfToken names to VdfInput or VdfOutput
/// pointers on behalf of VdfNode.  The map is responsible for constructing
/// Connector objects and retains ownership of the pointed-to objects.
///
/// Only a limited subset of the associative container interface is provided
/// and lookup is implemented with a linear search.  This can be efficient
/// for storing small numbers of elements, especially when key comparison is
/// quick.
///
template <typename Connector>
class VdfConnectorMap
{
private:
    using _Pair = std::pair<TfToken, Connector*>;
    using _Vector = TfSmallVector<_Pair, 1>;

public:
    using key_type = TfToken;
    using mapped_type = Connector*;
    using value_type = _Pair;
    using pointer = _Pair*;
    using reference = _Pair&;
    using const_reference = const _Pair&;
    using size_type = typename _Vector::size_type;
    using difference_type = typename _Vector::difference_type;
    using iterator = typename _Vector::const_iterator;
    using reverse_iterator = typename _Vector::const_reverse_iterator;
    using const_iterator = typename _Vector::const_iterator;
    using const_reverse_iterator = typename _Vector::const_reverse_iterator;


    /// Creates an empty vector.
    VdfConnectorMap() = default;

    /// Creates a map that can hold at least \p n elements without
    /// reallocation.
    ///
    explicit VdfConnectorMap(size_t n) {
        _vec.reserve(n);
    }

    /// Destroys all connector instances owned by this map.
    ~VdfConnectorMap() {
        clear();
    }

    /// Returns a const_iterator pointing to the beginning of the map.
    const_iterator begin() const {
        return _vec.begin();
    }

    /// Returns a const_iterator pointing to the end of the map.
    const_iterator end() const {
        return _vec.end();
    }

    /// Returns a const_reverse_iterator pointing to the beginning of the map.
    const_reverse_iterator rbegin() const {
        return _vec.rbegin();
    }

    /// Returns a const_reverse_iterator pointing to the end of the map.
    const_reverse_iterator rend() const {
        return _vec.rend();
    }

    /// Returns the size of the map.
    size_type size() const {
        return _vec.size();
    }

    /// Returns \c true if the \c map's size is 0.
    bool empty() const {
        return _vec.empty();
    }

    /// Swaps the contents of this map with \p x.
    void swap(VdfConnectorMap& x) {
        _vec.swap(x._vec);
    }

    /// Swaps the contents of \p lhs and \p rhs.
    friend void swap(VdfConnectorMap& lhs, VdfConnectorMap& rhs) {
        lhs.swap(rhs);
    }

    /// Erases all of the elements.
    ///
    /// This destroys all connector instances owned by this map.
    ///
    void clear();

    /// Finds the element with key \p k.
    const_iterator find(const key_type& k) const;

    /// Test two maps for equality.
    bool operator==(const VdfConnectorMap& x) const {
        return _vec == x._vec;
    }

    bool operator!=(const VdfConnectorMap& x) const {
        return _vec != x._vec;
    }

    /// Inserts an element with key \p k if one does not already exist.
    ///
    /// If a new element is inserted, a new Connector is constructed by
    /// forwarding \p args.  Otherwise, \p args are not moved.
    ///
    /// Returns a pair of (const_iterator, bool) where the iterator points to
    /// the element in the map for key \k and bool is true if a new element
    /// was inserted.
    ///
    template <typename... Args>
    std::pair<const_iterator, bool>
    try_emplace(const key_type& k, Args&&... args);

private:

    _Vector _vec;
};


template <typename Connector>
typename VdfConnectorMap<Connector>::const_iterator
VdfConnectorMap<Connector>::find(const key_type& k) const
{
    const const_iterator endIter = end();
    for (const_iterator i = begin(); i != endIter; ++i) {
        if (i->first == k) {
            return i;
        }
    }

    return endIter;
}

template <typename Connector>
template <typename... Args>
std::pair<typename VdfConnectorMap<Connector>::const_iterator, bool> 
VdfConnectorMap<Connector>::try_emplace(const key_type& k, Args&&... args)
{
    if (const const_iterator i = find(k); i != end()) {
        return {i, false};
    }

    _vec.emplace_back(k, new Connector(std::forward<Args>(args)...));
    return {std::prev(end()), true};
}

template <typename Connector>
void
VdfConnectorMap<Connector>::clear()
{
    for (const auto &[_, connector] : _vec) {
        delete connector;
    }
    _vec.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
