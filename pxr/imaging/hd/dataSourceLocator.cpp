//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/dataSourceLocator.h"

#include "pxr/base/arch/hints.h"

#include <sstream>
#include <algorithm>

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

const HdDataSourceLocator &
HdDataSourceLocator::EmptyLocator()
{
    static HdDataSourceLocator theEmptyLocator;
    return theEmptyLocator;
}

HdDataSourceLocator::HdDataSourceLocator() = default;

HdDataSourceLocator::HdDataSourceLocator(
    const TfToken &t1)
{
    // ignore single empty token
    if (!t1.IsEmpty()) {
        _tokens.push_back(t1);
    }
}

HdDataSourceLocator::HdDataSourceLocator(
    const TfToken &t1, const TfToken &t2)
    : _tokens{t1, t2}
{
}

HdDataSourceLocator::HdDataSourceLocator(
    const TfToken &t1, const TfToken &t2, const TfToken &t3)
    : _tokens{t1, t2, t3}
{
}

HdDataSourceLocator::HdDataSourceLocator(
    const TfToken &t1, const TfToken &t2, const TfToken &t3, const TfToken &t4)
    : _tokens{t1, t2, t3, t4}
{
}

HdDataSourceLocator::HdDataSourceLocator(
    const TfToken &t1, const TfToken &t2, const TfToken &t3, const TfToken &t4,
    const TfToken &t5)
    : _tokens{t1, t2, t3, t4, t5}
{
}

HdDataSourceLocator::HdDataSourceLocator(
    const TfToken &t1, const TfToken &t2, const TfToken &t3,
    const TfToken &t4, const TfToken &t5, const TfToken &t6)
    : _tokens{t1, t2, t3, t4, t5, t6}
{
}

HdDataSourceLocator::HdDataSourceLocator(size_t count, const TfToken *tokens)
{
    _tokens.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        _tokens.push_back(tokens[i]);
    }
}

size_t
HdDataSourceLocator::GetElementCount() const {
    return _tokens.size();
}

const TfToken &
HdDataSourceLocator::GetElement(size_t i) const {
    return _tokens[i];
}

const TfToken &
HdDataSourceLocator::GetLastElement() const
{
    if (_tokens.empty()) {
        static TfToken empty;
        return empty;
    }

    return _tokens.back();
}

const TfToken &
HdDataSourceLocator::GetFirstElement() const
{
    if (_tokens.empty()) {
        static TfToken empty;
        return empty;
    }

    return _tokens.front();
}

HdDataSourceLocator
HdDataSourceLocator::ReplaceLastElement(const TfToken &name) const
{
    if (_tokens.empty()) {
        return *this;
    }

    HdDataSourceLocator result(*this);
    result._tokens.back() = name;

    return result;
}

HdDataSourceLocator
HdDataSourceLocator::RemoveLastElement() const {
    if (_tokens.size() < 2) {
        return HdDataSourceLocator();
    }

    return HdDataSourceLocator(_tokens.size() - 1, _tokens.data());
}

HdDataSourceLocator
HdDataSourceLocator::RemoveFirstElement() const {
    if (_tokens.size() < 2) {
        return HdDataSourceLocator();
    }

    return HdDataSourceLocator(_tokens.size() - 1, _tokens.data() + 1);
}

HdDataSourceLocator
HdDataSourceLocator::Append(const TfToken &name) const
{
    HdDataSourceLocator result(*this);
    result._tokens.push_back(name);
    return result;
}

HdDataSourceLocator
HdDataSourceLocator::Append(const HdDataSourceLocator &locator) const
{
    HdDataSourceLocator result(*this);
    result._tokens.insert(result._tokens.end(),
        locator._tokens.begin(), locator._tokens.end());
    return result;
}

HdDataSourceLocator
HdDataSourceLocator::Prepend(const TfToken &name) const
{
    HdDataSourceLocator result;
    result._tokens.push_back(name);
    result._tokens.insert(result._tokens.end(), _tokens.begin(), _tokens.end());
    return result;
}

HdDataSourceLocator
HdDataSourceLocator::Prepend(const HdDataSourceLocator &locator) const
{
    HdDataSourceLocator result(locator);
    result._tokens.insert(result._tokens.end(), _tokens.begin(), _tokens.end());
    return result;
}

