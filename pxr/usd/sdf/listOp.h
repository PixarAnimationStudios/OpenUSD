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
#ifndef PXR_USD_SDF_LIST_OP_H
#define PXR_USD_SDF_LIST_OP_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/token.h"

#include <boost/functional/hash.hpp>
#include <boost/optional/optional_fwd.hpp>

#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \enum SdfListOpType
///
/// Enum for specifying one of the list editing operation types.
///
enum SdfListOpType {
    SdfListOpTypeExplicit,
    SdfListOpTypeAdded,
    SdfListOpTypeDeleted,
    SdfListOpTypeOrdered,
    SdfListOpTypePrepended,
    SdfListOpTypeAppended
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

    /// Create a ListOp in explicit mode with the given \p explicitItems.
    SDF_API
    static SdfListOp CreateExplicit(
        const ItemVector& explicitItems = ItemVector());

    /// Create a ListOp in non-explicit mode with the given 
    /// \p prependedItems, \p appendedItems, and \p deletedItems
    SDF_API
    static SdfListOp Create(
        const ItemVector& prependedItems = ItemVector(),
        const ItemVector& appendedItems = ItemVector(),
        const ItemVector& deletedItems = ItemVector());

    /// Create an empty ListOp in non-explicit mode.
    SDF_API SdfListOp();

    SDF_API void Swap(SdfListOp<T>& rhs);

    /// Returns \c true if the editor has an explicit list (even if it's
    /// empty) or it has any added, prepended, appended, deleted,
    /// or ordered keys.
    bool HasKeys() const
    {
        if (IsExplicit()) {
            return true;
        }
        if (_addedItems.size() != 0 ||
            _prependedItems.size() != 0 ||
            _appendedItems.size() != 0 ||
            _deletedItems.size() != 0) {
            return true;
        }
        return _orderedItems.size() != 0;
    }

    /// Returns \c true if the given item is in any of the item lists.
    SDF_API bool HasItem(const T& item) const;

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

    /// Returns the explicit items.
    const ItemVector& GetPrependedItems() const
    {
        return _prependedItems;
    }

    /// Returns the explicit items.
    const ItemVector& GetAppendedItems() const
    {
        return _appendedItems;
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
    SDF_API const ItemVector& GetItems(SdfListOpType type) const;

    SDF_API void SetExplicitItems(const ItemVector &items);
    SDF_API void SetAddedItems(const ItemVector &items);
    SDF_API void SetPrependedItems(const ItemVector &items);
    SDF_API void SetAppendedItems(const ItemVector &items);
    SDF_API void SetDeletedItems(const ItemVector &items);
    SDF_API void SetOrderedItems(const ItemVector &items);

    /// Sets the item vector for the given operation \p type.
    SDF_API void SetItems(const ItemVector &items, SdfListOpType type);

    /// Removes all items and changes the list to be non-explicit.
    SDF_API void Clear();

    /// Removes all items and changes the list to be explicit.
    SDF_API void ClearAndMakeExplicit();

    /// Callback type for ApplyOperations.
    typedef std::function<
        boost::optional<ItemType>(SdfListOpType, const ItemType&)
    > ApplyCallback;

    /// Applies edit operations to the given ItemVector.
    /// If supplied, \p cb will be called on each item in the operation vectors
    /// before they are applied to \p vec. Consumers can use this to transform
    /// the items stored in the operation vectors to match what's stored in
    /// \p vec.
    SDF_API 
    void ApplyOperations(ItemVector* vec, 
                         const ApplyCallback& cb = ApplyCallback()) const;

    /// Applies edit operations to the given ListOp.
    ///
    /// The result is a ListOp that, when applied to a list, has the same
    /// effect as applying \p inner and then \p this in sequence.
    ///
    /// The result will be empty if the result is not well defined.
    /// The result is well-defined when \p inner and \p this do not
    /// use the 'ordered' or 'added' item lists.  In other words, only
    /// the explicit, prepended, appended, and deleted portions of
    /// SdfListOp are closed under composition with ApplyOperations().
    SDF_API 
    boost::optional<SdfListOp<T>>
    ApplyOperations(const SdfListOp<T> &inner) const;

    /// Callback type for ModifyOperations.
    typedef std::function<
        boost::optional<ItemType>(const ItemType&)
    > ModifyCallback;

    /// Modifies operations specified in this object.
    /// \p callback is called for every item in all operation vectors.  If the 
    /// returned key is empty then the key is removed, otherwise it's replaced 
    /// with the returned key.
    ///
    /// Returns true if a change was made, false otherwise.
    SDF_API bool ModifyOperations(const ModifyCallback& callback);

    /// Replaces the items in the specified operation vector in the range
    /// (index, index + n] with the given \p newItems. If \p newItems is empty
    /// the items in the range will simply be removed.
    SDF_API 
    bool ReplaceOperations(const SdfListOpType op, size_t index, size_t n, 
                           const ItemVector& newItems);

    /// Composes a stronger SdfListOp's opinions for a given operation list
    /// over this one.
    SDF_API 
    void ComposeOperations(const SdfListOp<T>& stronger, SdfListOpType op);

    friend inline size_t hash_value(const SdfListOp &op) {
        size_t h = 0;
        boost::hash_combine(h, op._isExplicit);
        boost::hash_combine(h, op._explicitItems);
        boost::hash_combine(h, op._addedItems);
        boost::hash_combine(h, op._prependedItems);
        boost::hash_combine(h, op._appendedItems);
        boost::hash_combine(h, op._deletedItems);
        boost::hash_combine(h, op._orderedItems);
        return h;
    }

    bool operator==(const SdfListOp<T> &rhs) const {
        return _isExplicit == rhs._isExplicit &&
               _explicitItems == rhs._explicitItems &&
               _addedItems == rhs._addedItems &&
               _prependedItems == rhs._prependedItems &&
               _appendedItems == rhs._appendedItems &&
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

    void _AddKeys(SdfListOpType, const ApplyCallback& cb,
                  _ApplyList* result, _ApplyMap* search) const;
    void _PrependKeys(SdfListOpType, const ApplyCallback& cb,
                      _ApplyList* result, _ApplyMap* search) const;
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
    ItemVector _prependedItems;
    ItemVector _appendedItems;
    ItemVector _deletedItems;
    ItemVector _orderedItems;
};

// Helper function for applying an ordering operation described by \p orderVector
// to vector \p v.
template <class ItemType>
SDF_API
void SdfApplyListOrdering(std::vector<ItemType>* v, 
                          const std::vector<ItemType>& order);

// Ostream output methods for list values (useful for debugging and required
// for storing a list value in a VtValue).
template <typename T>
SDF_API
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
typedef class SdfListOp<class SdfPayload> SdfPayloadListOp;
typedef class SdfListOp<class SdfUnregisteredValue> SdfUnregisteredValueListOp;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_LIST_OP_H
