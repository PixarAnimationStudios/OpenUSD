//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_SPAN_H
#define PXR_BASE_TF_SPAN_H

/// \file tf/span.h

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/diagnostic.h"

#include <cstddef>
#include <iterator>
#include <type_traits>


PXR_NAMESPACE_OPEN_SCOPE


/// \class TfSpan
/// Represents a range of contiguous elements.
///
/// This simply pairs a pointer with a size, while adding a common
/// array interface.
///
/// A span allows ranges of elements to be referenced in a container-neutral
/// manner. While it is possible to achieve that effect by simply passing around
/// raw pointers, a span has the advantage of carrying around additional size
/// information, both enabling use of common array patterns, as well as
/// providing sufficient information to perform boundary tests.
///
/// A TfSpan is implicitly convertible from common array types,
/// as well as from other spans, but preserves const-ness:
///
/// \code
///     std::vector<int> data;
///     TfSpan<int> span(data);         // Okay
///
///     VtIntArray data;
///     TfSpan<int> span = data;        // Okay
///     TfSpan<const int> cspan = span; // Okay
///
///     const std::vector<int> data;
///     TfSpan<const int> span = data;  // Okay
///
///     const std::vector<int> data;
///     TfSpan<int> span = data;        // Error! Discards cv-qualifier.
/// \endcode
///
/// Helper methods TfMakeSpan and TfMakeConstSpan are also provided to enable
/// auto-typing when constructing spans:
/// \code
///     VtIntArray data;
///     auto readOnlySpan = TfMakeConstSpan(data); // TfSpan<const int>
///     auto readWriteSpan = TfMakeSpan(data); // TfSpan<int>
/// \endcode
///
/// Spans do not own the data they reference. It is up to the user of the span
/// to ensure that the underlying data is not destructed while the span is in
/// use.
///
/// This is modelled after std::span (C++20), but does not currently include
/// any specialization for static extents.
///
template <typename T>
class TfSpan
{
public:
    using element_type = T;
    using value_type = typename std::remove_cv<T>::type;
    using pointer = T*;
    using reference = T&;
    using index_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    TfSpan() noexcept = default;

    /// Construct a span over the range of [ptr, ptr+count).
    /// In debug builds, a runtime assertion will fail if \p count > 0 and
    /// \p ptr is null. The behavior is otherwise undefined for invalid ranges.
    TfSpan(pointer ptr, index_type count)
        : _data(ptr), _size(count)
        {
            TF_DEV_AXIOM(count == 0 || ptr);
        }

    /// Construct a span over the range [first, last).
    TfSpan(pointer first, pointer last)
        : TfSpan(first, index_type(last-first))
        {
            TF_DEV_AXIOM(last >= first);
        }

    /// Construct a span from a container.
    /// The resulting span has a range of
    /// [cont.data(), cont.data()+cont.size())
    template <class Container>
    TfSpan(Container& cont,
           typename std::enable_if<
               !std::is_const<element_type>::value &&
               std::is_same<typename Container::value_type, value_type
                >::value, Container
           >::type* = 0)
        : _data(cont.data()), _size(cont.size())
        {
            TF_DEV_AXIOM(_size == 0 || _data);
        }

    /// Construct a span from a container.
    /// The resulting span has a range of
    /// [cont.data(), cont.data()+cont.size())
    template <class Container>
    TfSpan(const Container& cont,
           typename std::enable_if<
               std::is_same<typename Container::value_type, value_type
                >::value, Container
           >::type* = 0)
        : _data(cont.data()), _size(cont.size())
        {
            TF_DEV_AXIOM(_size == 0 || _data);
        }

    /// Return a pointer to the first element of the span.   
    pointer data() const noexcept { return _data; }

    /// Return the total number of elements in the span.
    index_type size() const noexcept { return _size; }

    /// Returns true if this span contains no elements, false otherwise.
    bool empty() const noexcept { return _size == 0; }

    /// Returns a reference to the \p idx'th element of the span.
    /// In debug builds, a runtime assertion will fail if \p idx is out of
    /// range. The behavior is otherwise undefined if \p idx is out of range.
    reference operator[](index_type idx) const {
        TF_DEV_AXIOM(idx < _size);
        return _data[idx];
    }

    /// Return a reference to the first element in the span.
    reference front() const {
        TF_DEV_AXIOM(!empty());
        return *begin();
    }

    /// Return a reference to the last element in the span.
    reference back() const {
        TF_DEV_AXIOM(!empty());
        return *(end() - 1);
    }

    /// Returns a non-const iterator the start of the span.
    iterator begin() const noexcept { return _data; }

    /// Returns a cons iterator to the start of the span.
    const_iterator cbegin() const noexcept { return _data; }

    /// Returns a non-const iterator to the end of the span.
    iterator end() const noexcept { return _data + _size; }

    /// Returns a const iterator to the end of the span.
    const_iterator cend() const noexcept { return _data + _size; }

    /// Returns a non-const reverse iterator the start of the span.
    reverse_iterator rbegin() const noexcept
        { return reverse_iterator(end()); }

    /// Returns a cons reverse iterator to the start of the span.
    const_reverse_iterator crbegin() const noexcept 
        { return const_reverse_iterator(cend()); }

    /// Returns a non-const reverse iterator to the end of the span.
    reverse_iterator rend() const noexcept
        { return reverse_iterator(begin()); }

    /// Returns a const reverse iterator to the end of the span.
    const_reverse_iterator crend() const noexcept
        { return const_reverse_iterator(cbegin()); }

    /// Returns a new span referencing a sub-range of this span.
    /// If \p count == -1 (or std::dynamic_extent in C++20), the new span
    /// has a range of [data()+offset, data()+size()). Otherwise, the new
    /// span has range [data()+offset, data()+offset+count).
    TfSpan<T> subspan(difference_type offset, difference_type count=-1) const {
        TF_DEV_AXIOM(offset >= 0 && (index_type)offset < _size);
        if (count == -1) {
            return TfSpan<T>(_data + offset, _size - offset);
        } else {
            TF_DEV_AXIOM(count >= 0);
            TF_DEV_AXIOM(((index_type)offset+(index_type)count) <= _size);
            return TfSpan<T>(_data + offset, count);
        }
    }

    /// Return a subspan consisting of the first \p count elements of this span.
    TfSpan<T> first(size_t count) const {
        return subspan(0, count);
    }

    /// Return a subspan consisting of the last \p count elements of this span.
    TfSpan<T> last(size_t count) const {
        TF_DEV_AXIOM(_size >= count);
        return TfSpan<T>(end() - count, count);
    }

private:
    pointer _data = nullptr;
    index_type _size = 0;
};


/// Helper for constructing a non-const TfSpan from a container.
template <typename Container>
TfSpan<typename Container::value_type>
TfMakeSpan(Container& cont)
{
    return TfSpan<typename Container::value_type>(cont);
}


/// Helper for constructing a const TfSpan from a container.
template <typename Container>
TfSpan<const typename Container::value_type>
TfMakeConstSpan(const Container& cont)
{
    return TfSpan<const typename Container::value_type>(cont);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_SPAN_H
