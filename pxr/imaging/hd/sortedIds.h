//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_SORTED_IDS_H
#define PXR_IMAGING_HD_SORTED_IDS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// Manages a container of Hydra Ids in a sorted order.
///
/// For performance reasons, sorting of the list is deferred
/// due to inserting a large number of items at once.
///
/// The class chooses the type of sort based on how many unsorted items
/// there are in the list.
///
class Hd_SortedIds {
public:
    HD_API
    Hd_SortedIds();

    HD_API
    ~Hd_SortedIds() = default;

    HD_API
    Hd_SortedIds(Hd_SortedIds &&other);

    /// Sorts the ids if needed and returns the sorted list of ids.
    HD_API
    const SdfPathVector &GetIds();

    /// Add a new id to the collection
    HD_API
    void Insert(const SdfPath &id);

    /// Remove an id from the collection.
    HD_API
    void Remove(const SdfPath &id);

    /// Remove a range of id from the collection.
    /// Range defined by position index in sorted list.
    /// end is inclusive.
    HD_API
    void RemoveRange(size_t start, size_t end);

    /// Removes all ids from the collection.
    HD_API
    void Clear();

private:
    SdfPathVector           _ids;
    size_t                  _sortedCount;
    ptrdiff_t               _afterLastDeletePoint;

    void _InsertSort();
    void _FullSort();
    void _Sort();

    Hd_SortedIds(const Hd_SortedIds &) = delete;
    Hd_SortedIds &operator =(const Hd_SortedIds &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SORTED_IDS_H
