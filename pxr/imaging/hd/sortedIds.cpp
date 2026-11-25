//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/sortedIds.h"
#include "pxr/imaging/hd/perfLog.h"

#include <algorithm>
#include <bitset>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// A simple bloom filter to help determine if we need to search the removes list
// for insertions or vice-versa during mixed remove/insert updates.
class _PathBloomFilter
{
    // With 65536 bits in the bloom filter, 2 hash functions, at capacity 1024
    // the expected probability of a false positive is 1/1056.
    static constexpr size_t _LgNumBits = 16;
    static constexpr size_t _NumBits = size_t(1) << _LgNumBits;
    static constexpr size_t _Capacity = 1024;

public:

    // Insert an item.
    void Insert(SdfPath const &p) {
        const uint64_t hash = p.GetHash();
        _bits.set(_Index1(hash));
        _bits.set(_Index2(hash));
        ++_size;
    }

    // Return true if there's a possibility that the filter contains `p`,
    // false if it is definitely not present.
    bool MightContain(SdfPath const &p) const {
        const uint64_t hash = p.GetHash();
        return _bits.test(_Index1(hash)) && _bits.test(_Index2(hash));
    }

    // Return true if the number of Insert() calls is at or over capacity.
    bool IsFull() const {
        return _size >= _Capacity;
    }

private:
    // The two hash functions just take the lowest 2 chunks of LgNumBits.
    uint32_t _Index1(uint64_t hash) const {
        return hash & (_NumBits-1);
    }
    uint32_t _Index2(uint64_t hash) const {
        return (hash >> _LgNumBits) & (_NumBits-1);
    }

    size_t _size = 0;
    std::bitset<_NumBits> _bits;
};

// Return the first element in [first, last) that is not less than `value`, like
// std::lower_bound.  Probe at exponentially increasing distances from `first`
// to bracket the result, then binary search within the bracket.  This takes
// O(log d) comparisons where d is the distance to the result, and compares
// nearby elements first.
template <class Iter, class T>
Iter
_GallopLowerBound(Iter first, Iter last, T const &value)
{
    const size_t avail = std::distance(first, last);
    Iter lo = first;
    size_t step = 1;
    while (step <= avail && *(first + (step - 1)) < value) {
        lo = first + step;
        step *= 2;
    }
    return std::lower_bound(lo, first + std::min(step - 1, avail), value);
}

} // anon

// The purpose of this helper is to ensure correct semantics for queued mixed
// inserts & removals.  The problem is that order matters.  For example, if the
// existing list is [], then the sequence Remove('A'), Insert('A') should
// produce ['A'], while the sequence Insert('A'), Remove('A') should produce [].
// However, if the queued inserts & removals are disjoint with no elements in
// common, then they can be performed in either order.
//
// In practice, callers rarely if ever call Insert() and Remove() with common
// elements.  The typical case is a "rename" operation where the old names are
// removed and the new names are inserted.  So we use a bloom filter to quickly
// check if a path is in the other list when we are asked to Insert() or
// Remove().  If it is definitely not present, we can just push_back.  Otherwise
// we linear-search the other list to see if it truly is present.  If it is, we
// reject the operation, and the caller will _Sort() the ids and retry from
// there.
struct Hd_SortedIds::_UpdateImpl
{
    // Try to arrange to batch-insert `id`. If there's a preexising remove for
    // `id` or if we're over capacity, we reject and return false.  In that case
    // the caller has to _Sort() and retry the insert.
    bool Insert(SdfPathVector *insertsPtr, SdfPath const &id) {
        SdfPathVector &inserts = *insertsPtr;
        // If we're at capacity for inserts, or we have an existing remove for
        // this id, we can't take the insert, caller must _Sort.
        if (_insertsBloom.IsFull() ||
            (_removesBloom.MightContain(id) && _Find(_removes, id))) {
            return false;
        }
        // Otherwise append it to inserts & insert in its bloom filter.
        inserts.push_back(id);
        _insertsBloom.Insert(id);
        return true;
    }

