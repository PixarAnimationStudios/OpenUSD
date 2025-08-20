//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_LINEAR_MAP_H
#define PXR_EXEC_VDF_LINEAR_MAP_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/tf/smallVector.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class VdfLinearMap
///
/// \brief Used for storing small maps with cheaply comparable key types.
///
/// Works like a map or TfHashMap, but find() is implemented with a linear
/// search.  This is more efficient for storing smallish numbers of elements,
/// especially when key comparison is quick.
///
/// \todo Move this to Tf.
///

template <typename K, typename V>
class VdfLinearMap
{
private:
    typedef std::pair<K, V> _Pair;

    typedef TfSmallVector<_Pair, 1> _Vector;


public:
    typedef K key_type;
    typedef V mapped_type;
    typedef _Pair value_type;
    typedef V* pointer;
    typedef V& reference;
    typedef const V& const_reference;
    typedef typename _Vector::size_type size_type;
    typedef typename _Vector::difference_type difference_type;
    typedef typename _Vector::iterator iterator;
    typedef typename _Vector::reverse_iterator reverse_iterator;
    typedef typename _Vector::const_iterator const_iterator;
    typedef typename _Vector::const_reverse_iterator const_reverse_iterator;


    //! Creates an empty vector.
    VdfLinearMap() {}
    
    //! Creates a map with the given size.
    VdfLinearMap(size_t size) {
        _vec.reserve(size);
    }


    //! Returns an const_iterator pointing to the beginning of the map.
    iterator begin() {
        return _vec.begin();
    }

    //! Returns an const_iterator pointing to the end of the map.
    iterator end() {
        return _vec.end();
    }

    //! Returns an reverse_iterator pointing to the beginning of the map.
    reverse_iterator rbegin() {
        return _vec.rbegin();
    }

    //! Returns an reverse_iterator pointing to the end of the map.
    reverse_iterator rend() {
        return _vec.rend();
    }

    //! Returns an const_iterator pointing to the beginning of the map.
    const_iterator begin() const {
        return _vec.begin();
    }

    //! Returns an const_iterator pointing to the end of the map.
    const_iterator end() const {
        return _vec.end();
    }

    //! Returns an const_reverse_iterator pointing to the beginning of the map.
    const_reverse_iterator rbegin() const {
        return _vec.rbegin();
    }

    //! Returns an const_reverse_iterator pointing to the end of the map.
    const_reverse_iterator rend() const {
        return _vec.rend();
    }

    //! Returns the size of the map.
    size_type size() const {
        return _vec.size();
    }

    //! Returns the largest possible size of the map.
    size_type max_size() const {
        return _vec.max_size();
    }

    //! \c true if the \c map's size is 0.
    bool empty() const {
        return _vec.empty();
    }


    //! Swaps the contents of two map.
    void swap(VdfLinearMap& x) {
        _vec.swap(x._vec);
    }

    //! Erases all of the elements
    void clear() {
        _vec.clear();
    }


    //! Finds the element with key \p k.
    iterator find(const key_type& k);

    //! Finds the element with key \p k.
    const_iterator find(const key_type& k) const;

    //! Returns the number of elemens with key \p k.
    size_t count(const key_type& k) const;

    //! Test two maps for equality.
    bool operator==(const VdfLinearMap& x) const {
        return _vec == x._vec;
    }

    bool operator!=(const VdfLinearMap& x) const {
        return _vec != x._vec;
    }

    //! Returns a pair of <iterator, bool> where iterator points to the element
    //! in the list and bool is true if a new element was inserted.
    //!
    std::pair<iterator, bool> insert(const value_type& x);

private:

    _Vector _vec;
};


///////////////////////////////////////////////////////////////////////////////


template <typename K, typename V>
typename VdfLinearMap<K, V>::iterator
VdfLinearMap<K, V>::find(const key_type& k)
{
    for (iterator i = begin(); i != end(); ++i) {
        if (i->first == k) {
            return i;
        }
    }

    return end();
}

template <typename K, typename V>
typename VdfLinearMap<K, V>::const_iterator
VdfLinearMap<K, V>::find(const key_type& k) const
{
    const_iterator endIter = end();
    for (const_iterator i = begin(); i != endIter; ++i) {
        if (i->first == k) {
            return i;
        }
    }

    return endIter;
}

template <typename K, typename V>
size_t
VdfLinearMap<K, V>::count(const key_type& k) const
{
    size_t count = 0;
    for (const_iterator i = begin(); i != end(); ++i) {
        if (i->first == k) {
            ++count;
        }
    }

    return count;
}

template <typename K, typename V>
std::pair< typename VdfLinearMap<K, V>::iterator, bool> 
VdfLinearMap<K, V>::insert(const value_type& x)
{
    iterator i = find(x.first);
    if ( i != end() ) {
        return std::make_pair(i, false);
    }

    _vec.push_back(x);
    return std::make_pair(std::prev(end()), true);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
