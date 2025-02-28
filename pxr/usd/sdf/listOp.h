//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_LIST_OP_H
#define PXR_USD_SDF_LIST_OP_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/hash.h"

#include <functional>
#include <iosfwd>
#include <list>
#include <map>
#include <optional>
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
/// SdfListOp is a value type representing an operation that edits a list.
/// It may append or prepend items, delete them, or replace the list entirely.
///
/// SdfListOp maintains lists of items to be prepended, appended, deleted, or 
/// used explicitly. If used in explicit mode, the ApplyOperations method replaces the given list
/// with the set explicit items. Otherwise, the ApplyOperations
/// method is used to apply the list-editing options in the input list in the
/// following order:
/// - Delete
/// - Prepend
/// - Append
///
/// Lists are meant to contain unique values, and all list operations
/// will remove duplicates if encountered. Prepending items and using
/// explicit mode will preserve the position of the first of the duplicates 
/// to be encountered, while appending items will preserve the last.

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

    /// Return the item vector identified by \p type.
    SDF_API const ItemVector& GetItems(SdfListOpType type) const;

    /// Returns the effective list of items represented by the operations in
    /// this list op. This function should be used to determine the final list
    /// of items added instead of looking at the individual explicit, prepended,
    /// and appended item lists. 
    ///
    /// This is equivalent to calling ApplyOperations on an empty item vector.
    SDF_API ItemVector GetAppliedItems() const;

    /// Sets the explicit items. If duplicates are present in \p items, 
    /// preserves the first occurence.
    /// Returns true if no duplicates were present, false otherwise. If
    /// duplicates were present, errMsg is set to indicate which item was duplicated.
    SDF_API bool SetExplicitItems(const ItemVector &items, std::string* errMsg = nullptr);

    /// Sets the prepended items. If duplicates are present in \p items, 
    /// preserves the first occurence.
    /// Returns true if no duplicates were present, false otherwise. If
    /// duplicates were present, errMsg is set to indicate which item was duplicated.
    SDF_API bool SetPrependedItems(const ItemVector &items, std::string* errMsg = nullptr);

    /// Sets the appended items. If duplicates are present in \p items, 
    /// preserves the last occurence.
    /// Returns true if no duplicates were present, false otherwise. If
    /// duplicates were present, errMsg is set to indicate which item was duplicated.
    SDF_API bool SetAppendedItems(const ItemVector &items, std::string* errMsg = nullptr);

    /// Sets the deleted items. If duplicates are present in \p items, 
    /// preserves the first occurence.
    /// Returns true if no duplicates were present, false otherwise. If
    /// duplicates were present, errMsg is set to indicate which item was duplicated.
    SDF_API bool SetDeletedItems(const ItemVector &items, std::string* errMsg = nullptr);

    /// Sets the item vector for the given operation \p type.
    /// Removes duplicates in \p items if present.
    /// Returns true if no duplicates were present, false otherwise. If
    /// duplicates were present, errMsg is set to indicate which item was duplicated.
    SDF_API bool SetItems(const ItemVector &items, SdfListOpType type, 
                            std::string* errMsg = nullptr);

    /// Removes all items and changes the list to be non-explicit.
    SDF_API void Clear();

    /// Removes all items and changes the list to be explicit.
    SDF_API void ClearAndMakeExplicit();

    /// Callback type for ApplyOperations.
    typedef std::function<
        std::optional<ItemType>(SdfListOpType, const ItemType&)
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
    std::optional<SdfListOp<T>>
    ApplyOperations(const SdfListOp<T> &inner) const;

    /// Callback type for ModifyOperations.
    typedef std::function<
        std::optional<ItemType>(const ItemType&)
    > ModifyCallback;

    /// Modifies operations specified in this object.
    /// \p callback is called for every item in all operation vectors.  If the 
    /// returned key is empty then the key is removed, otherwise it's replaced 
    /// with the returned key.
    /// 
    /// If \p callback returns a key that was previously returned for the
    /// current operation vector being processed, the returned key will be
    /// removed.
    ///
    /// Returns true if a change was made, false otherwise.
    SDF_API
    bool ModifyOperations(const ModifyCallback& callback);

    /// \deprecated Please use ModifyOperations(const ModifyCallback& callback)
    /// instead.
    SDF_API
    bool ModifyOperations(const ModifyCallback& callback,
                          bool unusedRemoveDuplicates);

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

    /// \deprecated The add and reorder operations have been deprecated in favor 
    /// of the append and prepend operations.
    const ItemVector& GetAddedItems() const
    {
        return _addedItems;
    }

    /// \deprecated The add and reorder operations have been deprecated in favor 
    /// of the append and prepend operations.
    const ItemVector& GetOrderedItems() const
    {
        return _orderedItems;
    }
    /// \deprecated The add and reorder operations have been deprecated in favor 
    /// of the append and prepend operations.
    SDF_API void SetAddedItems(const ItemVector &items);

    /// \deprecated The add and reorder operations have been deprecated in favor 
    /// of the append and prepend operations.
    SDF_API void SetOrderedItems(const ItemVector &items);

    friend inline size_t hash_value(const SdfListOp &op) {
        return TfHash::Combine(
            op._isExplicit,
            op._explicitItems,
            op._addedItems,
            op._prependedItems,
            op._appendedItems,
            op._deletedItems,
            op._orderedItems
        );
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

    void _PrependKeys(const ApplyCallback& cb,
                      _ApplyList* result, _ApplyMap* search) const;
    void _AppendKeys(const ApplyCallback& cb,
                     _ApplyList* result, _ApplyMap* search) const;
    void _DeleteKeys(const ApplyCallback& cb,
                     _ApplyList* result, _ApplyMap* search) const;

    /// \deprecated 
    /// Use _PrependKeys or _AppendKeys instead.
    void _AddKeys(SdfListOpType, const ApplyCallback& cb,
                  _ApplyList* result, _ApplyMap* search) const;

    /// \deprecated 
    /// Use _PrependKeys or _AppendKeys instead.
    void _ReorderKeys(const ApplyCallback& cb,
                      _ApplyList* result, _ApplyMap* search) const;
    static void _ReorderKeysHelper(ItemVector order, const ApplyCallback& cb,
                                   _ApplyList *result, _ApplyMap *search);
    template <class ItemType>
    friend void SdfApplyListOrdering(std::vector<ItemType> *v,
                                    const std::vector<ItemType> &order);
    bool _MakeUnique(std::vector<T>& items, bool reverse=false, 
                    std::string* errMsg = nullptr);

private:
    bool _isExplicit;
    ItemVector _explicitItems;
    ItemVector _addedItems;
    ItemVector _prependedItems;
    ItemVector _appendedItems;
    ItemVector _deletedItems;
    ItemVector _orderedItems;
};

// ADL swap.
template <class T>
void swap(SdfListOp<T> &x, SdfListOp<T> &y)
{
    x.Swap(y);
}

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
