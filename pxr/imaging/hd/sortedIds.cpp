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

static const ptrdiff_t INVALID_DELETE_POINT = -1;

PXR_NAMESPACE_OPEN_SCOPE

//
// Tweakable value
//
// If the count of unsorted ids is this value or lower, use insert
// sort rather than a full sort.
//
static const size_t INSERTION_SORT_MAX_ENTRIES = 32;

Hd_SortedIds::Hd_SortedIds()
 : _ids()
 , _idIndices()
 , _sortedCount(0)
 , _afterLastDeletePoint(INVALID_DELETE_POINT)
{

}

Hd_SortedIds::Hd_SortedIds(Hd_SortedIds &&other)
 : _ids(std::move(other._ids))
 , _idIndices(std::move(other._idIndices))
 , _sortedCount(other._sortedCount)
 , _afterLastDeletePoint(other._afterLastDeletePoint)
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
    _idIndices[id] = _ids.size() - 1;
    _afterLastDeletePoint = INVALID_DELETE_POINT;
}

void
Hd_SortedIds::Remove(const SdfPath &id)
{
    // For better removal, we use an extra index list to find the
    // position of the id in the list. This is a trade-off between
    // former implementation which can be partially sorted on demand.

    // Search in the index list for fast access.
    auto idIndexIter = _idIndices.find(id);
    if (idIndexIter != _idIndices.end()) {
        if (idIndexIter->first == id) {
            SdfPathVector::iterator idToRemove = _ids.begin() + idIndexIter->second;
            SdfPathVector::iterator lastElement = --_ids.end();
            
            if (idToRemove != lastElement) {
                // Update index
                _idIndices[*lastElement] = idIndexIter->second;
                _idIndices.erase(id);
                
                std::iter_swap(idToRemove, lastElement);

                if (std::distance(idToRemove, lastElement) == 1) {
                    // idToRemove points to the last element after pop_back()
                    _afterLastDeletePoint = INVALID_DELETE_POINT;
                } else {
                    _afterLastDeletePoint = idToRemove - _ids.begin();
                    ++_afterLastDeletePoint;
                }
                _ids.pop_back();

                // As we've moved an element from the end into the middle
                // the list is now only sorted up to the place where the element
                // was removed.

                _sortedCount = std::min(_sortedCount,
                                        static_cast<size_t>(
                                                  (idToRemove - _ids.begin())));
            } else {
                _idIndices.erase(id);
                _ids.pop_back();
                _afterLastDeletePoint = INVALID_DELETE_POINT;

                // As we've removed an element from the end of the list
                // the list remains in the same sort state, so
                // trim the length if necessary.
                // Note: Can't use the idToRemove iterator as that has
                // been invalidated.
                _sortedCount = std::min(_sortedCount, _ids.size());
            }
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

    for(size_t index = start; index != end; index++)
        _idIndices.erase(_ids[index]);
    
    SdfPathVector::iterator itStart = _ids.begin() + start;
    SdfPathVector::iterator itEnd   = _ids.begin() + (end + 1);
    _ids.erase(itStart, itEnd);

    _sortedCount -= numToRemove;
    _afterLastDeletePoint = INVALID_DELETE_POINT;
}

void
Hd_SortedIds::Clear()
{
    _ids.clear();
    _idIndices.clear();
    _sortedCount = 0;
    _afterLastDeletePoint = INVALID_DELETE_POINT;
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
    
    // Update indices.
    size_t indexValue = 0;
    for(auto const& id : _ids)
        _idIndices[id] = indexValue++;
}

void
Hd_SortedIds::_FullSort()
{
    // Sort the unsorted part
    SdfPathVector::iterator mid = _ids.begin() + _sortedCount;
    std::sort(mid, _ids.end());

    // If needed, merge
    if (mid == _ids.begin() || *(mid-1) < *(mid)) {
        // List is fully sorted.
        
        // Update remaining indices
        for(size_t indexValue = _sortedCount; indexValue < _ids.size(); ++indexValue)
            _idIndices[_ids[indexValue]] = indexValue;
    } else {
        std::inplace_merge(_ids.begin(), mid, _ids.end());
        
        // Update all indices
        size_t indexValue = 0;
        for(auto const& id : _ids)
            _idIndices[id] = indexValue++;
    }
}

void
Hd_SortedIds::_Sort()
{
    HD_TRACE_FUNCTION();

    size_t numIds = _ids.size();


    if (_sortedCount == numIds) {
        return;
    }

    if (numIds - _sortedCount < INSERTION_SORT_MAX_ENTRIES) {
        _InsertSort();
    } else {
        _FullSort();
    }

    _sortedCount = numIds;
    _afterLastDeletePoint = INVALID_DELETE_POINT;
}

PXR_NAMESPACE_CLOSE_SCOPE
