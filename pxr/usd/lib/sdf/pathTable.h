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
#ifndef SDF_PATHTABLE_H
#define SDF_PATHTABLE_H

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/pointerAndBits.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/work/loops.h"
#include <boost/iterator/iterator_facade.hpp>
#include <boost/noncopyable.hpp>

#include <algorithm>
#include <utility>
#include <vector>

/// \class SdfPathTable
///
/// A mapping from SdfPath to \a MappedType, somewhat similar to map<SdfPath,
/// MappedType> and TfHashMap<SdfPath, MappedType>, but with key
/// differences.  Notably:
///
/// Works exclusively with absolute paths.
///
/// Inserting a path \a p also implicitly inserts all of \a p's ancestors.
///
/// Erasing a path \a p also implicitly erases all of \a p's descendants.
///
/// The table has an order: it's a preordering of the paths in the table, but
/// with arbitrary sibling order.  Given a path \a p in the table, all other
/// paths in the table with \a p as a prefix appear contiguously, immediately
/// following \a p.  For example, suppose a table contains the paths:
///
///   {'/a/b/c', '/a', '/a/d', '/', '/a/b'}
///
/// Then there are two possible valid orderings:
///
///   ['/', '/a', '/a/d', '/a/b', '/a/b/c']
///   ['/', '/a', '/a/b', '/a/b/c', '/a/d']
///
/// In addition to the ordinary map and TfHashMap methods, this class
/// provides a method \a FindSubtreeRange, which, given a path \a p, returns
/// a pair of iterators [\a b, \a e) defining a range such that for every
/// iterator \a i in [\a b, \a e), i->first is either equal to \a p or is
/// prefixed by \a p.
///
/// Iterator Invalidation
///
/// Like most other node-based containers, iterators are only invalidated when
/// the element they refer to is removed from the table.  Note however, that
/// since removing the element with path \a p also implicitly removes all
/// elements with paths prefixed by \a p, a call to erase(\a i) may invalidate
/// many iterators.
///
template <class MappedType>
class SdfPathTable
{
public:

    typedef SdfPath key_type;
    typedef MappedType mapped_type;
    typedef std::pair<key_type, mapped_type> value_type;

private:

    // An _Entry represents an item in the table.  It holds the item's value, a
    // pointer (\a next) to the next item in the hash bucket's linked list, and
    // two pointers (\a firstChild and \a nextSibling) that describe the tree
    // structure.
    struct _Entry : boost::noncopyable {
        _Entry(value_type const &value, _Entry *n)
            : value(value)
            , next(n)
            , firstChild(0)
            , nextSiblingOrParent(0, false) {}

        // If this entry's nextSiblingOrParent field points to a sibling, return
        // a pointer to it, otherwise return null.
        _Entry *GetNextSibling() {
            return nextSiblingOrParent.template BitsAs<bool>() ?
                nextSiblingOrParent.Get() : 0;
        }
        // If this entry's nextSiblingOrParent field points to a sibling, return
        // a pointer to it, otherwise return null.
        _Entry const *GetNextSibling() const {
            return nextSiblingOrParent.template BitsAs<bool>() ?
                nextSiblingOrParent.Get() : 0;
        }

        // If this entry's nextSiblingOrParent field points to a parent, return
        // a pointer to it, otherwise return null.
        _Entry *GetParentLink() {
            return nextSiblingOrParent.template BitsAs<bool>() ? 0 :
                nextSiblingOrParent.Get();
        }
        // If this entry's nextSiblingOrParent field points to a parent, return
        // a pointer to it, otherwise return null.
        _Entry const *GetParentLink() const {
            return nextSiblingOrParent.template BitsAs<bool>() ? 0 :
                nextSiblingOrParent.Get();
        }

        // Set this entry's nextSiblingOrParent field to point to the passed
        // sibling.
        void SetSibling(_Entry *sibling) {
            nextSiblingOrParent.Set(sibling, /* isSibling */ true);
        }

        // Set this entry's nextSiblingOrParent field to point to the passed
        // parent.
        void SetParentLink(_Entry *parent) {
            nextSiblingOrParent.Set(parent, /* isSibling */ false);
        }

