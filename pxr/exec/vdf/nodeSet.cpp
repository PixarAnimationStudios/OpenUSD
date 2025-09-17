//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/nodeSet.h"
#include "pxr/exec/vdf/node.h"

PXR_NAMESPACE_OPEN_SCOPE

void
VdfNodeSet::Clear()
{
    if (_bits.GetSize() != 0) {
        _bits.Resize(0);
    }
}

void
VdfNodeSet::Insert(const VdfNodeSet &rhs)
{
    // Make sure the bit set can accomodate the rhs.
    _bits.ResizeKeepContent(std::max(_bits.GetSize(), rhs._bits.GetSize()));

    // Or in the rhs. Note that this supports or-ing a smaller source set.
    _bits.OrSubset(rhs._bits);
}

bool
VdfNodeSet::Remove(const VdfNode &node)
{
    const VdfIndex index = VdfNode::GetIndexFromId(node.GetId());

    // Only attempt to remove, if the index does not fall beyond the size.
    if (index < _bits.GetSize()) { 
        if (_bits.IsSet(index)) {
            _bits.Clear(index);

            // The node was contained in the set.
            return true;
        }
    }
    
    // Node was not contained in the set.
    return false;
}

void
VdfNodeSet::_Grow(size_t size)
{
    // Do we need to grow the capacity? The growth factor is 1.5.
    const size_t capacity = size + (size / 2);
    if (capacity > _bits.GetSize()) {
        _bits.ResizeKeepContent(capacity);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