bool
HdDataSourceLocator::HasPrefix(const HdDataSourceLocator &prefix) const
{
    if (prefix.IsEmpty()) {
        return true;
    }

    if (prefix._tokens.size() > _tokens.size()) {
        return false;
    }

    for (size_t i = 0, e = prefix._tokens.size(); i < e; ++i) {
        if (_tokens[i] != prefix._tokens[i]) {
            return false;
        }
    }

    return true;
}


HdDataSourceLocator
HdDataSourceLocator::GetCommonPrefix(const HdDataSourceLocator &other) const
{
    const size_t maxPossibleLen = std::min(_tokens.size(), other._tokens.size());
    size_t i = 0;
    for (; i < maxPossibleLen; ++i) {
        if (_tokens[i] != other._tokens[i]) {
            break;
        }
    }

    return HdDataSourceLocator(i, _tokens.data());
}

bool
HdDataSourceLocator::Intersects(const HdDataSourceLocator &other) const
{
    const size_t commonLength = std::min(other._tokens.size(), _tokens.size());
    for (size_t i = 0; i < commonLength; ++i) {
        if (other._tokens[i] != _tokens[i]) {
            return false;
        }
    }
    return true;
}

HdDataSourceLocator
HdDataSourceLocator::ReplacePrefix(
        const HdDataSourceLocator &oldPrefix,
        const HdDataSourceLocator &newPrefix) const
{
    if (!HasPrefix(oldPrefix)) {
        return *this;
    }

    HdDataSourceLocator result(newPrefix);
    result._tokens.insert(result._tokens.end(),
        _tokens.begin() + oldPrefix.GetElementCount(), _tokens.end());

    return result;
}


std::string
HdDataSourceLocator::GetString(const char *delimiter) const
{
    if (_tokens.empty()) {
        return std::string();
    }
    std::ostringstream buffer;
    for (size_t i = 0, e = _tokens.size() - 1; i < e; ++i) {
        buffer << _tokens[i].data() << delimiter;
    }

    buffer << _tokens.back().data();

    return buffer.str();
}

bool
HdDataSourceLocator::operator<(const HdDataSourceLocator &rhs) const
{
    const size_t lhslen = _tokens.size();
    const size_t rhslen = rhs._tokens.size();
    const size_t len = std::min(lhslen, rhslen);

    for (size_t i = 0; i < len; ++i) {
        if (_tokens[i] < rhs._tokens[i]) {
            return true;
        } else if (_tokens[i] > rhs._tokens[i]) {
            return false;
        } // else if (_tokens[i] == rhs._tokens[i]) continue;
    }

    // If we end up here, one of *this and rhs is a prefix of the other,
    // or they're equal.
    return lhslen < rhslen;
}


std::ostream&
operator<<(std::ostream& out, const HdDataSourceLocator &self)
{
    return out << self.GetString();
}

//-----------------------------------------------------------------------------

void
HdDataSourceLocatorSet::_Normalize()
{
    if (_locators.size() < 2) {
        return;
    }

    std::sort(_locators.begin(), _locators.end());

    for (size_t i = 1; i < _locators.size(); ) {
        if (_locators[i].Intersects(_locators[i-1])) {
            _locators.erase(_locators.begin() + i);
        } else {
            ++i;
        }
    }
}


const HdDataSourceLocatorSet &
HdDataSourceLocatorSet::UniversalSet()
{
    static const HdDataSourceLocatorSet &result{
        HdDataSourceLocator::EmptyLocator()};
    return result;
}

HdDataSourceLocatorSet::HdDataSourceLocatorSet(
    const HdDataSourceLocator &locator)
  : _locators{locator}
{
}

HdDataSourceLocatorSet::HdDataSourceLocatorSet(
    const std::initializer_list<const HdDataSourceLocator> &l)
  : _locators(l.begin(), l.end())
{
    // Since the initializer list comes in unsorted, we need to sort and
    // uniquify it.
    _Normalize();
}

static bool
_LessThanNotPrefix(const HdDataSourceLocator &a, const HdDataSourceLocator &b)
{
    // Long-form version of "return (a < b) && (!b.HasPrefix(a));"

    if (a.IsEmpty()) {
        return false;
    }

    const size_t lhslen = a.GetElementCount();
    const size_t rhslen = b.GetElementCount();
    const size_t len = std::min(lhslen, rhslen);

    for (size_t i = 0; i < len; ++i) {
        if (a.GetElement(i) < b.GetElement(i)) {
            // e.g. /foo/a < /foo/b
            return true;
        } else if (a.GetElement(i) > b.GetElement(i)) {
            // e.g. /foo/b !< /foo/a
            return false;
        } // else if (a.GetElement(i) == b.GetElement(i)) { continue; }
    }

    // If we end up here, one of a and b is a prefix of the other, or they're
    // equal. if lhslen <= rhslen, a is a prefix of b, so return false.
    // ... otherwise, a > b, so return false.
    return false;
}