    // Try to arrange to batch-remove `id`. If there's a preexising insert for
    // `id` or if we're over capacity, we reject and return false.  In that case
    // the caller has to _Sort() and retry the remove.
    bool Remove(SdfPathVector *insertsPtr, SdfPath const &id) {
        SdfPathVector &inserts = *insertsPtr;
        // If we're at capacity for removes, or we have an existing insert for
        // this id, we can't take the remove, caller must _Sort.
        if (_removesBloom.IsFull() ||
            (_insertsBloom.MightContain(id) && _Find(inserts, id))) {
            return false;
        }
        // Otherwise append it to _removes & insert in its bloom filter.
        _removes.push_back(id);
        _removesBloom.Insert(id);
        return true;
    }

    // Return _moves by std::move(), leave it indeterminate.
    SdfPathVector TakeRemoves() {
        return std::move(_removes);
    }

private:
    bool _Find(SdfPathVector &ids, SdfPath const &id) const {
        // Just linear search.  We hit this very rarely unless we have Insert()
        // & Remove() calls with the same ids.
        return std::find(ids.begin(), ids.end(), id) != ids.end();
    }
    
    SdfPathVector _removes;
    // The _inserts is stored as the Hd_SortedIds::_edits, and passed to
    // Insert() and Remove() functions
    _PathBloomFilter _insertsBloom;
    _PathBloomFilter _removesBloom;
};

Hd_SortedIds::Hd_SortedIds() = default;

Hd_SortedIds::~Hd_SortedIds() = default;

Hd_SortedIds::Hd_SortedIds(Hd_SortedIds const &other)
    : _ids(other._ids)
    , _edits(other._edits)
    , _mode(other._mode)
    , _updater(other._updater ? new _UpdateImpl { *other._updater } : nullptr)
{
}

HD_API
Hd_SortedIds::Hd_SortedIds(Hd_SortedIds &&) = default;

Hd_SortedIds &
Hd_SortedIds::operator=(Hd_SortedIds const &other)
{
    _ids = other._ids;
    _edits = other._edits;
    _mode = other._mode;
    _updater.reset(
        other._updater ? new _UpdateImpl { *other._updater } : nullptr);
    return *this;
}

HD_API
Hd_SortedIds &Hd_SortedIds::operator=(Hd_SortedIds &&) = default;

const SdfPathVector &
Hd_SortedIds::GetIds()
{
    _Sort();
    return _ids;
}

void
Hd_SortedIds::Insert(const SdfPath &id)
{
    if (_mode == _NoMode) {
        _mode = _InsertMode;
    }
    if (_mode == _InsertMode) {
        _edits.push_back(id);
        return;
    }
    if (_mode != _UpdateMode) {
        _Sort();
        _mode = _UpdateMode;
        _updater.reset(new _UpdateImpl);
    }
    if (!_updater->Insert(&_edits, id)) {
        // If the updater can't take this insert, we _Sort() and retry.
        _Sort();
        Insert(id);
    }
}

void
Hd_SortedIds::Remove(const SdfPath &id)
{
    if (_mode == _NoMode) {
        _mode = _RemoveMode;
    }
    if (_mode == _RemoveMode) {
        _edits.push_back(id);
        return;
    }
    if (_mode != _UpdateMode) {
        _Sort();
        _mode = _UpdateMode;
        _updater.reset(new _UpdateImpl);
    }
    if (!_updater->Remove(&_edits, id)) {
        // If the updater can't take this remove, we _Sort() and retry.
        _Sort();
        Remove(id);
    }
}

