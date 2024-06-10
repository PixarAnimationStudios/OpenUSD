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
/// Note that this class behaves like a multiset.  Duplicate elements are
/// allowed.
///
class Hd_SortedIds {
public:
    /// Sorts the ids if needed and returns the sorted list of ids.
    HD_API
    const SdfPathVector &GetIds();

    /// Add an id to the collection.  If the id is already present in the
    /// collection, a duplicate id is added.
    HD_API
    void Insert(const SdfPath &id);

    /// Remove up to one occurrence of id from the collection.  If the id is not
    /// present, do nothing.  Otherwise remove one copy of id.
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
    void _Sort();

    SdfPathVector                            _ids;
    SdfPathVector                            _updates;
    bool                                     _removing=false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SORTED_IDS_H