void
HdDataSourceLocatorSet::_InsertAndDeleteSuffixes(
    _Locators::iterator *position,
    const HdDataSourceLocator &locator)
{
    *position = _locators.insert(*position, locator);

    // If we inserted anything, we need to walk forward and delete any
    // suffixes of the inserted item.
    auto deleteStart = *position + 1;
    auto deleteEnd = deleteStart;
    while (deleteEnd != _locators.end() && deleteEnd->HasPrefix(**position)) {
        deleteEnd++;
    }
    if (deleteEnd != deleteStart) {
        *position = _locators.erase(deleteStart, deleteEnd) - 1;
    }
}

void
HdDataSourceLocatorSet::insert(const HdDataSourceLocator &locator)
{
    if (_locators.empty()) {
        _locators.push_back(locator);
        return;
    }

    // Binary search for locators of interest already in the set.
    // Locators already in the set can be split into 3 disjoint ranges:
    //  { e < locator }, { locator.HasPrefix(e) }, { e > locator }
    // (... in that order, where the first range fails the prefix check).
    // Searching for "locator":
    // 1.) If we find e with locator.HasPrefix(e), no-op since locator is
    //     already in the set.
    // 2.) Otherwise, we look for the first index where e > locator, and
    //     insert locator there.

    auto it = std::lower_bound(_locators.begin(), _locators.end(),
            locator, _LessThanNotPrefix);
    if (it != _locators.end() && locator.HasPrefix(*it)) {
        return;
    }
    // Otherwise, we need to add it.
    _InsertAndDeleteSuffixes(&it, locator);
}

void
HdDataSourceLocatorSet::insert(const HdDataSourceLocatorSet &locatorSet)
{
    if (locatorSet._locators.empty()) {
        return;
    }

    if (_locators.empty()) {
        _locators = locatorSet._locators;
        return;
    }

    // Like with single-insert, both sets are sorted, so that for any element
    // L in locatorSet, _locators is partitioned into the disjoint ranges:
    //  { e < locator }, { locator.HasPrefix(e) }, { e > locator }
    // (... in that order, where the first range fails the prefix check).
    // Additionally, for an element M in locatorSet, with M > L, the partition
    // indices for that element in _locators will be >= the partition indices
    // for L. This monotonicity lets us do the set insert with one pass through
    // _locators.
    auto thisIt = _locators.begin();
    for (auto otherIt = locatorSet._locators.begin();
         otherIt != locatorSet._locators.end(); ++otherIt) {
         while (thisIt != _locators.end() &&
               _LessThanNotPrefix(*thisIt, *otherIt)) {
             // Note: std::lower_bound here would improve our best case and
             // hurt our worst case, O(log N)/O(N log N) vs O(N) respectively.
             ++thisIt;
         }
         if (thisIt == _locators.end()) {
             // If we've reached the end of _locators, append the rest of
             // locatorSet to the end.
             _locators.insert(thisIt, otherIt, locatorSet._locators.end());
             return;
         }
         if (otherIt->HasPrefix(*thisIt)) {
             // otherIt is already in the set.
             continue;
         }
         // Otherwise, we need to add it.
         _InsertAndDeleteSuffixes(&thisIt, *otherIt);
    }
}

void
HdDataSourceLocatorSet::insert(HdDataSourceLocatorSet &&locatorSet)
{
    if (_locators.empty()) {
        _locators = std::move(locatorSet._locators);
        return;
    }

    // Note that the swapping the two small vectors might be expensive
    // itself, so introducing a cut-off.
    // This is a guess - we have not run performance tests to find the
    // optimal value for this cut-off.
    constexpr size_t _swapCutoff = 5;

    if (_locators.size() + _swapCutoff < locatorSet._locators.size()) {
        _locators.swap(locatorSet._locators);
    }

    insert(locatorSet);
}

