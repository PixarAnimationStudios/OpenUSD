//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_DENSE_HASH_SET_H
#define PXR_BASE_TF_DENSE_HASH_SET_H

/// \file tf/denseHashSet.h

#include "pxr/pxr.h"
#include "pxr/base/arch/attributes.h"
#include "pxr/base/tf/hashmap.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfDenseHashSet
///
/// This is a space efficient container that mimics the TfHashSet API that
/// uses a vector for storage when the size of the set is small.
///
/// When the set gets bigger than \p Threshold a TfHashMap is allocated
/// that is used to accelerate lookup in the vector.
///
/// \warning This differs from a TfHashSet in so far that inserting and
/// removing elements invalidate all iterators of the container.
///
template <
    class    Element,
    class    HashFn,
    class    EqualElement  = std::equal_to<Element>,
    unsigned Threshold = 128
>
class TfDenseHashSet
{
public:

    typedef Element value_type;

////////////////////////////////////////////////////////////////////////////////

private:

    // The vector type holding all data for this dense hash set.
    typedef std::vector<Element> _Vector;

    // The hash map used when the map holds more than Threshold elements.
    typedef TfHashMap<Element, size_t, HashFn, EqualElement> _HashMap;

////////////////////////////////////////////////////////////////////////////////

public:

    /// An iterator type for this set.  Note that this one is const as well,
    /// as we can't allow in-place modification of elements due to the 
    /// potentially allocated hash map.
    typedef typename _Vector::const_iterator iterator; 

    /// A const_iterator type for this set.
    typedef typename _Vector::const_iterator const_iterator; 

    /// Return type for insert() method.
    typedef std::pair<const_iterator, bool> insert_result;

public:

    /// Ctor.
    ///
    explicit TfDenseHashSet(
        const HashFn       &hashFn       = HashFn(),
        const EqualElement &equalElement = EqualElement())
    {
        _hash() = hashFn;
        _equ()  = equalElement;
    }

    /// Copy Ctor.
    ///
    TfDenseHashSet(const TfDenseHashSet &rhs)
    :   _storage(rhs._storage) {
        if (rhs._h) {
            _h = std::make_unique<_HashMap>(*rhs._h);
        }
    }

    /// Move Ctor.
    ///
    TfDenseHashSet(TfDenseHashSet &&rhs) = default;

    /// Construct from range.
    ///
    template <class Iterator>
    TfDenseHashSet(Iterator begin, Iterator end) {
        insert(begin, end);
    }

    /// Construct from an initializer_list.
    ///
    TfDenseHashSet(std::initializer_list<Element> l) {
        insert(l.begin(), l.end());
    }

    /// Copy assignment operator.
    ///
    TfDenseHashSet &operator=(const TfDenseHashSet &rhs) {
        if (this != &rhs) {
            TfDenseHashSet temp(rhs);
            temp.swap(*this);
        }
        return *this;
    }

    /// Move assignment operator.
    ///
    TfDenseHashSet &operator=(TfDenseHashSet &&rhs) = default;

    /// Assignment from an initializer_list.
    ///
    TfDenseHashSet &operator=(std::initializer_list<Element> l) {
        clear();
        insert(l.begin(), l.end());
        return *this;
    }

    /// Equality operator.
    ///
    bool operator==(const TfDenseHashSet &rhs) const {

        if (size() != rhs.size())
            return false;

        //XXX: Should we compare the HashFn and EqualElement too?
        const_iterator tend = end();

        for(const_iterator iter = begin(); iter != tend; ++iter) {
            if (!rhs.count(*iter))
                return false;
        }

        return true;
    }

    bool operator!=(const TfDenseHashSet &rhs) const {
        return !(*this == rhs);
    }

    /// Erases all of the elements
    ///
    void clear() {
        _vec().clear();
        _h.reset();
    }

    /// Swaps the contents of two sets.
    ///
    void swap(TfDenseHashSet &rhs) {
        _storage.swap(rhs._storage);
        _h.swap(rhs._h);
    }

    /// \c true if the \c set's size is 0.
    ///
    bool empty() const {
        return _vec().empty();
    }

    /// Returns the size of the set.
    ///
    size_t size() const {
        return _vec().size();
    }

    /// Returns an const_iterator pointing to the beginning of the set.
    ///
    const_iterator begin() const {
        return _vec().begin();
    }
    
    /// Returns an const_iterator pointing to the end of the set.
    ///
    const_iterator end() const {
        return _vec().end();
    }

    /// Finds the element with key \p k.
    ///
    const_iterator find(const Element &k) const {

        if (_h) {
            typename _HashMap::const_iterator iter = _h->find(k);
            if (iter == _h->end())
                return end();
    
            return _vec().begin() + iter->second;
        }
    
        typename _Vector::const_iterator iter, end = _vec().end();
    
        for(iter = _vec().begin(); iter != end; ++iter)
            if (_equ()(*iter, k))
                break;
    
        return iter;
    }