        // Add \a child as a child of this entry.
        void AddChild(_Entry *child) {
            // If there are already one or more children, make \a child be our
            // new first child.  Otherwise, add \a child as the first child.
            if (firstChild)
                child->SetSibling(firstChild);
            else
                child->SetParentLink(this);
            firstChild = child;
        }

        void RemoveChild(_Entry *child) {
            // Remove child from this _Entry's children.
            if (child == firstChild) {
                firstChild = child->GetNextSibling();
            } else {
                // Search the list to find the preceding child, then unlink the
                // child to remove.
                _Entry *prev, *cur = firstChild;
                do {
                    prev = cur;
                    cur = prev->GetNextSibling();
                } while (cur != child);
                prev->nextSiblingOrParent = cur->nextSiblingOrParent;
            }
        }

        // The value object mapped by this entry.
        value_type value;

        // The next field links together entries in chained hash table buckets.
        _Entry *next;

        // The firstChild and nextSiblingOrParent fields describe the tree
        // structure of paths.  An entry has one or more children when
        // firstChild is non null.  Its chlidren are stored in a singly linked
        // list, where nextSiblingOrParent points to the next entry in the list.
        //
        // The end of the list is reached when the bit stored in
        // nextSiblingOrParent is set false, indicating a pointer to the parent
        // rather than another sibling.
        _Entry *firstChild;
        TfPointerAndBits<_Entry> nextSiblingOrParent;
    };

    // Hash table's list of buckets is a vector of _Entry ptrs.
    typedef std::vector<_Entry *> _BucketVec;

public:

    // The iterator class, used to make both const and non-const 
    // iterators.  Currently only forward traversal is supported.
    template <class, class> friend class Iterator;
    template <class ValType, class EntryPtr>
    class Iterator :
        public boost::iterator_facade<Iterator<ValType, EntryPtr>,
                                      ValType, boost::forward_traversal_tag>
    {
    public:
        /// The standard requires default construction but places practically no
        /// requirements on the semantics of default-constructed iterators.
        Iterator() {}

        /// Copy constructor (also allows for converting non-const to const).
        template <class OtherVal, class OtherEntryPtr>
        Iterator(Iterator<OtherVal, OtherEntryPtr> const &other)
            : _entry(other._entry)
            {}

        /// Return an iterator \a e, defining a maximal range [\a *this, \a e)
        /// such that for all \a i in the range, \a i->first is \a
        /// (*this)->first or is prefixed by \a (*this)->first.
        Iterator GetNextSubtree() const {
            Iterator result(0);
            if (_entry) {
                if (EntryPtr sibling = _entry->GetNextSibling()) {
                    // Next subtree is next sibling, if present.
                    result._entry = sibling;
                } else {
                    // Otherwise, walk up parents until we either find one with
                    // a next sibling or run out.
                    for (EntryPtr p = _entry->GetParentLink(); p;
                         p = p->GetParentLink()) {
                        if (EntryPtr sibling = p->GetNextSibling()) {
                            result._entry = sibling;
                            break;
                        }
                    }
                }
            }
            return result;
        }

    protected:
        friend class boost::iterator_core_access;
        friend class SdfPathTable;
        template <class, class> friend class Iterator;

        explicit Iterator(EntryPtr entry)
            : _entry(entry) {}

        // Fundamental functionality to implement the iterator.
        // boost::iterator_facade will invoke these as necessary to implement
        // the full iterator public interface.

        // Iterator increment.
        inline void increment() {
            // Move to first child, if present.  Otherwise move to next subtree.
            _entry = _entry->firstChild ? _entry->firstChild :
                GetNextSubtree()._entry;
        }

        // Equality comparison.  A template, to allow comparison between const
        // and non-const iterators.
        template <class OtherVal, class OtherEntryPtr>
        bool equal(Iterator<OtherVal, OtherEntryPtr> const &other) const {
            return _entry == other._entry;
        }

        // Dereference.
        ValType &dereference() const {
            return _entry->value;
        }

        // Store pointer to current entry.
        EntryPtr _entry;
    };

    typedef Iterator<value_type, _Entry *> iterator;
    typedef Iterator<const value_type, const _Entry *> const_iterator;