void
HdDataSourceLocatorSet::append(const HdDataSourceLocator &locator)
{
    if (_locators.size() == 0 ||
        _LessThanNotPrefix(_locators.back(), locator)) {
        _locators.push_back(locator);
    } else {
        insert(locator);
    }
}

HdDataSourceLocatorSet::const_iterator
HdDataSourceLocatorSet::begin() const
{
    return _locators.begin();
}

HdDataSourceLocatorSet::const_iterator
HdDataSourceLocatorSet::end() const
{
    return _locators.end();
}

HdDataSourceLocatorSet::const_iterator
HdDataSourceLocatorSet::_FirstIntersection(
    const HdDataSourceLocator &locator) const
{
    // Note: operator< and _LessThanNotPrefix are almost as expensive as
    // intersects, so for very small arrays the std::lower_bound can actually
    // hurt us and we want to just loop over everything: we'd do O(ceil(log a))
    // compares plus an intersects, vs O(a) intersects. (e.g. a = 4, we'd do
    // up to 3 compares plus an intersects).
    constexpr size_t _binarySearchCutoff = 5;

    if (_locators.size() < _binarySearchCutoff) {
        for (const_iterator it = _locators.begin();
             it != _locators.end();
             ++it) {
            if (it->Intersects(locator)) {
                return it;
            }
        }
        return _locators.end();
    }

    // As with insert, we can split the set into 3 disjoint ranges.
    // We want to find the first item such that e > locator or
    // locator.HasPrefix(e); lower_bound gets us this in O(log N), and then
    // we just need to check which condition holds. Note that if e > locator,
    // we need to check if e.HasPrefix(locator) as well...
    const auto it = std::lower_bound(_locators.begin(), _locators.end(),
            locator, _LessThanNotPrefix);
    if (it != _locators.end() &&
        (locator.HasPrefix(*it) || it->HasPrefix(locator))) {
        return it;
    }

    return _locators.end();
}

bool
HdDataSourceLocatorSet::Intersects(const HdDataSourceLocator &locator) const
{
    return _FirstIntersection(locator) != _locators.end();
}

bool
HdDataSourceLocatorSet::Intersects(
        const HdDataSourceLocatorSet &locatorSet) const
{
    // Note: operator< and _LessThanNotPrefix are almost as expensive as
    // intersects, so for very small arrays where we do O(a+b) compares
    // and then an intersects, this can be more expensive than just doing
    // O(a*b) compares. (e.g. a=b=2 yields 5 vs 4 operations).
    constexpr size_t _zipperCompareCutoff = 9;

    if (_locators.size() * locatorSet._locators.size() < _zipperCompareCutoff) {
        for (const auto &a : _locators) {
            for (const auto &b : locatorSet) {
                if (a.Intersects(b)) {
                    return true;
                }
            }
        }
        return false;
    }

    TRACE_FUNCTION();

    // As with insert, we can split the set into 3 disjoint ranges.
    // Additionally, as we walk elements in locatorSet, if M > L, the range
    // partitions for M will be >= the partition indices for L. This
    // monotonicity lets us walk both sets simultaneously, in one pass, looking
    // for matches.
    auto thisIt = _locators.begin();
    for (auto otherIt = locatorSet._locators.begin();
         otherIt != locatorSet._locators.end(); ++otherIt) {
        while (thisIt != _locators.end() &&
               _LessThanNotPrefix(*thisIt, *otherIt)) {
            // See the note in Set::insert about performance of iteration vs
            // std::lower_bound...
            ++thisIt;
        }
        if (thisIt == _locators.end()) {
            // Couldn't find otherIt in _locators, and since everything
            // past otherIt > otherIt, they all are not in _locators either.
            return false;
        }
        if (otherIt->HasPrefix(*thisIt)) {
            return true;
        }
        // At this point, we know that *thisIt >= *otherIt, and
        // !otherIt->HasPrefix(*thisIt). If !thisIt->HasPrefix(*otherIt), then
        // otherIt isn't part of any intersection and we can continue to the
        // next element.
        if (thisIt->HasPrefix(*otherIt)) {
            return true;
        }
    }

    return false;
}

bool
HdDataSourceLocatorSet::Contains(
        const HdDataSourceLocator &locator) const
{
    // Note: See _binarySearchCutoff in Intersects.
    constexpr size_t _binarySearchCutoff = 5;

    if (_locators.size() < _binarySearchCutoff) {
        for (const auto &l : _locators) {
            if (locator.HasPrefix(l)) {
                return true;
            }
        }
        return false;
    }

    TRACE_FUNCTION();

    const auto it = std::lower_bound(
        _locators.begin(), _locators.end(),
        locator, _LessThanNotPrefix);
    return it != _locators.end() && locator.HasPrefix(*it);    
}

