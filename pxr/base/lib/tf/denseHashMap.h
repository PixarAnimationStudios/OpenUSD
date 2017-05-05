//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef TF_DENSE_HASH_MAP_H
#define TF_DENSE_HASH_MAP_H

/// \file tf/denseHashMap.h

#include "pxr/pxr.h"
#include "pxr/base/tf/hashmap.h"

#include <vector>

#include <boost/compressed_pair.hpp>
#include <boost/operators.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfDenseHashMap
///
/// This is a space efficent container that mimics the TfHashMap API that
/// uses a vector for storage when the size of the map is small.
///
/// When the map gets bigger than \p Threshold a TfHashMap is allocated
/// that is used to accelerate lookup in the vector.
///
/// \warning This differs from a TfHashMap in so far that inserting and
/// removing elements invalidate all iterators of the container.
///
template <
    class    Key,
    class    Data,
    class    HashFn,
    class    EqualKey  = std::equal_to<Key>,
    unsigned Threshold = 128
>
class TfDenseHashMap :
    private boost::equality_comparable<
        TfDenseHashMap<Key, Data, HashFn, EqualKey, Threshold>
    >
{
public:

    typedef std::pair<const Key, Data> value_type;
    typedef Key                        key_type;
    typedef Data                       mapped_type;

////////////////////////////////////////////////////////////////////////////////

private:

    // This helper implements a std::pair with an assignment operator that
    // uses placement new instead of assignment.  The benefit here is that 
    // the two elements of the pair may be const.
    //
    struct _InternalValueType 
    {
        _InternalValueType() {}

        _InternalValueType(const Key &k, const Data &d)
        :   _value(k, d) {}

        _InternalValueType &operator=(const _InternalValueType &rhs) {

            if (this != &rhs) {
                // Since value_type's first member is const we need to 
                // use placement new to put the new element in place.  Just
                // make sure we destruct the element we are about to overwrite.
                _value.value_type::~value_type();
                new (&_value) value_type(rhs.GetValue());
            }

            return *this;
        }

        value_type &GetValue() {
            return _value;
        }

        const value_type &GetValue() const {
            return _value;
        }

        void swap(_InternalValueType &rhs) {

            // We do this in order to take advantage of a potentially fast
            // swap implementation.
            Key tmp = _value.first;

            _value.first.Key::~Key();
            new (const_cast<Key *>(&_value.first)) Key(rhs._value.first);

            rhs._value.first.Key::~Key();
            new (const_cast<Key *>(&rhs._value.first)) Key(tmp);

            std::swap(_value.second, rhs._value.second);
        }

    private:

        // Data hold by _InternalValueType.  Note that the key portion of
        // value_type maybe const.
        value_type _value;
    };

    // The vector type holding all data for this dense hash map.
    typedef std::vector<_InternalValueType> _Vector;

    // The hash map used when the map holds more than Threshold elements.
    typedef TfHashMap<Key, size_t, HashFn, EqualKey> _HashMap;

    // Note that we don't just use _Vector::iterator for accessing elements
    // of the TfDenseHashMap.  This is because the vector's iterator would
    // expose the _InternalValueType _including_ its assignment operator
    // that allows overwriting keys.
    //
    // Clearly not a good thing.
    //
    // Therefore we use boost::iterator_facade to create an iterator that uses
    // the map's value_type as externally visible type.
    //
    template <class ElementType, class UnderlyingIterator>
    class _IteratorBase :
        public boost::iterator_facade<
            _IteratorBase<ElementType, UnderlyingIterator>,
            ElementType,
            boost::bidirectional_traversal_tag>
    {
    public:

        // Empty ctor.
        _IteratorBase() {}

        // Allow conversion of an iterator to a const_iterator.
        template<class OtherIteratorType>
        _IteratorBase(const OtherIteratorType &rhs)
        :   _iter(rhs._GetUnderlyingIterator()) {}

    private:

        friend class TfDenseHashMap;
        friend class boost::iterator_core_access;

        // Ctor from an underlying iterator.
        _IteratorBase(const UnderlyingIterator &iter)
        :   _iter(iter) {}

        template<class OtherIteratorType>
        bool equal(const OtherIteratorType &rhs) const {
            return _iter == rhs._iter;
        }
        
        void increment() {
            ++_iter;
        }

        void decrement() {
            --_iter;
        }
        
        ElementType &dereference() const {
            // The dereference() method accesses the correct value_type (ie.
            // the one with potentially const key_type.  This way, clients don't
            // see the assignment operator of _InternalValueType.
            return _iter->GetValue();
        }

        UnderlyingIterator _GetUnderlyingIterator() const {
            return _iter;
        }

    private:
        
        UnderlyingIterator _iter;
    };

////////////////////////////////////////////////////////////////////////////////

public:

    /// An iterator type for this map.  Note that it provides access to the
    /// This::value_type only.
    typedef
        _IteratorBase<value_type, typename _Vector::iterator>
        iterator; 

    /// An iterator type for this map.  Note that it provides access to the
    /// This::value_type only.
    typedef
        _IteratorBase<const value_type, typename _Vector::const_iterator>
        const_iterator; 

    /// Return type for insert() method.
    typedef std::pair<iterator, bool> insert_result;

public:

    /// Ctor.
    ///
    explicit TfDenseHashMap(
        const HashFn   &hashFn   = HashFn(),
        const EqualKey &equalKey = EqualKey())
    {
        _hash() = hashFn;
        _equ()  = equalKey;
    }

    /// Construct with range.
    ///
    template <class Iterator>
    TfDenseHashMap(Iterator begin, Iterator end) {
        insert(begin, end);
    }

    /// Copy Ctor.
    ///
    TfDenseHashMap(const TfDenseHashMap &rhs)
    :   _vectorHashFnEqualFn(rhs._vectorHashFnEqualFn) {
        if (rhs._h)
            _h.reset(new _HashMap(*rhs._h));
    }

    /// Assignment operator.
    ///
    TfDenseHashMap &operator=(TfDenseHashMap rhs) {
        swap(rhs);
        return *this;
    }

    /// Equality operator.
    ///
    bool operator==(const TfDenseHashMap &rhs) const {

        if (size() != rhs.size())
            return false;

        //XXX: Should we compare the HashFn and EqualKey too?
        const_iterator tend = end(), rend = rhs.end();

        for(const_iterator iter = begin(); iter != tend; ++iter) {
            const_iterator riter = rhs.find(iter->first);
            if (riter == rend)
                return false;
            if (iter->second != riter->second)
                return false;
        }

        return true;
    }

    /// Erases all of the elements
    ///
    void clear() {
        _vec().clear();
        _h.reset();
    }

    /// Swaps the contents of two maps.
    ///
    void swap(TfDenseHashMap &rhs) {
        _vectorHashFnEqualFn.swap(rhs._vectorHashFnEqualFn);
        _h.swap(rhs._h);
    }

    /// \c true if the \c map's size is 0.
    ///
    bool empty() const {
        return _vec().empty();
    }

    /// Returns the size of the map.
    ///
    size_t size() const {
        return _vec().size();
    }

    /// Returns an const_iterator pointing to the beginning of the map.
    ///
    iterator begin() {
        return _vec().begin();
    }

    /// Returns an const_iterator pointing to the end of the map.
    ///
    iterator end() {
        return _vec().end();
    }

    /// Returns an const_iterator pointing to the beginning of the map.
    ///
    const_iterator begin() const {
        return _vec().begin();
    }
    
    /// Returns an const_iterator pointing to the end of the map.
    ///
    const_iterator end() const {
        return _vec().end();
    }

    /// Finds the element with key \p k.
    ///
    iterator find(const key_type &k) {
        if (_h) {
            typename _HashMap::const_iterator iter = _h->find(k);
            if (iter == _h->end())
                return end();

            return _vec().begin() + iter->second;
        } else {
            return _FindInVec(k);
        }
    }

    /// Finds the element with key \p k.
    ///
    const_iterator find(const key_type &k) const {
        if (_h) {
            typename _HashMap::const_iterator iter = _h->find(k);
            if (iter == _h->end())
                return end();

            return _vec().begin() + iter->second;
        } else {
            return _FindInVec(k);
        }
    }

    /// Returns the number of elemens with key \p k.  Which is either 0 or 1.
    ///
    size_t count(const key_type &k) const {
        return find(k) != end();
    }

    /// Returns a pair of <iterator, bool> where iterator points to the element
    /// in the list and bool is true if a new element was inserted.
    ///
    insert_result insert(const value_type &v) {
        if (_h) {
            // Attempt to insert the new index.  If this fails, we can't insert
            // v.
            std::pair<typename _HashMap::iterator, bool> res =
                _h->insert(std::make_pair(v.first, size()));

            if (!res.second)
                return insert_result(_vec().begin() + res.first->second, false);
        } else {
            // Bail if already inserted.
            iterator iter = _FindInVec(v.first);
            if (iter != end())
                return insert_result(iter, false);
        }

        // Insert at end and create table if necessary.
        _vec().push_back(_InternalValueType(v.first, v.second));
        _CreateTableIfNeeded();

        return insert_result(std::prev(end()), true);
    }

    /// Insert a range into the hash map.  Note that \p i0 and \p i1 can't 
    /// point into the hash map.
    ///
    template<class IteratorType>
    void insert(IteratorType i0, IteratorType i1) {
        // Assume elements are more often than not unique, so if the sum of the
        // current size and the size of the range is greater than or equal to
        // the threshold, we create the table immediately so we don't do m*n
        // work before creating the table.
        if (size() + std::distance(i0, i1) >= Threshold)
            _CreateTable();

        // Insert elements.
        for(IteratorType iter = i0; iter != i1; ++iter)
            insert(*iter);
    }

    /// Insert a range of unique elements into the container.  [begin, end)
    /// *must not* contain any duplicate elements.
    ///
    template <class Iterator>
    void insert_unique(Iterator begin, Iterator end) {
        // Special-case empty container.
        if (empty()) {
            _vec().assign(begin, end);
            _CreateTableIfNeeded();
        } else {
            // Just insert, since duplicate checking will use the hash.
            insert(begin, end);
        }
    }

    /// Indexing operator.  Inserts a default constructed DataType() for \p key
    /// if there is no value for \p key already.
    ///
    /// Returns a reference to the value type for \p key.
    ///
    Data &operator[](const key_type &key) {
        return insert(value_type(key, Data())).first->second;
    }

    /// Erase element with key \p k.  Returns the number of elements erased.
    ///
    size_t erase(const key_type &k) {

        iterator iter = find(k);
        if (iter != end()) {
            erase(iter);
            return 1;
        }
        return 0;
    }

    /// Erases element pointed to by \p iter.
    ///
    void erase(const iterator &iter) {

        // Erase key from hash table if applicable.
        if (_h)
            _h->erase(iter->first);
    
        // If we are not removing that last element...
        if (iter != std::prev(end())) {
    
            // Need to get the underlying vector iterator directly, because
            // we want to operate on the vector.
            typename _Vector::iterator vi = iter._GetUnderlyingIterator();
    
            // ... move the last element into the erased placed.
            std::swap(*vi, _vec().back());
    
            // ... and update the moved element's index.
            if (_h)
                (*_h)[vi->GetValue().first] = vi - _vec().begin();
        }
    
        _vec().pop_back();
    }

    /// Erases a range from the map.
    ///
    void erase(iterator i0, iterator i1) {

        if (_h) {
            for(iterator iter = i0; iter != i1; ++iter)
                _h->erase(iter->first);
        }

        typename _Vector::const_iterator vremain = _vec().erase(
            i0._GetUnderlyingIterator(), i1._GetUnderlyingIterator());

        if (_h) {
            for(; vremain != _vec().end(); ++vremain)
                (*_h)[vremain->GetValue().first] = vremain - _vec().begin();
        }
    }

    /// Optimize storage space.
    ///
    void shrink_to_fit() {

        // Shrink the vector to best size.
        //XXX: When switching to c++0x we should call _vec().shrink_to_fit().
        _Vector(_vec()).swap(_vec());

        if (!_h)
            return;

        size_t sz = size();

        // If we have a hash map and are underneath the threshold, discard it.
        if (sz < Threshold) {

            _h.reset();

        } else {

            // Otherwise, allocate a new hash map with the optimal size.
            _h.reset(new _HashMap(sz, _hash(), _equ()));
            for(size_t i=0; i<sz; ++i)
                _h->insert(std::make_pair(_vec()[i].GetValue().first, i));
        }
    }

    /// Reserve space.
    ///
    void reserve(size_t n) {
        _vec().reserve(n);
    }

////////////////////////////////////////////////////////////////////////////////

private:

    // Helper to access the storage vector.
    _Vector &_vec() {
        return _vectorHashFnEqualFn.first().first();
    }

    // Helper to access the hash functor.
    HashFn &_hash() {
        return _vectorHashFnEqualFn.first().second();
    }

    // Helper to access the equality functor.
    EqualKey &_equ() {
        return _vectorHashFnEqualFn.second();
    }

    // Helper to access the storage vector.
    const _Vector &_vec() const {
        return _vectorHashFnEqualFn.first().first();
    }

    // Helper to access the hash functor.
    const HashFn &_hash() const {
        return _vectorHashFnEqualFn.first().second();
    }

    // Helper to access the equality functor.
    const EqualKey &_equ() const {
        return _vectorHashFnEqualFn.second();
    }

    // Helper to linear-search the vector for a key.
    inline iterator _FindInVec(const key_type &k) {
        _Vector &vec = _vec();
        EqualKey &equ = _equ();
        typename _Vector::iterator iter = vec.begin(), end = vec.end();
        for (; iter != end; ++iter) {
            if (equ(iter->GetValue().first, k))
                break;
        }
        return iter;
    }

    // Helper to linear-search the vector for a key.
    inline const_iterator _FindInVec(const key_type &k) const {
        _Vector const &vec = _vec();
        EqualKey const &equ = _equ();
        typename _Vector::const_iterator iter = vec.begin(), end = vec.end();
        for (; iter != end; ++iter) {
            if (equ(iter->GetValue().first, k))
                break;
        }
        return iter;
    }

    // Helper to create the acceleration table if size dictates.
    inline void _CreateTableIfNeeded() {
        if (size() >= Threshold) {
            _CreateTable();
        }
    }

    // Unconditionally create the acceleration table if it doesn't already
    // exist.
    inline void _CreateTable() {
        if (!_h) {
            _h.reset(new _HashMap(Threshold, _hash(), _equ()));
            for(size_t i=0; i < size(); ++i)
                _h->insert(std::make_pair(_vec()[i].GetValue().first, i));
        }
    }

    // Vector holding all elements along with the EqualKey functor.  Since
    // sizeof(EqualKey) == 0 in many cases we use a compressed_pair to not
    // pay a size penality.

    typedef
        boost::compressed_pair<
            boost::compressed_pair<_Vector, HashFn>,
            EqualKey>
        _VectorHashFnEqualFn;

    _VectorHashFnEqualFn _vectorHashFnEqualFn;

    // Optional hash map that maps from keys to vector indices.
    boost::scoped_ptr<_HashMap> _h;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_DENSE_HASH_MAP_H
