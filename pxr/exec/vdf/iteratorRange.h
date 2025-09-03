//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_ITERATOR_RANGE_H
#define PXR_EXEC_VDF_ITERATOR_RANGE_H

/// \file

#include "pxr/pxr.h"

#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfIteratorRange
///
/// This class allows for construction of iterable ranges.
///
template<typename Iterator>
class VdfIteratorRange
{
public:

    /// Type of the elements this range gives access to.
    ///
    using value_type =
        typename std::remove_cv<typename Iterator::value_type>::type;

    /// Type of iterator used in the range.
    ///
    using iterator = Iterator;

    /// Type of constant iterator used in the range.
    ///
    using const_iterator = Iterator;

    /// Constructs an iterable range from a begin iterator.
    ///
    template <typename... Args>
    VdfIteratorRange(Args&&... args) :
        _begin(std::forward<Args>(args)...),
        _end(_begin) {
        _end.AdvanceToEnd();
    }

    /// Constructs an iterable range from \p begin and \p end iterators.
    ///
    VdfIteratorRange(Iterator begin, Iterator end) : _begin(begin), _end(end) {}

    /// Returns an iterator to the beginning of the iterable range.
    ///
    Iterator begin() const {
        return _begin;
    }

    /// Returns an iterator to the end of the iterable range.
    ///
    Iterator end() const {
        return _end;
    }

    /// Returns \c true if the range is empty.
    ///
    bool IsEmpty() const {
        return _begin == _end;
    }

private:

    // The begin iterator of this range.
    Iterator _begin;

    // The end iterator of this range.
    Iterator _end;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
