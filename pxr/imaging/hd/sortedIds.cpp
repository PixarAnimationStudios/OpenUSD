//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/sortedIds.h"
#include "pxr/imaging/hd/perfLog.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

const SdfPathVector &
Hd_SortedIds::GetIds()
{
    _Sort();
    return _ids;
}

void
Hd_SortedIds::Insert(const SdfPath &id)
{
    if (_removing) {
        _Sort();
        _removing = false;
    }
    _updates.push_back(id);
}

void
Hd_SortedIds::Remove(const SdfPath &id)
{
    if (!_removing) {
        _Sort();
        _removing = true;
    }
    _updates.push_back(id);
}

HD_API
void Hd_SortedIds::RemoveRange(size_t start, size_t end)
{
    size_t numIds = _ids.size();
    size_t numToRemove = (end - start + 1);

    if (!_updates.empty()) {
        TF_CODING_ERROR("RemoveRange can only be called while list sorted");
        return;
    }

    if (numToRemove == numIds) {
        Clear();
        return;
    }

    SdfPathVector::iterator itStart = _ids.begin() + start;
    SdfPathVector::iterator itEnd   = _ids.begin() + (end + 1);

    _ids.erase(itStart, itEnd);
}

void
Hd_SortedIds::Clear()
{
    _ids.clear();
    _updates.clear();
    _removing = false;
}

// The C++ standard requires that the output cannot overlap with either range.
// This implementation allows the output range to start at the first input
// range.
template <class Iter1, class Iter2, class OutIter>
static inline OutIter
_SetDifference(Iter1 f1, Iter1 l1, Iter2 f2, Iter2 l2, OutIter out)
{
    while (f1 != l1 && f2 != l2) {
        if (*f1 < *f2) {
            *out = *f1;
            ++out, ++f1;
        }
        else if (*f2 < *f1) {
            ++f2;
        }
        else {
            ++f1, ++f2;
        }
    }
    return std::copy(f1, l1, out);
}

void
Hd_SortedIds::_Sort()
{
    if (_updates.empty()) {
        return;
    }

    HD_TRACE_FUNCTION();

    // The most important thing to do here performance-wise is to minimize the
    // number of lexicographical SdfPath less-than operations that we do on
    // paths that are not equal.

    // Sort the updates.
    std::sort(_updates.begin(), _updates.end());

    // Important case: adding new ids, _ids is currently empty.
    if (!_removing && _ids.empty()) {
        swap(_ids, _updates);
        return;
    }

    if (_removing) {
        // Find the range in _ids that we will remove from.
        auto removeBegin =
            lower_bound(_ids.begin(), _ids.end(), _updates.front());

        if (_updates.size() == 1) {
            // For a single remove, we can just erase it if present.
            if (removeBegin != _ids.end() && *removeBegin == _updates.front()) {
                _ids.erase(removeBegin);
            }
        }
        else {
            auto removeEnd =
                upper_bound(_ids.begin(), _ids.end(), _updates.back());

            // If the number of elements we're removing is small compared to the
            // size of the range, then do individual binary search & erase
            // rather than a set-difference over the range.
            //
            // Empirical testing suggests the break-event point is with very low
            // density -- about one removal per 2800 elements to search.  Note
            // that the best value for this constant could depend quite a lot on
            // the performance characterisitcs of the hardware.  The test
            // "testHdSortedIdsPerf" can be useful to help find an optimal value
            // for this.
            //
            // One reason it's such a low density, even though set-difference
            // needs to do operator< on every path, is that almost all of those
            // operator< will be performed on the same two paths, and that
            // special case is really fast.
            static constexpr size_t BinarySearchFrac = 2800;
            const size_t removeRangeSize = distance(removeBegin, removeEnd);
            if (removeRangeSize / BinarySearchFrac > _updates.size()) {
                // Binary search & erase each _updates element.
                auto updateIter = _updates.begin();
                const auto updateEnd = _updates.end();
                while (removeBegin != removeEnd && updateIter != updateEnd) {
                    if (*removeBegin == *updateIter) {
                        --removeEnd;
                        removeBegin = _ids.erase(removeBegin);
                    }
                    removeBegin = ++updateIter != updateEnd
                        ? lower_bound(removeBegin, removeEnd, *updateIter)
                        : removeEnd;
                }
            }
            else {
                // Take the difference.
                auto newRemoveEnd = _SetDifference(
                    make_move_iterator(removeBegin),
                    make_move_iterator(removeEnd),
                    make_move_iterator(_updates.begin()),
                    make_move_iterator(_updates.end()),
                    removeBegin);

                // Shift remaining backward.
                const auto dst = std::move(removeEnd, _ids.end(), newRemoveEnd);
                _ids.erase(dst, _ids.end());
            }
        }
    }
    else {
        // Find the range in _ids that we will add to.
        auto addBegin = lower_bound(_ids.begin(), _ids.end(), _updates.front());

        if (_updates.size() == 1) {
            // For a single add, we just insert it (even if present).
            _ids.insert(addBegin, std::move(_updates.front()));
        }
        else {
            auto addEnd =
                upper_bound(_ids.begin(), _ids.end(), _updates.back());

            if (std::distance(addBegin, addEnd) == 0) {
                // We're inserting into an empty range in _ids.
                _ids.insert(addBegin,
                            std::make_move_iterator(_updates.begin()),
                            std::make_move_iterator(_updates.end()));
            }
            else {
                // Create tmp space to write the union to, then do the union.
                SdfPathVector tmp;
                tmp.reserve(std::distance(addBegin, addEnd) + _updates.size());
                // We explicitly use merge rather than set_union here to
                // preserve the semantics that inserting duplicates always
                // succeeds.
                std::merge(make_move_iterator(addBegin),
                           make_move_iterator(addEnd),
                           make_move_iterator(_updates.begin()),
                           make_move_iterator(_updates.end()),
                           back_inserter(tmp));
                
                size_t numAdded = tmp.size() - std::distance(addBegin, addEnd);

                // Resize _ids and refetch possibly invalidated iterators.
                const size_t addBeginIdx = distance(_ids.begin(), addBegin);
                const size_t addEndIdx = distance(_ids.begin(), addEnd);
                _ids.resize(_ids.size() + numAdded);
                addBegin = _ids.begin() + addBeginIdx;
                addEnd = _ids.begin() + addEndIdx;

                // Shift remaining items to end.
                std::move_backward(addEnd, _ids.end()-numAdded, _ids.end());
                
                // Move the union to its final place in _ids.
                std::move(tmp.begin(), tmp.end(), addBegin);
            }
        }
    }
    
    _updates.clear();
    _removing = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
