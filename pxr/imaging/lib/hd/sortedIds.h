//
// Copyright 2017 Pixar
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
#ifndef HD_SORTED_IDS_H
#define HD_SORTED_IDS_H

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

#endif // HD_SORTED_IDS_H
