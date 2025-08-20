//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_NODE_SET_H
#define PXR_EXEC_VDF_NODE_SET_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/node.h"

#include "pxr/base/tf/bits.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Class that efficiently stores a set of VdfNodes.
///
/// Uses node indices to identify nodes, for efficient storage.
///
class VdfNodeSet {
public:

    /// Default constructor
    ///
    VdfNodeSet() = default;

    /// Is this set empty?
    ///
    bool IsEmpty() const {
        return _bits.GetSize() == 0 || _bits.GetNumSet() == 0;
    }

    /// Get the number of elements contained in this set.
    ///
    size_t GetSize() const {
        return _bits.GetNumSet();
    }

    /// Clears the node set.
    ///
    /// Note that unlike on STL containers, this method also reclaims memory.
    ///
    VDF_API
    void Clear();

    /// Returns \c true if \p node is in the set.
    ///
    bool Contains(const VdfNode &node) const {
        return Contains(VdfNode::GetIndexFromId(node.GetId()));
    }

    /// Returns \c true if the node with the given \p index is in the set.
    ///
    bool Contains(const VdfIndex index) const {
        return index < _bits.GetSize() ? _bits.IsSet(index) : false;
    }

    /// Inserts \p node into the set.
    ///
    inline void Insert(const VdfNode &node);

    /// Inserts another \p nodeSet into this set.
    ///
    VDF_API
    void Insert(const VdfNodeSet &rhs);

    /// Removes \p node from the set.
    ///
    /// Returns true if \p node was contained in the set.
    ///
    VDF_API
    bool Remove(const VdfNode &node);

    /// Iterator types.
    ///
    typedef TfBits::View<TfBits::AllSet>::const_iterator iterator;
    typedef TfBits::View<TfBits::AllSet>::const_iterator const_iterator;

    /// Returns an iterator at the beginning of the iterable range.
    ///
    const_iterator begin() const {
        return _bits.GetAllSetView().begin();
    }

    /// Returns an iterator at the end of the iterable range.
    ///
    const_iterator end() const {
        return _bits.GetAllSetView().end();
    }

    /// Swaps two VdfNodeSet instances.
    ///
    friend void swap(VdfNodeSet &lhs, VdfNodeSet &rhs) {
        lhs._bits.Swap(rhs._bits);
    }

private:

    // Grow the underlying storage to accomodate at least the number of
    // specified indices.
    VDF_API
    void _Grow(size_t size);

    // The bit set representing nodes included in the set. The size of this
    // bit set denotes the capacity.
    TfBits _bits;

};

void
VdfNodeSet::Insert(const VdfNode &node)
{
    // Make sure to grow the bitset to accomodate the corresponding index of
    // the specified node.
    const VdfIndex index = VdfNode::GetIndexFromId(node.GetId());
    if (index >= _bits.GetSize()) {
        _Grow(index + 1);
    }

    // Set the index.
    _bits.Set(index);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
