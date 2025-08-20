//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_BOXED_CONTAINER_H
#define PXR_EXEC_VDF_BOXED_CONTAINER_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/boxedContainerTraits.h"

#include "pxr/base/tf/smallVector.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Each range represents a logical group of elements stored in a
/// Vdf_BoxedContainer.
///
class Vdf_BoxedRanges
{
public:
    /// A range of data elements as denoted by [ begin, end ) indices. Each
    /// range of elements represents a logical group of data elements.
    ///
    struct Range
    {
        unsigned int begin;
        unsigned int end;
    };

    /// Constructs an empty set of boxed ranges.
    ///
    Vdf_BoxedRanges() = default;

    /// Constructs a set with one range containing all \p n elements.
    ///
    explicit Vdf_BoxedRanges(unsigned int n)
        : _ranges(1, Range{0, n})
    {}

    /// Returns the number of individual ranges stored in this container.
    ///
    unsigned int GetNumRanges() const {
        return _ranges.size();
    }

    /// Returns the range at index \p i.  Each range represents a logical group
    /// of data elements.
    ///
    Range GetRange(unsigned int i) const {
        return _ranges[i];
    }

    /// Appends a new group.
    ///
    void AppendRange(unsigned int begin, unsigned int end) {
        _ranges.push_back(Range{begin, end});
    }

    // Overload for ADL swap idiom
    friend inline void swap(Vdf_BoxedRanges &lhs, Vdf_BoxedRanges &rhs) {
        lhs._ranges.swap(rhs._ranges);
    }

private:
    // The individual ranges stored in this vector denoted by the end element
    // index of each range.
    TfSmallVector<Range, 1> _ranges;
};

////////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_BoxedContainer
/// 
/// This simple container stores multiple values that flow through the network
/// as a single data flow element. It enables data flow of vectorized data
/// without encoding the length of that data in the topology of the network.
/// This container is transparent to client code, such that its contents can
/// be consumed just like any vectorized data.
///
template<typename T>
class Vdf_BoxedContainer
{
    static_assert(
        !Vdf_IsBoxedContainer<T>, "Recursive boxing is not allowed");

    // Adjust the local capacity of our data so that we use as much local
    // capacity as we can without growing sizeof(_DataVec).  We do this so
    // that the boxed container always fits in Vdf_VectorData::DataHolder.
    // Otherwise, we would have to separately allocate the boxed container
    // from the vector impl, which defeats the purpose of TfSmallVector's
    // local storage.
    static const TfSmallVectorBase::size_type N =
        TfSmallVectorBase::ComputeSerendipitousLocalCapacity<T>();
    using _DataVec = TfSmallVector<T, N>;

public:

    /// Constructs an empty container with no elements and no ranges.
    ///
    Vdf_BoxedContainer() = default;

    /// Constructs a container with \p n elements and one range containing all
    /// elements. Each element will be default initialized.
    ///
    explicit Vdf_BoxedContainer(unsigned int n) :
        _data(n, _DataVec::DefaultInit),
        _ranges(n)
    {}

    /// Returns \c true if \p rhs compares equal with this container.
    ///
    bool operator==(const Vdf_BoxedContainer &rhs) const {
        return _data == rhs._data;
    }

    /// Returns \c true if \p rhs does not compare equal with this container.
    ///
    bool operator!=(const Vdf_BoxedContainer &rhs) const {
        return !operator==(rhs);
    }

    /// Returns \c true if the container does not hold any elements.
    ///
    bool empty() const {
        return _data.empty();
    }

    /// Returns the number of elements stored in this container.
    ///
    size_t size() const {
        return _data.size();
    }

    /// Reserves storage for \p n elements in this container.
    ///
    void reserve(unsigned int n) {
        _data.reserve(n);
    }

    /// Returns the immutable value stored at index \p i.
    ///
    const T &operator[](unsigned int i) const {
        return _data[i];
    }

    /// Returns the mutable value stored at index \p i.
    ///
    T &operator[](unsigned int i) {
        return _data[i];
    }

    /// Returns a pointer to the immutable data elements.
    ///
    const T *data() const {
        return _data.data();
    }

    /// Returns a pointer to the mutable data elements.
    ///
    T *data() {
        return _data.data();
    }

    /// Returns the subranges of boxed data.
    ///
    const Vdf_BoxedRanges &GetRanges() const {
        return _ranges;
    }

    /// Appends the data elements [ \p begin, \p end ) to the end of the 
    /// container, and adds a new group containing those same data elements.
    ///
    template<typename Iterator>
    void AppendRange(Iterator begin, Iterator end);

    // Overload for ADL swap idiom
    friend inline void swap(Vdf_BoxedContainer &lhs, Vdf_BoxedContainer &rhs) {
        lhs._data.swap(rhs._data);
        swap(lhs._ranges, rhs._ranges);
    }

private:

    // The elements stored in this container.
    _DataVec _data;

    // The individual ranges stored in this vector denoted by the end element
    // index of each range.
    Vdf_BoxedRanges _ranges;

};

///////////////////////////////////////////////////////////////////////////////

template<typename T>
template<typename Iterator>
void
Vdf_BoxedContainer<T>::AppendRange(Iterator begin, Iterator end) {
    const unsigned int previousSize = _data.size();
    _data.insert(_data.end(), begin, end);
    _ranges.AppendRange(previousSize, _data.size());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* PXR_EXEC_VDF_BOXED_CONTAINER_H */
