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
#ifndef SDF_LIST_OP_H
#define SDF_LIST_OP_H

#include "pxr/base/tf/token.h"

#include <boost/function.hpp>
#include <boost/functional/hash.hpp>
#include <boost/optional.hpp>

#include <iosfwd>
#include <list>
#include <map>
#include <string>
#include <vector>

/// \enum SdfListOpType
///
/// Enum for specifying one of the list editing operation types.
///
enum SdfListOpType {
    SdfListOpTypeExplicit,
    SdfListOpTypeAdded,
    SdfListOpTypeDeleted,
    SdfListOpTypeOrdered
};

/// \struct Sdf_ListOpTraits
///
/// Trait classes for specializing behaviors of SdfListOp for a given item
/// type.
///
template <class T>
struct Sdf_ListOpTraits
{
    typedef std::less<T> ItemComparator;
};

/// \class SdfListOp
///
/// Value type representing a list-edit operation.
///
/// SdfListOp is a value type representing an operation that edits a list.
/// It may add or remove items, reorder them, or replace the list entirely.
///
template <typename T>
class SdfListOp {
public:
    typedef T ItemType;
    typedef std::vector<ItemType> ItemVector;
    typedef ItemType value_type;
    typedef ItemVector value_vector_type;

    SdfListOp();

    void Swap(SdfListOp<T>& rhs);

    /// Returns \c true if the editor has an explicit list (even if it's
    /// empty) or it has any added, deleted, or ordered keys.
    bool HasKeys() const
    {
        if (IsExplicit()) {
            return true;
        }
        if (_addedItems.size() != 0 or
            _deletedItems.size() != 0) {
            return true;
        }
        return _orderedItems.size() != 0;
    }

    /// Returns \c true if the list is explicit.
    bool IsExplicit() const
    {
        return _isExplicit;
    }

    /// Returns the explicit items.
    const ItemVector& GetExplicitItems() const
    {
        return _explicitItems;
    }

    /// Returns the explicit items.
    const ItemVector& GetAddedItems() const
    {
        return _addedItems;
    }

    /// Returns the deleted items.
    const ItemVector& GetDeletedItems() const
    {
        return _deletedItems;
    }

    /// Returns the ordered items.
    const ItemVector& GetOrderedItems() const
    {
        return _orderedItems;
    }

    /// Return the item vector identified by \p type.
    const ItemVector& GetItems(SdfListOpType type) const;

    void SetExplicitItems(const ItemVector &items);
    void SetAddedItems(const ItemVector &items);
    void SetDeletedItems(const ItemVector &items);
    void SetOrderedItems(const ItemVector &items);

    /// Sets the item vector for the given operation \p type.
    void SetItems(const ItemVector &items, SdfListOpType type);

    /// Removes all items and changes the list to be non-explicit.
    void Clear();

    /// Removes all items and changes the list to be explicit.
    void ClearAndMakeExplicit();

    /// Callback type for ApplyOperations.
    typedef boost::function<
        boost::optional<ItemType>(SdfListOpType, const ItemType&)
    > ApplyCallback;

    /// Applies edit operations to the given ItemVector.
    /// If supplied, \p cb will be called on each item in the operation vectors
    /// before they are applied to \p vec. Consumers can use this to transform
    /// the items stored in the operation vectors to match what's stored in
    /// \p vec.
    void ApplyOperations(ItemVector* vec, 
                         const ApplyCallback& cb = ApplyCallback()) const;

    /// Callback type for ModifyOperations.
    typedef boost::function<
        boost::optional<ItemType>(const ItemType&)
    > ModifyCallback;

    /// Modifies operations specified in this object.
    /// \p callback is called for every item in all operation vectors.  If the 
    /// returned key is empty then the key is removed, otherwise it's replaced 
    /// with the returned key.
    ///
    /// Returns true if a change was made, false otherwise.
    bool ModifyOperations(const ModifyCallback& callback);

    /// Replaces the items in the specified operation vector in the range
    /// (index, index + n] with the given \p newItems. If \p newItems is empty
    /// the items in the range will simply be removed.
    bool ReplaceOperations(const SdfListOpType op, size_t index, size_t n, 
                           const ItemVector& newItems);

    /// Composes a stronger SdfListOp's opinions for a given operation list
    /// over this one.
    void ComposeOperations(const SdfListOp<T>& stronger, SdfListOpType op);

    friend inline size_t hash_value(const SdfListOp &op) {
        size_t h = 0;
        boost::hash_combine(h, op._isExplicit);
        boost::hash_combine(h, op._explicitItems);
        boost::hash_combine(h, op._addedItems);
        boost::hash_combine(h, op._deletedItems);
        boost::hash_combine(h, op._orderedItems);
        return h;
    }

    bool operator==(const SdfListOp<T> &rhs) const {
        return _isExplicit == rhs._isExplicit &&
               _explicitItems == rhs._explicitItems &&
               _addedItems == rhs._addedItems &&
               _deletedItems == rhs._deletedItems &&
               _orderedItems == rhs._orderedItems;
    };

    bool operator!=(const SdfListOp<T> &rhs) const {
        return !(*this == rhs);
    };

private:
    void _SetExplicit(bool isExplicit);

    typedef typename Sdf_ListOpTraits<T>::ItemComparator _ItemComparator;
    typedef std::list<ItemType> _ApplyList;
    typedef std::map<ItemType, typename _ApplyList::iterator, _ItemComparator>
        _ApplyMap;

    void _AppendKeys(SdfListOpType, const ApplyCallback& cb,
                     _ApplyList* result, _ApplyMap* search) const;
    void _DeleteKeys(SdfListOpType, const ApplyCallback& cb,
                     _ApplyList* result, _ApplyMap* search) const;
    void _ReorderKeys(SdfListOpType, const ApplyCallback& cb,
                      _ApplyList* result, _ApplyMap* search) const;

private:
    bool _isExplicit;
    ItemVector _explicitItems;
    ItemVector _addedItems;
    ItemVector _deletedItems;
    ItemVector _orderedItems;
};

// Helper function for applying an ordering operation described by \p orderVector
// to vector \p v.
template <class ItemType>
void SdfApplyListOrdering(std::vector<ItemType>* v, 
                          const std::vector<ItemType>& order);

// Ostream output methods for list values (useful for debugging and required
// for storing a list value in a VtValue).
template <typename T>
std::ostream & operator<<( std::ostream &, const SdfListOp<T> & );

// Concrete, instantiated listop types.
typedef class SdfListOp<int> SdfIntListOp;
typedef class SdfListOp<unsigned int> SdfUIntListOp;
typedef class SdfListOp<int64_t> SdfInt64ListOp;
typedef class SdfListOp<uint64_t> SdfUInt64ListOp;
typedef class SdfListOp<TfToken> SdfTokenListOp;
typedef class SdfListOp<std::string> SdfStringListOp;
typedef class SdfListOp<class SdfPath> SdfPathListOp;
typedef class SdfListOp<class SdfReference> SdfReferenceListOp;
typedef class SdfListOp<class SdfUnregisteredValue> SdfUnregisteredValueListOp;

#endif // SDF_LIST_OP_H
