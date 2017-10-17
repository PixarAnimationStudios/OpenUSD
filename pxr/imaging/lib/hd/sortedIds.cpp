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
#include "pxr/imaging/hd/sortedIds.h"
#include "pxr/imaging/hd/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// Tweakable value
//
// If the ids are already at least this percent sorted, use
// insert sort rather than a full sort.
//
static const size_t SORTED_PERCENT = 90;

Hd_SortedIds::Hd_SortedIds()
 : _ids()
 , _sortedCount(0)
{

}

Hd_SortedIds::Hd_SortedIds(Hd_SortedIds &&other)
 : _ids(std::move(other._ids))
 , _sortedCount(other._sortedCount)
{
}

const SdfPathVector &
Hd_SortedIds::GetIds()
{
    _Sort();
    return _ids;
}

void
Hd_SortedIds::Insert(const SdfPath &id)
{
    _ids.push_back(id);
}

void
Hd_SortedIds::Remove(const SdfPath &id)
{
    // The first implementation of this deletion code deleted the
    // element in place.  This kept the list sorted, but was a
    // performance issue on unloading a stage as a lot of prims
    // get removed and the shifting of the vector become a bottleneck
    // So instead, we do a more efficient removal (by swapping the
    // element to be removed with the element on the end of the vector).
    // The down side is that the list is now unsorted, so needs to be
    // sorted again (which is deferred).
    //
    // However, this means that the list is now unsorted in mass removal.
    // In order to use the binary search, we need a sorted list, but sorting
    // again would be too expensive in this case, so if the list is not sorted
    // fallback to a linear search for the id to be removed.

    SdfPathVector::iterator idToRemove;
    if (_sortedCount == _ids.size()) {
        // Sorted, so use binary search
        idToRemove = std::lower_bound(_ids.begin(),
                                      _ids.end(),
                                      id);
    } else {
        // Unsorted, so use linear search
        idToRemove = std::find(_ids.begin(),
                               _ids.end(),
                               id);
    }

    if (idToRemove != _ids.end()) {
        if (*idToRemove == id) {
            SdfPathVector::iterator lastElement = _ids.end();
            --lastElement;

            std::iter_swap(idToRemove, lastElement);

            _ids.pop_back();

            // As we've moved an element from the end into the middle
            // the list is now only sorted up to the place where the element
            // was removed.


            _sortedCount = std::min(_sortedCount,
                                    static_cast<size_t>(
                                              (idToRemove - _ids.begin())));
        }
    }
}


HD_API
void Hd_SortedIds::RemoveRange(size_t start, size_t end)
{
    size_t numIds = _ids.size();
    size_t numToRemove = (end - start + 1);

    if (_sortedCount != numIds) {
        TF_CODING_ERROR("RemoveRange can only be called while list sorted\n");
        return;
    }

    if (numToRemove == numIds) {
        Clear();
        return;
    }

    SdfPathVector::iterator itStart = _ids.begin() + start;
    SdfPathVector::iterator itEnd   = _ids.begin() + (end + 1);

    _ids.erase(itStart, itEnd);
    _sortedCount -= numToRemove;
}

void
Hd_SortedIds::Clear()
{
    _ids.clear();
    _sortedCount = 0;
}

void
Hd_SortedIds::_InsertSort()
{
    SdfPathVector::iterator sortPosIt = _ids.begin();
    // skip already sorted items
    sortPosIt += _sortedCount;

    while (sortPosIt != _ids.end()) {
        SdfPathVector::iterator insertPosIt = std::lower_bound(_ids.begin(),
                                                               sortPosIt,
                                                               *sortPosIt);

        std::rotate(insertPosIt, sortPosIt, sortPosIt + 1);
        ++sortPosIt;
    }
}

void
Hd_SortedIds::_FullSort()
{
    std::sort(_ids.begin(), _ids.end());
}

void
Hd_SortedIds::_Sort()
{
    HD_TRACE_FUNCTION();

    size_t numIds = _ids.size();


    if (_sortedCount == numIds) {
        return;
    }

    //   (_sortedCount / numIds) * 100 > SORTED_PERCENT
    if (100 * _sortedCount > SORTED_PERCENT * numIds) {
        _InsertSort();
    } else {
        _FullSort();
    }

    _sortedCount = numIds;
}

PXR_NAMESPACE_CLOSE_SCOPE