bool
HdDataSourceLocatorSet::IsEmpty() const
{
    return _locators.empty();
}

HdDataSourceLocatorSet
HdDataSourceLocatorSet::ReplacePrefix(
    const HdDataSourceLocator &oldPrefix,
    const HdDataSourceLocator &newPrefix) const
{
    if (ARCH_UNLIKELY(IsEmpty() || (oldPrefix == newPrefix))) {
        return *this;
    }

    // Note: See _binarySearchCutoff in Intersects.
    constexpr size_t _binarySearchCutoff = 5;

    if (_locators.size() < _binarySearchCutoff) {
        HdDataSourceLocatorSet result = *this;
        _Locators &locators = result._locators;

        for (auto &l : locators) {
            l = l.ReplacePrefix(oldPrefix, newPrefix);
        }

        result._Normalize();
        return result;
    }

    TRACE_FUNCTION();
    // lower_bound with operator < gives us the first element that is not less 
    // than (i.e., greater than or equal to) oldPrefix, which is what we want
    // here (unlike in the insertion case where we use _LessThanNotPrefix).
    // e.g. given the locator set {a/a, a/b/c, a/b/d, a/c} and the prefix a/b,
    // lower_bound gives us the element a/b/c.
    auto it =
        std::lower_bound(_locators.begin(), _locators.end(), oldPrefix);
    
    if (it != _locators.end()) {

        if (it->HasPrefix(oldPrefix)) {
            
            HdDataSourceLocatorSet result = *this;
            _Locators &locators = result._locators;

            auto lowerIt = locators.begin() +
                        std::distance(_locators.begin(), it);

            if (*lowerIt == oldPrefix) {
                // The closed under descendancy nature of HdDataLocatorSet
                // implies that the next element cannot be a descendant of the 
                // current one, implying that it won't share the prefix.
                *lowerIt = newPrefix;

            } else {

                // Find first element such that elem.HasPrefix(oldPrefix) is
                // false.
                auto upperIt =
                    std::lower_bound(
                        std::next(lowerIt, 1), locators.end(), oldPrefix,
                        [](const HdDataSourceLocator &elem,
                           const HdDataSourceLocator &prefix) {

                            return elem.HasPrefix(prefix);
                        
                        });

                for (auto it = lowerIt; it < upperIt; it++) {
                    *it = it->ReplacePrefix(oldPrefix, newPrefix);
                }
            }

            result._Normalize();

            return result;
        }
        // Otherwise, there's nothing to do since no element in the set
        // has the prefix oldPrefix.
    }
 
    return *this;
}

const HdDataSourceLocator &
HdDataSourceLocatorSet::IntersectionIterator::operator*() const
{
    if (_isFirst && _locator.HasPrefix(*_iterator)) {
        return _locator;
    }
    return *_iterator;
}

HdDataSourceLocatorSet::IntersectionIterator &
HdDataSourceLocatorSet::IntersectionIterator::operator++()
{
    _isFirst = false;
    ++_iterator;
    if (_iterator != _end && !_iterator->HasPrefix(_locator)) {
        _iterator = _end;
    }
    return *this;
}

HdDataSourceLocatorSet::IntersectionIterator
HdDataSourceLocatorSet::IntersectionIterator::operator++(int)
{
    IntersectionIterator result(*this);
    operator++();
    return result;
}

HdDataSourceLocatorSet::IntersectionView
HdDataSourceLocatorSet::Intersection(const HdDataSourceLocator &locator) const
{
    return IntersectionView(
        IntersectionIterator(
            /* isFirst = */ true,
            _FirstIntersection(locator),
            end(),
            locator),
        IntersectionIterator(
            /* isFirst = */ false,
            end(),
            end(),
            locator));
}

std::ostream&
operator<<(std::ostream& out, const HdDataSourceLocatorSet &self) 
{
    out << "{ ";
    bool separator = false;
    for (auto const& l : self) {
        if (separator) {
            out << ", ";
        } else {
            separator = true;
        }
        out << l;
    }
    out << " }";
    return out;

}

PXR_NAMESPACE_CLOSE_SCOPE
