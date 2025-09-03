//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_INDEXED_DATA_ITERATOR_H
#define PXR_EXEC_VDF_INDEXED_DATA_ITERATOR_H

/// \file

#include "pxr/pxr.h"

#include <iterator>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// This is a simple "iterator filter" that erases the type and traits of the
/// container used by VdfIndexedData<T> to store its indices and data.  It
/// allows VdfIndexedData to supply a simple iterator with few guarantees or
/// inferences, which client code can use with e.g. STL algorithms, while still
/// preserving most of VdfIndexedData's ability to change container
/// implementations without disrupting clients.
///
/// This models a const_iterator with forward traversal capability (only).
///
template <typename IteratedType>
class VdfIndexedDataIterator
{
public:
    using BaseIterator = typename std::vector<IteratedType>::const_iterator;

    using iterator_category = std::forward_iterator_tag;
    using value_type = typename BaseIterator::value_type;
    using reference = typename BaseIterator::reference;
    using pointer = typename BaseIterator::pointer;
    using difference_type = typename BaseIterator::difference_type;

    // Only default construction is publicly allowed, and the resulting
    // object is invalid.
    VdfIndexedDataIterator() = default;

    reference operator*() const { return *_baseIterator; }
    pointer operator->() const { return &(*_baseIterator); }

    VdfIndexedDataIterator &operator++() {
        ++_baseIterator;
        return *this;
    }

    VdfIndexedDataIterator operator++(int) {
        VdfIndexedDataIterator r(*this);
        ++_baseIterator;
        return r;
    }

    bool operator==(const VdfIndexedDataIterator &rhs) const {
        return _baseIterator == rhs._baseIterator;
    }

    bool operator!=(const VdfIndexedDataIterator &rhs) const {
        return _baseIterator != rhs._baseIterator;
    }

private:
    // Allow VdfIndexedData (only) to construct these.
    template <typename T>
    friend class VdfIndexedData;

    explicit VdfIndexedDataIterator(const BaseIterator& iter) :
        _baseIterator(iter) {}

    // The underlying base iterator
    BaseIterator _baseIterator;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // #ifndef PXR_EXEC_VDF_INDEXED_DATA_ITERATOR_H