void Hd_SortedIds::RemoveRange(size_t start, size_t end)
{
    size_t numIds = _ids.size();
    size_t numToRemove = (end - start + 1);

    if (_mode != _NoMode) {
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
    _edits.clear();
    _mode = _NoMode;
    _updater.reset();
}

void
Hd_SortedIds::_Sort()
{
    // The most important thing to do here performance-wise is to minimize the
    // number of lexicographical SdfPath less-than operations that we do on
    // paths that are not equal.

    if (_mode != _UpdateMode && _edits.empty()) {
        _mode = _NoMode;
        return;
    }

    HD_TRACE_FUNCTION();

    // To handle Update()s, extract the removals and insertions into separate
    // lists and treat as a _RemoveMode batch followed by an _InsertMode batch.
    // This works correctly wrt ordering, since the _updater has ensured that
    // the sets of removes and inserts are disjoint.
    if (_mode == _UpdateMode) {
        // Inserts are in _edits, Removes are in _updater->_removes

        // Move inserts aside for a moment.
        SdfPathVector inserts = std::move(_edits);

        // Move _removes into _edits, set _RemoveMode, then _Sort().
        _edits = _updater->TakeRemoves();
        _mode = _RemoveMode;
        _Sort();

        // Now move inserts to _edits, set _InsertMode, then _Sort().
        _edits = std::move(inserts);
        _mode = _InsertMode;
        _Sort();

        // Done.
        _updater.reset();
        return;
    }

    // Here we're either in _InsertMode or _RemoveMode.  Sort the updates.
    std::sort(_edits.begin(), _edits.end());

    bool removing = (_mode == _RemoveMode);
    
    // Important case: adding new ids, _ids is currently empty.
    if (!removing && _ids.empty()) {
        swap(_ids, _edits);
        _mode = _NoMode;
        return;
    }

    if (removing) {
        // Find the range in _ids that we will remove from.
        auto removeBegin =
            lower_bound(_ids.begin(), _ids.end(), _edits.front());

        if (_edits.size() == 1) {
            // For a single remove, we can just erase it if present.
            if (removeBegin != _ids.end() && *removeBegin == _edits.front()) {
                _ids.erase(removeBegin);
            }
        }
        else {
            const auto removeEnd =
                upper_bound(removeBegin, _ids.end(), _edits.back());

            // Compact _ids in a single pass.  As each element to remove is
            // found, shift the kept span preceding it down over the elements
            // removed so far.  Elements before the first removal do not move.
            // Galloping search finds each element in O(log gap) comparisons,
            // so this adapts to both sparse and dense removals.
            auto out = _ids.end(); // Write position, end() until first removal.
            auto keep = removeBegin; // Start of the pending kept span.
            auto cur = removeBegin;
            for (SdfPath const &edit: _edits) {
                cur = _GallopLowerBound(cur, removeEnd, edit);
                if (cur == removeEnd) {
                    break;
                }
                if (*cur == edit) {
                    out = out == _ids.end() ? cur : std::move(keep, cur, out);
                    keep = ++cur;
                }
            }
            if (out != _ids.end()) {
                _ids.erase(std::move(keep, _ids.end(), out), _ids.end());
            }
        }
    }
    else {
        // Find the range in _ids that we will add to.
        auto addBegin = lower_bound(_ids.begin(), _ids.end(), _edits.front());

        if (_edits.size() == 1) {
            // For a single add, we just insert it (even if present).
            _ids.insert(addBegin, std::move(_edits.front()));
        }
        else {
            auto addEnd = upper_bound(addBegin, _ids.end(), _edits.back());

            if (std::distance(addBegin, addEnd) == 0) {
                // We're inserting into an empty range in _ids.
                _ids.insert(addBegin,
                            std::make_move_iterator(_edits.begin()),
                            std::make_move_iterator(_edits.end()));
            }
            else {
                // Merge in place from the back.  Grow _ids, shift the elements
                // after the range to the new end, then merge the range with
                // _edits backward into the space that opens up.  We merge
                // rather than take the union to preserve the semantics that
                // inserting duplicates always succeeds.
                const size_t numAdded = _edits.size();
                const size_t addBeginIdx = distance(_ids.begin(), addBegin);
                const size_t addEndIdx = distance(_ids.begin(), addEnd);
                const size_t oldSize = _ids.size();
                _ids.resize(oldSize + numAdded);

                std::move_backward(_ids.begin() + addEndIdx,
                                   _ids.begin() + oldSize, _ids.end());

                const auto first = _ids.begin() + addBeginIdx;
                auto src = _ids.begin() + addEndIdx;
                auto dst = src + numAdded;
                auto edit = _edits.end();
                while (edit != _edits.begin()) {
                    if (src != first && *(edit-1) < *(src-1)) {
                        *--dst = std::move(*--src);
                    }
                    else {
                        *--dst = std::move(*--edit);
                    }
                }
            }
        }
    }
    
    _edits.clear();
    _mode = _NoMode;
}

PXR_NAMESPACE_CLOSE_SCOPE