    /// Result type for insert().
    typedef std::pair<iterator, bool> _IterBoolPair;

    /// Default constructor.
    SdfPathTable() : _size(0), _mask(0) {}

    /// Copy constructor.
    SdfPathTable(SdfPathTable const &other)
        : _buckets(other._buckets.size())
        , _size(0) // size starts at 0, since we insert elements.
        , _mask(other._mask)
    {
        // Walk all elements in the other container, inserting into this one,
        // and creating the right child/sibling links along the way.
        for (const_iterator i = other.begin(), end = other.end();
             i != end; ++i) {
            iterator j = _InsertInTable(*i).first;
            // Ensure first child and next sibling links are created.
            if (i._entry->firstChild and not j._entry->firstChild) {
                j._entry->firstChild =
                    _InsertInTable(i._entry->firstChild->value).first._entry;
            }
            // Ensure the nextSibling/parentLink is created.
            if (i._entry->nextSiblingOrParent.Get() and not
                j._entry->nextSiblingOrParent.Get()) {
                j._entry->nextSiblingOrParent.Set(
                    _InsertInTable(i._entry->nextSiblingOrParent.
                                   Get()->value).first._entry,
                    i._entry->nextSiblingOrParent.template BitsAs<bool>());
            }
        }
    }

    /// Destructor.
    ~SdfPathTable() {
        // Call clear to free all nodes.
        clear();
    }

    /// Assignment.
    SdfPathTable &operator=(SdfPathTable const &other) {
        if (this != &other)
            SdfPathTable(other).swap(*this);
        return *this;
    }

    /// Return an iterator to the start of the table.
    iterator begin() {
        // Return an iterator pointing to the root if this table isn't empty.
        if (empty())
            return end();
        return find(SdfPath::AbsoluteRootPath());
    }

    /// Return a const_iterator to the start of the table.
    const_iterator begin() const {
        // Return an iterator pointing to the root if this table isn't empty.
        if (empty())
            return end();
        return find(SdfPath::AbsoluteRootPath());
    }

    /// Return an iterator denoting the end of the table.
    iterator end() {
        return iterator(0);
    }

    /// Return a const_iterator denoting the end of the table.
    const_iterator end() const {
        return const_iterator(0);
    }

    /// Remove the element with path \a path from the table as well as all
    /// elements whose paths are prefixed by \a path.  Return true if any
    /// elements were removed, false otherwise.
    ///
    /// Note that since descendant paths are also erased, size() may be
    /// decreased by more than one after calling this function.
    bool erase(SdfPath const &path) {
        iterator i = find(path);
        if (i == end())
            return false;
        erase(i);
        return true;
    }

    /// Remove the element pointed to by \p i from the table as well as all
    /// elements whose paths are prefixed by \a i->first.  \a i must be a valid
    /// iterator for this table.
    ///
    /// Note that since descendant paths are also erased, size() may be
    /// decreased by more than one after calling this function.
    void erase(iterator const &i) {
        // Delete descendant nodes, if any.  Then remove from parent, finally
        // erase from hash table.
        _Entry * const entry = i._entry;
        _EraseSubtree(entry);
        _RemoveFromParent(entry);
        _EraseFromTable(entry);
    }

    /// Return an iterator to the element corresponding to \a path, or \a end()
    /// if there is none.
    iterator find(SdfPath const &path) {
        if (not empty()) {
            // Find the item in the list.
            for (_Entry *e = _buckets[_Hash(path)]; e; e = e->next) {
                if (e->value.first == path)
                    return iterator(e);
            }
        }
        return end();
    }

    /// Return a const_iterator to the element corresponding to \a path, or
    /// \a end() if there is none.
    const_iterator find(SdfPath const &path) const {
        if (not empty()) {
            // Find the item in the list.
            for (_Entry const *e = _buckets[_Hash(path)]; e; e = e->next) {
                if (e->value.first == path)
                    return const_iterator(e);
            }
        }
        return end();
    }

