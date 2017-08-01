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

Hd_SortedIds::Hd_SortedIds(const Hd_SortedIds &&other)
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
    _Sort();

    SdfPathVector::iterator it = std::lower_bound(_ids.begin(),
                                                  _ids.end(),
                                                  id);
    if (it != _ids.end()) {
        if (*it == id) {
            _ids.erase(it);
        }
    }

    // Update size
    _sortedCount = _ids.size();
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