    /// Returns the number of elements with key \p k.  Which is either 0 or 1.
    ///
    size_t count(const Element &k) const {
        return find(k) != end();
    }

    /// Returns a pair of <iterator, bool> where iterator points to the element
    /// in the list and bool is true if a new element was inserted.
    ///
    insert_result insert(const value_type &v) {

        if (_h) {
    
            // Attempt to insert the new index.  If this fails, we can't
            // insert v.
    
            std::pair<typename _HashMap::iterator, bool> res =
                _h->insert(std::make_pair(v, size()));
    
            if (!res.second)
                return insert_result(_vec().begin() + res.first->second, false);
    
        } else {
    
            // Bail if already inserted.
            const_iterator iter = find(v);
            if (iter != end())
                return insert_result(iter, false);
        }

        // Insert at end and create table if necessary.
        _vec().push_back(v);
        _CreateTableIfNeeded();

        return insert_result(std::prev(end()), true);
    }

    /// Insert a range into the hash set.  Note that \p i0 and \p i1 can't 
    /// point into the hash set.
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
        for (IteratorType iter = i0; iter != i1; ++iter)
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

    /// Erase element with key \p k.  Returns the number of elements erased.
    ///
    size_t erase(const Element &k) {

        const_iterator iter = find(k);
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
            _h->erase(*iter);
    
        // If we are not removing that last element...
        if (iter != std::prev(end())) {
            using std::swap;

            // ... move the last element into the erased placed.
            // Note that we can cast constness away because we explicitly update
            // the TfHashMap _h below.
            swap(*const_cast<Element *>(&(*iter)), _vec().back());
    
            // ... and update the moved element's index.
            if (_h)
                (*_h)[*iter] = iter - _vec().begin();
        }
    
        _vec().pop_back();
    }

    /// Erases a range from the set.
    ///
    void erase(const iterator &i0, const iterator &i1) {

        if (_h) {
            for(const_iterator iter = i0; iter != i1; ++iter)
                _h->erase(iter->first);
        }

        const_iterator vremain = _vec().erase(i0, i1);

        if (_h) {
            for(; vremain != _vec().end(); ++vremain)
                (*_h)[*vremain] = vremain - _vec().begin();
        }
    }

    /// Optimize storage space.
    ///
    void shrink_to_fit() {

        // Shrink the vector to best size.
        _vec().shrink_to_fit();

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
                (*_h)[_vec()[i]] = i;
        }
    }

    /// Index into set via \p index.
    ///
    const Element &operator[](size_t index) const {
        TF_VERIFY(index < size());
        return _vec()[index];
    }

////////////////////////////////////////////////////////////////////////////////

private:

    // Helper to access the storage vector.
    _Vector &_vec() {
        return _storage.vector;
    }

    // Helper to access the hash functor.
    HashFn &_hash() {
        return _storage;
    }

    // Helper to access the equality functor.
    EqualElement &_equ() {
        return _storage;
    }

    // Helper to access the storage vector.
    const _Vector &_vec() const {
        return _storage.vector;
    }

    // Helper to access the hash functor.
    const HashFn &_hash() const {
        return _storage;
    }

    // Helper to access the equality functor.
    const EqualElement &_equ() const {
        return _storage;
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
                (*_h)[_vec()[i]] = i;
        }
    }

    // Since sizeof(EqualElement) == 0 and sizeof(HashFn) == 0 in many cases
    // we use the empty base optimization to not pay a size penalty.
    // In C++20, explore using [[no_unique_address]] as an alternative
    // way to get this optimization.
    struct ARCH_EMPTY_BASES _CompressedStorage :
        private EqualElement, private HashFn {
        static_assert(!std::is_same<EqualElement, HashFn>::value,
                      "EqualElement and HashFn must be distinct types.");
        _CompressedStorage() = default;
        _CompressedStorage(const EqualElement& equal, const HashFn& hashFn)
            : EqualElement(equal), HashFn(hashFn) {}

        void swap(_CompressedStorage& other) {
            using std::swap;
            vector.swap(other.vector);
            swap(static_cast<EqualElement&>(*this),
                 static_cast<EqualElement&>(other));
            swap(static_cast<HashFn&>(*this), static_cast<HashFn&>(other));
        }
        _Vector vector;
        friend class TfDenseHashSet;
    };
    _CompressedStorage _storage;

    // Optional hash map that maps from keys to vector indices.
    std::unique_ptr<_HashMap> _h;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_DENSE_HASH_SET_H