    /// Return a pair of iterators [\a b, \a e), describing the maximal range
    /// such that for all \a i in the range, \a i->first is \a b->first or
    /// is prefixed by \a b->first.
    std::pair<iterator, iterator>
    FindSubtreeRange(SdfPath const &path) {
        std::pair<iterator, iterator> result;
        result.first = find(path);
        result.second = result.first.GetNextSubtree();
        return result;
    }

    /// Return a pair of const_iterators [\a b, \a e), describing the maximal
    /// range such that for all \a i in the range, \a i->first is \a b->first or
    /// is prefixed by \a b->first.
    std::pair<const_iterator, const_iterator>
    FindSubtreeRange(SdfPath const &path) const {
        std::pair<const_iterator, const_iterator> result;
        result.first = find(path);
        result.second = result.first.GetNextSubtree();
        return result;
    }

    /// Return 1 if there is an element for \a path in the table, otherwise 0.
    size_t count(SdfPath const &path) const {
        return find(path) != end();
    }

    /// Return the number of elements in the table.
    inline size_t size() const { return _size; }

    /// Return true if this table is empty.
    inline bool empty() const { return not size(); }

    /// Insert \a value into the table, and additionally insert default entries
    /// for all ancestral paths of \a value.first that do not already exist in
    /// the table.
    ///
    /// Return a pair of iterator and bool.  The iterator points to the inserted
    /// element, the bool indicates whether insertion was successful.  The bool
    /// is true if \a value was successfully inserted and false if an element
    /// with path \a value.first was already present in the map.
    ///
    /// Note that since ancestral paths are also inserted, size() may be
    /// increased by more than one after calling this function.
    _IterBoolPair insert(value_type const &value) {
        // Insert in table.
        _IterBoolPair result = _InsertInTable(value);
        if (result.second) {
            // New element -- make sure the parent is inserted.
            _Entry * const newEntry = result.first._entry;
            SdfPath const &parentPath = _GetParentPath(value.first);
            if (not parentPath.IsEmpty()) {
                iterator parIter =
                    insert(value_type(parentPath, mapped_type())).first;
                // Add the new entry to the parent's children.
                parIter._entry->AddChild(newEntry);
            }
        }
        return result;
    }

    /// Shorthand for the following, where \a t is an SdfPathTable<T>.
    /// \code
    ///     t.insert(value_type(path, mapped_type())).first->second
    /// \endcode
    mapped_type &operator[](SdfPath const &path) {
        return insert(value_type(path, mapped_type())).first->second;
    }

    /// Remove all elements from the table, leaving size() == 0.  Note that this
    /// function will not shrink the number of buckets used for the hash table.
    /// To do that, swap this instance with a default constructed instance.
    /// See also \a TfReset.
    void clear() {
        // Note this leaves the size of _buckets unchanged.
        for (size_t i = 0, n = _buckets.size(); i != n; ++i) {
            _Entry *entry = _buckets[i];
            while (entry) {
                _Entry *next = entry->next;
                delete entry;
                entry = next;
            }
            _buckets[i] = 0;
        }
        _size = 0;
    }

    /// Equivalent to clear(), but destroy contained objects in parallel.  This
    /// requires that running the contained objects' destructors is thread-safe.
    void ClearInParallel() {
        // Helper function for clearing path tables.
        void Sdf_ClearPathTableInParallel(void **, size_t, void (*)(void *));
        Sdf_ClearPathTableInParallel(reinterpret_cast<void **>(_buckets.data()),
                                     _buckets.size(), _DeleteEntryChain);
    }        

    /// Swap this table's contents with \a other.
    void swap(SdfPathTable &other) {
        _buckets.swap(other._buckets);
        std::swap(_size, other._size);
        std::swap(_mask, other._mask);
    }

    /// @}

private:

    // Helper to delete entries.
    static void _DeleteEntryChain(void *voidEntry) {
        _Entry *entry = static_cast<_Entry *>(voidEntry);
        while (entry) {
            _Entry *next = entry->next;
            delete entry;
            entry = next;
        }
    }            

    // Helper to get parent paths.
    static SdfPath _GetParentPath(SdfPath const &path) {
        return path.GetParentPath();
    }

