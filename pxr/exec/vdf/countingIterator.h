//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_COUNTING_ITERATOR
#define PXR_EXEC_VDF_COUNTING_ITERATOR

#include "pxr/pxr.h"

#include <iterator>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// Random access counting iterator that simply operates on an underlying
/// integer index.
///
template <typename T>
class Vdf_CountingIterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = const T;
    using reference = const T;
    using pointer = const T *;
    using difference_type = std::make_signed_t<value_type>;

    Vdf_CountingIterator() : _integer() {}
    explicit Vdf_CountingIterator(T i) : _integer(i) {}

    reference operator*() const { return _integer; }
    pointer operator->() const { return &_integer; }
    value_type operator[](const difference_type n) const  {
        return _integer + n;
    }

    Vdf_CountingIterator &operator++() {
        ++_integer;
        return *this; 
    }

    Vdf_CountingIterator operator++(int) {
        Vdf_CountingIterator r(*this);
        ++_integer;
        return r;
    }

    Vdf_CountingIterator operator+(const difference_type n) const {
        Vdf_CountingIterator r(*this);
        r._integer += n;
        return r;
    }

    Vdf_CountingIterator &operator+=(const difference_type n) {
        _integer += n;
        return *this;
    }

    Vdf_CountingIterator &operator--() {
        --_integer;
        return *this;
    }

    Vdf_CountingIterator operator--(int) {
        Vdf_CountingIterator r(*this);
        --_integer;
        return r;
    }

    Vdf_CountingIterator operator-(const difference_type n) const {
        Vdf_CountingIterator r(*this);
        r._integer -= n;
        return r;
    }

    Vdf_CountingIterator &operator-=(const difference_type n) {
        _integer -= n;
        return *this;
    }

    difference_type operator-(const Vdf_CountingIterator &rhs) const {
        return _integer - rhs._integer;
    }

    bool operator==(const Vdf_CountingIterator &rhs) const {
        return _integer == rhs._integer;
    }

    bool operator!=(const Vdf_CountingIterator &rhs) const {
        return _integer != rhs._integer;
    }

    bool operator<(const Vdf_CountingIterator &rhs) const {
        return _integer < rhs._integer;
    }

    bool operator<=(const Vdf_CountingIterator &rhs) const {
        return _integer <= rhs._integer;
    }

    bool operator>(const Vdf_CountingIterator &rhs) const {
        return _integer > rhs._integer;
    }

    bool operator>=(const Vdf_CountingIterator &rhs) const {
        return _integer >= rhs._integer;
    }

private:
    T _integer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