    // Helper to insert \a value in the hash table.  Is responsible for growing
    // storage space when necessary.  Does not consider the tree structure.
    _IterBoolPair _InsertInTable(value_type const &value) {
        // If we have no storage at all so far, grow.
        if (_mask == 0)
            _Grow();

        // Find the item, if present.
        _Entry **bucketHead = &(_buckets[_Hash(value.first)]);
        for (_Entry *e = *bucketHead; e; e = e->next)
            if (e->value.first == value.first)
                return _IterBoolPair(iterator(e), false);

        // Not present.  If the table is getting full then grow and re-find the
        // bucket.
        if (_IsTooFull()) {
            _Grow();
            bucketHead = &(_buckets[_Hash(value.first)]);
        }

        TfAutoMallocTag2 tag2("Sdf", "SdfPathTable::_FindOrCreate");
        TfAutoMallocTag tag(__ARCH_PRETTY_FUNCTION__);

        // Create a new item and insert it in the list
        *bucketHead = new _Entry(value, *bucketHead);

        // One more element
        ++_size;

        // Return the new item
        return _IterBoolPair(iterator(*bucketHead), true);
    }

    // Erase \a entry from the hash table.  Does not consider tree structure.
    void _EraseFromTable(_Entry *entry) {
        // Remove from table.
        _Entry **cur = &_buckets[_Hash(entry->value.first)];
        while (*cur != entry)
            cur = &((*cur)->next);

        // Unlink item and destroy it
        --_size;
        _Entry *tmp = *cur;
        *cur = tmp->next;
        delete tmp;
    }

    // Erase all the tree structure descendants of \a entry from the table.
    void _EraseSubtree(_Entry *entry) {
        // Delete descendant nodes, if any.
        if (_Entry * const firstChild = entry->firstChild) {
            _EraseSubtreeAndSiblings(firstChild);
            _EraseFromTable(firstChild);
        }
    }

    // Erase all the tree structure descendants and siblings of \a entry from
    // the table.
    void _EraseSubtreeAndSiblings(_Entry *entry) {
        // Remove subtree.
        _EraseSubtree(entry);

        // And siblings.
        if (_Entry * const nextSibling = entry->GetNextSibling()) {
            _EraseSubtreeAndSiblings(nextSibling);
            _EraseFromTable(nextSibling);
        }
    }

    // Remove \a entry from its parent's list of children in the tree structure
    // alone.  Does not consider the table.
    void _RemoveFromParent(_Entry *entry) {
        if (entry->value.first == SdfPath::AbsoluteRootPath())
            return;

        // Find parent in table.
        iterator parIter = find(_GetParentPath(entry->value.first));

        // Remove this entry from the parent's children.
        parIter._entry->RemoveChild(entry);
    }

    // Grow the table's number of buckets to the next larger size.  Rehashes the
    // elements into the new table, but leaves tree structure untouched.  (The
    // tree structure need not be modified).
    void _Grow() {
        TfAutoMallocTag2 tag2("Sdf", "SdfPathTable::_Grow");
        TfAutoMallocTag tag(__ARCH_PRETTY_FUNCTION__);

        // Allocate a new bucket list of twice the size.  Minimum nonzero number
        // of buckets is 8.
        _mask = std::max(size_t(7), (_mask << 1) + 1);
        _BucketVec newBuckets(_mask + 1);

        // Move items to a new bucket list
        for (size_t i = 0, n = _buckets.size(); i != n; ++i) {
            _Entry *elem = _buckets[i];
            while (elem) {
                // Save pointer to next item
                _Entry *next = elem->next;

                // Get the bucket in the new list
                _Entry *&m = newBuckets[_Hash(elem->value.first)];

                // Insert the item at the head
                elem->next = m;
                m = elem;

                // Next item
                elem = next;
            }
        }

        // Use the new buckets
        _buckets.swap(newBuckets);
    }

    // Return true if the table should be made bigger.
    bool _IsTooFull() const {
        return _size > _buckets.size();
    }

    // Return the bucket index for \a path.
    inline size_t _Hash(SdfPath const &path) const {
        return path.GetHash() & _mask;
    }

private:
    _BucketVec _buckets;
    size_t _size;
    size_t _mask;

};


#endif // SDF_PATHTABLE_H
