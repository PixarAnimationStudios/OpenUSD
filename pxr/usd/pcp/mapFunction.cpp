//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/ostreamMethods.h"

#include <limits>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

using _PathPair = std::pair<SdfPath, SdfPath>;
using _PathPairVector = std::vector<_PathPair>;

// Order PathPairs using element count and FastLessThan.
struct _PathPairOrder
{
    bool operator()(const _PathPair &lhs, const _PathPair &rhs) {
        // First order paths by element count which allows to us to be 
        // more efficient in finding the best path match in the source to
        // target direction. This also makes sure that "root identity" 
        // elements appear first.
        if (lhs.first.GetPathElementCount() != rhs.first.GetPathElementCount()) {
            return lhs.first.GetPathElementCount() < 
                rhs.first.GetPathElementCount();
        }

        // Otherwise, the path sourece path elememnt count is the same so
        // use the arbitrary fast less than.
        SdfPath::FastLessThan less;
        return less(lhs.first, rhs.first) ||
            (lhs.first == rhs.first && less(lhs.second, rhs.second));
    }
};

// Comparator for lower bounding source paths in a _PathPairOrder ordered
// container.
struct _PathPairOrderSourceLowerBound
{   
    bool operator()(const _PathPair &lhs, const SdfPath &rhs) {
        return _PathLessThan(lhs.first, rhs);
    }

    bool operator()(const SdfPath &lhs, const _PathPair &rhs) {
        return _PathLessThan(lhs, rhs.first);
    }
private:
    bool _PathLessThan(const SdfPath &lhs, const SdfPath &rhs)
    {
        if (lhs.GetPathElementCount() != rhs.GetPathElementCount()) {
            return lhs.GetPathElementCount() < rhs.GetPathElementCount();
        }

        SdfPath::FastLessThan less;
        return less(lhs, rhs);
    }
};

struct _SourceAndTargetPathPairs final
{
    static const int _MaxLocalPairs = 2;

    _SourceAndTargetPathPairs() {};

    _SourceAndTargetPathPairs(
        _PathPair const *begin, _PathPair const *end, bool hasRootIdentity)
        : numPairs(end-begin)
        , hasRootIdentity(hasRootIdentity) {
        if (numPairs == 0)
            return;
        if (numPairs <= _MaxLocalPairs) {
            std::uninitialized_copy(begin, end, localPairs);
        }
        else {
            new (&remotePairs) std::shared_ptr<_PathPair>(
                new _PathPair[numPairs], std::default_delete<_PathPair[]>());
            std::copy(begin, end, remotePairs.get());
        }
    }
        
    _SourceAndTargetPathPairs(_SourceAndTargetPathPairs const &other)
        : numPairs(other.numPairs)
        , hasRootIdentity(other.hasRootIdentity) {
        if (numPairs <= _MaxLocalPairs) {
            std::uninitialized_copy(
                other.localPairs,
                other.localPairs + other.numPairs, localPairs);
        }
        else {
            new (&remotePairs) std::shared_ptr<_PathPair>(other.remotePairs);
        }
    }
    _SourceAndTargetPathPairs(_SourceAndTargetPathPairs &&other)
        : numPairs(other.numPairs)
        , hasRootIdentity(other.hasRootIdentity) {
        if (numPairs <= _MaxLocalPairs) {
            _PathPair *dst = localPairs;
            _PathPair *src = other.localPairs;
            _PathPair *srcEnd = other.localPairs + other.numPairs;
            for (; src != srcEnd; ++src, ++dst) {
                ::new (static_cast<void*>(std::addressof(*dst)))
                    _PathPair(std::move(*src));
            }
        }
        else {
            new (&remotePairs)
                std::shared_ptr<_PathPair>(std::move(other.remotePairs));
        }
    }
    _SourceAndTargetPathPairs &
    operator=(_SourceAndTargetPathPairs const &other) {
        if (this != &other) {
            this->~_SourceAndTargetPathPairs();
            new (this) _SourceAndTargetPathPairs(other);
        }
        return *this;
    }
    _SourceAndTargetPathPairs &
    operator=(_SourceAndTargetPathPairs &&other) {
        if (this != &other) {
            this->~_SourceAndTargetPathPairs();
            new (this) _SourceAndTargetPathPairs(std::move(other));
        }
        return *this;
    }
    ~_SourceAndTargetPathPairs() {
        if (numPairs <= _MaxLocalPairs) {
            for (_PathPair *p = localPairs; numPairs--; ++p) {
                p->~_PathPair();
            }
        }
        else {
            remotePairs.~shared_ptr<_PathPair>();
        }
    }

    bool IsNull() const {
        return numPairs == 0 && !hasRootIdentity;
    }

    _PathPair const *begin() const {
        return numPairs <= _MaxLocalPairs ? localPairs : remotePairs.get();
    }

    _PathPair const *end() const {
        return begin() + numPairs;
    }

    bool operator==(_SourceAndTargetPathPairs const &other) const {
        return numPairs == other.numPairs &&
            hasRootIdentity == other.hasRootIdentity &&
            std::equal(begin(), end(), other.begin());
    }

    bool operator!=(_SourceAndTargetPathPairs const &other) const {
        return !(*this == other);
    }

    template <class HashState>
    friend void TfHashAppend(
        HashState &h, _SourceAndTargetPathPairs const &data) {
        h.Append(data.hasRootIdentity);
        h.Append(data.numPairs);
        h.AppendRange(std::begin(data), std::end(data));
    }

    union {
        _PathPair localPairs[_MaxLocalPairs > 0 ? _MaxLocalPairs : 1];
        std::shared_ptr<_PathPair> remotePairs;
    };
    typedef int PairCount;
    PairCount numPairs = 0;
    bool hasRootIdentity = false;
};

} // end anonymous namespace

struct PcpMapFunction::_Mappings final
{
    using SharedPtr = std::shared_ptr<_Mappings>;
    using SharedPtrVector = std::vector<std::shared_ptr<_Mappings>>;

    _Mappings() = default;
    _Mappings(_PathPair const *begin, _PathPair const *end, bool hasRootIdentity)
        : data(std::in_place_type<_SourceAndTargetPathPairs>, 
            begin, end, hasRootIdentity)
    { }

    template <class T>
    _Mappings(T&& mappings)
        : data(std::forward<T>(mappings))
    { }

    bool IsDeferredComposition() const
    {
        return std::holds_alternative<_Mappings::SharedPtrVector>(data);
    }

    bool IsPathPairs() const
    {
        return std::holds_alternative<_SourceAndTargetPathPairs>(data);
    }

    const _SourceAndTargetPathPairs& GetPathPairs() const
    {
        return std::get<_SourceAndTargetPathPairs>(data);
    }

    _SourceAndTargetPathPairs& GetPathPairs()
    {
        return std::get<_SourceAndTargetPathPairs>(data);
    }

    // Return true if f returns true for all _SourceAndTargetPathPairs
    // contained in this mapping.
    template <class Fn>
    bool AllOf(Fn&& f) const
    {
        if (IsPathPairs()) {
            return std::forward<Fn>(f)(GetPathPairs());
        }

        // List of mappings to visit.
        std::vector<const _Mappings*> toVisit(1, this);
        bool result = true;

        auto check = TfOverloads {
            [&f, &result](const _SourceAndTargetPathPairs& pathPairs) {
                result &= std::forward<Fn>(f)(pathPairs);
            },
            [&toVisit](const _Mappings::SharedPtrVector& mappings) {
                for (const _Mappings::SharedPtr& m : mappings) {
                    toVisit.push_back(m.get());
                }
            }
        };

        while (!toVisit.empty()) {
            const _Mappings* m = toVisit.back();
            toVisit.pop_back();
            
            std::visit(check, m->data);
            if (!result) {
                break;
            }
        }

        return result;
    }

    std::variant<
        // Flat list of (source path, target path) pairs.
        _SourceAndTargetPathPairs,

        // List of mappings that have been composed together,
        // in order from outermost to innermost function.
        _Mappings::SharedPtrVector
    > data;
};

PcpMapFunction::PcpMapFunction()
    : _mappings(new _Mappings)
{
}

PcpMapFunction::PcpMapFunction(
    std::shared_ptr<_Mappings>&& mappings,
    SdfLayerOffset offset)
    : _mappings(std::move(mappings))
    , _offset(offset)
{
}

// Finds the map entry whose source best matches the given path, i.e. the entry
// with the longest source path that is a prefix of the path. minElementCount 
// is used to only look for entries where the source path has at least that many 
// elements.
template <class PairIter>
static PairIter
_GetBestSourceMatch(
    const SdfPath &path, const PairIter &begin, const PairIter &end, 
    size_t minElementCount = 0)
{   
    // Path pairs are ordered such that source path length (i.e. element count)
    // is  non-decreasing. So we start looking for the best match at the last
    // path pair whose source length does not exceed the length of the path 
    // we're matching.
    PairIter i = std::upper_bound(begin, end, path.GetPathElementCount(),
        [](size_t pathLen, const auto &pathPair) {
            return pathLen < pathPair.first.GetPathElementCount();
        });

    // Iterate over path pairs in reverse as the matching pair with the greatest
    // source length will be the best match.
    for (; i != begin;) {
        --i;
        const SdfPath &source = i->first;
        // Opimization if we know the minimum element count (see 
        // _HasBetterTargetMatch). No match if we've reached source paths
        // shorter than the minimum.
        if (source.GetPathElementCount() < minElementCount) {
            return end;
        }
        // If we found a match, this is the best match because of source path
        // length ordering.
        if (path.HasPrefix(source)) {
            return i;
        }
    }

    return end;
}

// Finds the map entry whose target best matches the given path, i.e. the entry
// with the longest target path that is a prefix of the path. minElementCount 
// is used to only look for entries where the target path has at least that many 
// elements.
template <class PairIter>
static PairIter
_GetBestTargetMatch(
    const SdfPath &path, const PairIter &begin, const PairIter &end, 
    size_t minElementCount = 0)
{
    // While path pair entries are ordered by nondecreasing source path length,
    // we can't simultaneously make the same true for target paths. Thus we have
    // to look at every entry comparing path length and checking for a target 
    // path match.
    PairIter bestIter = end;
    size_t bestElementCount = minElementCount;
    for (PairIter i=begin; i != end; ++i) {
        const SdfPath &target = i->second;
        const size_t count = target.GetPathElementCount();
        if (count >= bestElementCount && path.HasPrefix(target)) {
            bestElementCount = count;
            bestIter = i;
        }
    }
    return bestIter;
}

// Returns true if there's a map entry that matches the given target path better
// than the given bestSourceMatch which has already been determined to be the best
// entry for mapping a certain source path to that target path. If invert is 
// true, we swap the meaning of source and target paths.
template <class PairIter>
static bool
_HasBetterTargetMatch(
    const SdfPath &targetPath, const PairIter &begin, const PairIter &end, 
    const PairIter &bestSourceMatch, bool invert = false)
{
    // For a target match to be "better" than the "best source match" the 
    // matching entry's target would have to be longer than the target of the
    // current best match.
    const size_t minElementCount = bestSourceMatch == end ? 0 :
        (invert ? bestSourceMatch->first : bestSourceMatch->second)
            .GetPathElementCount();
    PairIter bestTargetMatch = invert ?
        _GetBestSourceMatch(targetPath, begin, end, minElementCount) :
        _GetBestTargetMatch(targetPath, begin, end, minElementCount);
    return bestTargetMatch != end && bestTargetMatch != bestSourceMatch;
}

template <class PairIter>
static bool
_IsRedundant(const PairIter &entry, const PairIter &begin, const PairIter &end)
{
    const SdfPath &entrySource = entry->first;
    const SdfPath &entryTarget = entry->second;

    const bool isBlock = entryTarget.IsEmpty();

    // A map block is redundant if the source path already wouldn't map without
    // the block.
    if (isBlock) {
        // Find the best matching map entry that affects this source path,
        // ignoring the effect of this block. Note that we find this using the
        // entry source's parent path as the mapping that affects its parent is
        // what this block is blocking from affecting the source.
        PairIter bestSourceMatch = 
            _GetBestSourceMatch(entrySource.GetParentPath(), begin, end);

        // If there is no other mapping that affects the source path or the
        // other mapping is a block itself, then this block is redundant.
        if (bestSourceMatch == end || bestSourceMatch->second.IsEmpty()) {
            return true;
        }

        // Even though we found a relevant mapping for the source path, the
        // path may still not map without the block if the one-to-one 
        // bidirectional mapping requirement isn't met (see _Map)
        //
        // Map the block's source path to the what its target path would be if
        // not blocked
        const SdfPath targetPath = entrySource.ReplacePrefix(
            bestSourceMatch->first, bestSourceMatch->second);

        // If we find a better mapping inverse than the source to target mapping
        // then the source will fail to map without block and the block is 
        // redundant.
        return _HasBetterTargetMatch(targetPath, begin, end, bestSourceMatch);
    }

    // Otherwise we have a normal path mapping entry. This will be redundant
    // if the best matching ancestor mapping would cause the source path to 
    // map to the entry target path

    // Early out, the entry can't be redundant if it renames the source when it
    // is mapped.
    if (entrySource.GetNameToken() != entryTarget.GetNameToken()) {
        return false;
    }

    // Find the best matching map entry that affects this source path,
    // ignoring the effect of this entry. Note that we find this using the
    // entry source's parent path as the mapping that affects its parent is
    // what would affect this source without this entry.
    PairIter bestSourceMatch = 
        _GetBestSourceMatch(entrySource.GetParentPath(), begin, end);

    // If there is no other mapping that affects the source path or the
    // other mapping is a block, then this entry cannot be redundant.
    if (bestSourceMatch == end || bestSourceMatch->second.IsEmpty()) {
        return false;
    }

    // We still need to check that this entry doesn't map the source differently
    // than the other mapping. 

    // Early out; if the best match is the identity mapping then the entry is
    // redundant if it maps the source to the same path.
    if (bestSourceMatch->first.IsAbsoluteRootPath() &&
        bestSourceMatch->second.IsAbsoluteRootPath() &&
        entrySource == entryTarget) {
        return true;
    }
    
    // Early out; if the best match would map the source path to a different 
    // namespace depth than the entry does, then entry cannot be redundant.
    if ((entryTarget.GetPathElementCount() - 
            bestSourceMatch->second.GetPathElementCount()) !=
        (entrySource.GetPathElementCount() - 
            bestSourceMatch->first.GetPathElementCount())) {
        return false;
    }

    // This loop here is the equivalent of checking whether 
    // entrySource.ReplacePrefix(bestSourceMatch->first, bestSourceMatch->second)
    // results in the same path as entryTarget and returning false if it does
    // not.
    SdfPath sourceAncestorPath = entrySource.GetParentPath();
    SdfPath targetAncestorPath = entryTarget.GetParentPath();
    while(sourceAncestorPath != bestSourceMatch->first) {
        if (sourceAncestorPath.GetNameToken() != 
                targetAncestorPath.GetNameToken()) {
            return false;
        }
        sourceAncestorPath = sourceAncestorPath.GetParentPath();
        targetAncestorPath = targetAncestorPath.GetParentPath();
    }
    if (bestSourceMatch->second != targetAncestorPath) {
        return false;
    }

    // It's still possible that map entry we matched does not actually map our 
    // path if there's a better inverse mapping for our target (see _Map). 
    // In this case, this entry will not be redundant. Note again that we use
    // the parent path to exclude this entry itself.
    return !_HasBetterTargetMatch(
        entryTarget.GetParentPath(), begin, end, bestSourceMatch);
}

// Canonicalize pairs in-place by removing all redundant entries.  Redundant
// entries are those which can be removed without changing the semantics of the
// correspondence.  Note that this function modifies both the content of `begin`
// and `end` as well as the *value* of `begin` and `end` to produce the
// resulting range.  Return true if there's a root identity mapping ('/' ->
// '/').  It will not appear in the resulting \p vec. Also note that the entries
// in the range must be in sorted in _PathPairOrder before this function is 
// called.
template <class PairIter>
static bool
_Canonicalize(PairIter &begin, PairIter &end)
{
    TRACE_FUNCTION();

    for (PairIter i = begin; i != end; /* increment below */) {

        if (_IsRedundant(i, begin, end)) {
            // Entries are already sorted so move all subsequent entries down
            // one to erase this item. Note that while this is potentially 
            // O(n^2), in practice it's more efficient to keep the entries 
            // sorted for _IsRedundant checks than it is to make the erase 
            // efficient as we're guaranteed to call _IsRedundant on every
            // entry but very few entries will actually be redundant and require
            // erasure.
            std::move(i + 1, end, i);
            --end;
        } else {
            ++i;
        }
    }

    bool hasRootIdentity = false;
    if (begin != end) {
        auto const &absroot = SdfPath::AbsoluteRootPath();
        if (begin->first == absroot && begin->second == absroot) {
            ++begin;
            hasRootIdentity = true;
        }
    }
    return hasRootIdentity;
}

PcpMapFunction
PcpMapFunction::Create(const PathMap &sourceToTarget,
                       const SdfLayerOffset &offset)
{
    TfAutoMallocTag tag("Pcp", "PcpMapFunction::Create");
    TRACE_FUNCTION();

    // If we're creating the identity map function, just return it directly.
    auto absoluteRoot = SdfPath::AbsoluteRootPath();
    if (sourceToTarget.size() == 1 && offset.IsIdentity()) {
        auto const &pathPair = *sourceToTarget.begin();
        if (pathPair.first == absoluteRoot && pathPair.second == absoluteRoot) {
            return Identity();
        }
    }
    
    // Validate the arguments.
    {
        // Make sure we don't exhaust the representable range.
        const _SourceAndTargetPathPairs::PairCount maxPairCount =
            std::numeric_limits<_SourceAndTargetPathPairs::PairCount>::max();
        if (sourceToTarget.size() > maxPairCount) {
            TF_RUNTIME_ERROR("Cannot construct a PcpMapFunction with %zu "
                             "entries; limit is %zu",
                             sourceToTarget.size(), size_t(maxPairCount));
            return PcpMapFunction();
        }
    }
    for (const auto &[source, target] : sourceToTarget) {
        // Source and target paths must be prim paths, because mappings
        // are used on arcs and arcs are only expressed between prims.
        //
        // This is a coding error, because a PcpError should have been
        // emitted about these conditions before getting to this point.
        //
        // Additionally, the target path may be empty which is used to indicate
        // that a source path cannot be mapped. This is used to "block" the 
        // mapping of paths that would otherwise translate across a mapping
        // of one of its ancestor paths.
        auto isValidMapPath = [](const SdfPath &path) {
            return path.IsAbsolutePath() && 
                (path.IsAbsoluteRootOrPrimPath() ||
                 path.IsPrimVariantSelectionPath());
        };

        if (!isValidMapPath(source) ||
            !(target.IsEmpty() || isValidMapPath(target))) {
            TF_CODING_ERROR("The mapping of '%s' to '%s' is invalid.",
                            source.GetText(), target.GetText());
            return PcpMapFunction();
        }
    }

    _PathPairVector vec(sourceToTarget.begin(), sourceToTarget.end());
    _PathPair *begin = vec.data(), *end = vec.data() + vec.size();
    // XXX: This would be unnecessary if we used _PathPairOrder as the 
    // comparator input PathMap.
    std::sort(begin, end, _PathPairOrder());
    bool hasRootIdentity = _Canonicalize(begin, end);
    return PcpMapFunction(
        std::make_shared<_Mappings>(
            begin, end, hasRootIdentity), offset);
}

PcpMapFunction
PcpMapFunction::DeferredComposition(const PcpMapFunction& mapFn)
{
    // An identity map function never defers composition since composing
    // an identity with another map function is trivial. See Compose.
    if (mapFn.IsIdentity()) {
        return mapFn;
    }

    return PcpMapFunction(
        std::make_shared<_Mappings>(
            _Mappings::SharedPtrVector{mapFn._mappings}),
        mapFn._offset);
}

PcpMapFunction
PcpMapFunction::ImpliedClass(const PcpMapFunction& transferFunc,
                             const PcpMapFunction& classArc)
{
    TfAutoMallocTag tag("Pcp", "PcpMapFunction::ImpliedClass");
    TRACE_FUNCTION();

    if (transferFunc.IsIdentity()) {
        return classArc;
    }

    PcpMapFunction f = transferFunc.Compose(
        classArc.Compose(transferFunc.GetInverse()));

    if (!f.HasRootIdentity()) {
        // _GetNormalized always returns a PcpMapFunction whose mappings
        // are represented by _SourceAndTargetPathPairs, so we can just
        // set the root identity bit. This is safe to do because f is
        // a newly-created map function whose _mappings member isn't being
        // shared with any other map function.
        //
        // XXX:
        // We could explore just setting the hasRootIdentity bit on each
        // stored mapping if _mappings holds a list of mappings.
        f = f._GetNormalized();
        f._mappings->GetPathPairs().hasRootIdentity = true;
    }
    return f;
}

bool
PcpMapFunction::IsNull() const
{
    return _mappings->AllOf(
        [](const _SourceAndTargetPathPairs& pathPairs) {
            return pathPairs.IsNull();
        });
}

bool 
PcpMapFunction::IsDeferredComposition() const
{
    return _mappings->IsDeferredComposition();
}

PcpMapFunction *Pcp_MakeIdentity()
{
    PcpMapFunction *ret = new PcpMapFunction;
    ret->_mappings->GetPathPairs().hasRootIdentity = true;
    return ret;
}

const PcpMapFunction &
PcpMapFunction::Identity()
{
    static PcpMapFunction *_identityMapFunction = Pcp_MakeIdentity();
    return *_identityMapFunction;
}

TF_MAKE_STATIC_DATA(PcpMapFunction::PathMap, _identityPathMap)
{
    const SdfPath & absoluteRootPath = SdfPath::AbsoluteRootPath();
    _identityPathMap->insert(
        std::make_pair(absoluteRootPath, absoluteRootPath));
}

const PcpMapFunction::PathMap &
PcpMapFunction::IdentityPathMap() 
{
    return *_identityPathMap;
}

bool
PcpMapFunction::IsIdentity() const
{
    return IsIdentityPathMapping() && _offset.IsIdentity();
}

bool
PcpMapFunction::IsIdentityPathMapping() const
{
    return _mappings->AllOf(
        [](const _SourceAndTargetPathPairs& pathPairs) { 
            return pathPairs.numPairs == 0 && pathPairs.hasRootIdentity;
        });
}

bool
PcpMapFunction::HasRootIdentity() const
{
    return _mappings->AllOf(
        [](const _SourceAndTargetPathPairs& pathPairs) {
            return pathPairs.hasRootIdentity;
        });
}

void
PcpMapFunction::Swap(PcpMapFunction& map)
{
    using std::swap;
    swap(_mappings, map._mappings);
    swap(_offset, map._offset);
}

bool
PcpMapFunction::operator==(const PcpMapFunction &map) const
{
    TRACE_FUNCTION();

    // Compare normalized map functions since we want to check that the
    // fully-composed source-to-target mappings are the same between
    // both functions, regardless of whether one or the other was
    // composed from a deferred-composition map function.
    const PcpMapFunction thisFn = _GetNormalized();
    const PcpMapFunction otherFn = map._GetNormalized();

    return thisFn._mappings->GetPathPairs() == otherFn._mappings->GetPathPairs()
        && thisFn._offset == otherFn._offset;
}

bool
PcpMapFunction::operator!=(const PcpMapFunction &map) const
{
    return !(*this == map);
}

static SdfPath
_Map(const SdfPath& path,
     const _SourceAndTargetPathPairs& pairs,
     bool invert)
{
    // Note that we explicitly do not fix target paths here. This
    // is for consistency, so that consumers can be certain of
    // PcpMapFunction's behavior. If consumers want target paths
    // to be fixed, they must be certain to recurse on target paths
    // themselves.
    //
    // XXX: It may be preferable to have PcpMapFunction be in charge
    //      of doing that, but some path translation issues make that
    //      infeasible for now.

    const _PathPair *begin = pairs.begin();
    const _PathPair *end = pairs.end();
 
    // Find longest prefix that has a mapping;
    // this represents the most-specific mapping to apply.
    const _PathPair *bestMatch = invert ?
        _GetBestTargetMatch(path, begin, end) :
        _GetBestSourceMatch(path, begin, end);

    SdfPath result;
    // size_t minTargetElementCount = 0;
    if (bestMatch == end) {
        if (pairs.hasRootIdentity) {
            // Use the root identity.
            result = path;
        }
    } else if (invert) {
        result = path.ReplacePrefix(bestMatch->second, bestMatch->first, 
            /* fixTargetPaths = */ false);
    } else {
        result = path.ReplacePrefix(bestMatch->first, bestMatch->second, 
            /* fixTargetPaths = */ false);
    }

    if (result.IsEmpty()) {
        // No mapping or a blocked mapping found.
        return result;
    }

    // To maintain the bijection, we need to check if the mapped path
    // would translate back to the original path. For instance, given
    // the mapping:
    //      { / -> /, /_class_Model -> /Model }
    //
    // mapping /Model shouldn't be allowed, as the result is noninvertible:
    //      source to target: /Model -> /Model (due to identity mapping)
    //      target to source: /Model -> /_class_Model
    //
    // However, given the mapping:
    //     { /A -> /A/B }
    //
    // mapping /A/B should be allowed, as the result is invertible:
    //     source to target: /A/B -> /A/B/B
    //     target to source: /A/B/B -> /A/B
    //
    // Another example:
    //    { /A -> /B, /C -> /B/C }
    //
    // mapping /A/C should not be allowed, as the result is noninvertible:
    //    source to target: /A/C -> /B/C
    //    target to source: /B/C -> /C
    //
    // For examples, see test case for bug 74847 and bug 112645 in 
    // testPcpMapFunction.
    //
    // XXX: It seems inefficient to have to do this check every time
    //      we do a path mapping. I think it might be possible to figure
    //      out the 'disallowed' mappings and mark them in the mapping
    //      in PcpMapFunction's c'tor. That would let us get rid of this 
    //      code. Figuring out the 'disallowed' mappings might be
    //      expensive though, possibly O(n^2) where n is the number of
    //      paths in the mapping.
    //
    // We know that the best match will match for the inverse mapping fo the 
    // target, but there may be a better (closer) inverse match. If there is,
    // then we can't map this path one-to-one bidirectionally.
    if (_HasBetterTargetMatch(result, begin, end, bestMatch, invert)) {
        return SdfPath();
    }

    return result;
}

SdfPath
PcpMapFunction::_MapPathImpl(bool invert, const SdfPath &path) const
{
    if (_mappings->IsPathPairs()) {
        return _Map(path, _mappings->GetPathPairs(), invert);
    }

    // List of mappings to visit from last to first.
    std::vector<const _Mappings*> toVisit(1, _mappings.get());
    SdfPath result = path;

    auto mapPathImpl = TfOverloads {
        [&result, &invert](const _SourceAndTargetPathPairs& pathPairs) {
            result = _Map(result, pathPairs, invert);
        },
        [&toVisit, &invert](const _Mappings::SharedPtrVector& mappings) {
            if (invert) {
                // Want to apply target -> source mapping from outermost to
                // innermost function. Since mappings is in outer-to-inner
                // order, reverse iterate and append so that the outermost
                // function is the last element in toVisit.
                for (auto it = mappings.rbegin(), e = mappings.rend();
                     it != e; ++it) {
                    toVisit.push_back(it->get());
                }
            }
            else {
                // Want to apply source -> target mapping from innermost to
                // outermost function. Since mappings is in outer-to-inner
                // order, forward iterate and push_back so the innermost
                // function is the last element in toVisit.
                for (auto it = mappings.begin(), e = mappings.end();
                     it != e; ++it) {
                    toVisit.push_back(it->get());
                }
            }
        }
    };
    
    while (!toVisit.empty()) {
        const _Mappings* m = toVisit.back();
        toVisit.pop_back();
        
        std::visit(mapPathImpl, m->data);
        if (result.IsEmpty()) {
            break;
        }
    }

    return result;
}

SdfPath
PcpMapFunction::MapSourceToTarget(const SdfPath & path) const
{
    return _MapPathImpl(/* invert */ false, path);
}

SdfPath
PcpMapFunction::MapTargetToSource(const SdfPath & path) const
{
    return _MapPathImpl(/* invert */ true, path);
}

SdfPathExpression
PcpMapFunction::MapSourceToTarget(
    const SdfPathExpression &pathExpr,
    std::vector<SdfPathExpression::PathPattern> *unmappedPatterns,
    std::vector<SdfPathExpression::ExpressionReference> *unmappedRefs
    ) const
{
    return _MapPathExpressionImpl(/* invert */ false,
                                  pathExpr, unmappedPatterns, unmappedRefs);
}

SdfPathExpression
PcpMapFunction::MapTargetToSource(
    const SdfPathExpression &pathExpr,
    std::vector<SdfPathExpression::PathPattern> *unmappedPatterns,
    std::vector<SdfPathExpression::ExpressionReference> *unmappedRefs
    ) const
{
    return _MapPathExpressionImpl(/* invert */ true,
                                  pathExpr, unmappedPatterns, unmappedRefs);
}

SdfPathExpression
PcpMapFunction::_MapPathExpressionImpl(
    bool invert,
    const SdfPathExpression &pathExpr,
    std::vector<SdfPathExpression::PathPattern> *unmappedPatterns,
    std::vector<SdfPathExpression::ExpressionReference> *unmappedRefs
    ) const
{
    using PathExpr = SdfPathExpression;
    using Op = PathExpr::Op;
    using PathPattern = PathExpr::PathPattern;
    using ExpressionReference = PathExpr::ExpressionReference;
    std::vector<SdfPathExpression> stack;

    auto map = [&](SdfPath const &path) {
        return _MapPathImpl(invert, path);
    };

    auto logic = [&stack](Op op, int argIndex) {
        if (op == PathExpr::Complement) {
            if (argIndex == 1) {
                stack.back() =
                    PathExpr::MakeComplement(std::move(stack.back()));
            }
        }
        else {
            if (argIndex == 2) {
                PathExpr arg2 = std::move(stack.back());
                stack.pop_back();
                stack.back() = PathExpr::MakeOp(
                    op, std::move(stack.back()), std::move(arg2));
            }
        }
    };

    auto mapRef =
        [&stack, &map, &unmappedRefs](ExpressionReference const &ref) {
        if (ref.path.IsEmpty()) {
            // If empty path, retain the reference unchanged.
            stack.push_back(PathExpr::MakeAtom(ref));
        }
        else {
            SdfPath mapped = map(ref.path);
            // This reference is outside the domain, push the Nothing()
            // subexpression.
            if (mapped.IsEmpty()) {
                if (unmappedRefs) {
                    unmappedRefs->push_back(ref);
                }
                stack.push_back(SdfPathExpression::Nothing());
            }
            // Otherwise push the mapped reference.
            else {
                stack.push_back(PathExpr::MakeAtom(
                                    PathExpr::ExpressionReference {
                                        mapped, ref.name }));
            }
        }
    };
    
    auto mapPattern = [&stack, &map,
                       &unmappedPatterns](PathPattern const &pattern) {
        // If the pattern starts with '//' we persist it unchanged, as we deem
        // the intent to be "search everything" regardless of context.  This is
        // as opposed to any kind of non-speculative prefix, which refers to a
        // specific prim or property in the originating context.
        if (pattern.HasLeadingStretch()) {
            stack.push_back(PathExpr::MakeAtom(pattern));
        }
        else {
            SdfPath mapped = map(pattern.GetPrefix());
            // If the prefix path is outside the domain, push the Nothing()
            // subexpression.
            if (mapped.IsEmpty()) {
                if (unmappedPatterns) {
                    unmappedPatterns->push_back(pattern);
                }
                stack.push_back(SdfPathExpression::Nothing());
            }
            // Otherwise push the mapped pattern.
            else {
                PathPattern mappedPattern(pattern);
                mappedPattern.SetPrefix(mapped);
                stack.push_back(PathExpr::MakeAtom(mappedPattern));
            }
        }
    };

    // Walk the expression and map it.
    pathExpr.Walk(logic, mapRef, mapPattern);
    return stack.empty() ? SdfPathExpression {} : stack.back();
}

static _SourceAndTargetPathPairs
_ComposePathPairs(
    const _SourceAndTargetPathPairs& outerPairs,
    const _SourceAndTargetPathPairs& innerPairs)
{
    // A 100k random test subset from a production
    // shot show a mean result size of 1.906050;
    // typically a root identity + other path pair.
    constexpr int NumLocalPairs = 4;

    _PathPair localSpace[NumLocalPairs];
    std::vector<_PathPair> remoteSpace;
    _PathPair *scratchBegin = localSpace;
    int maxRequiredPairs =
        innerPairs.numPairs + int(innerPairs.hasRootIdentity) +
        outerPairs.numPairs + int(outerPairs.hasRootIdentity);
    if (maxRequiredPairs > NumLocalPairs) {
        remoteSpace.resize(maxRequiredPairs);
        scratchBegin = remoteSpace.data();
    }
    _PathPair *scratch = scratchBegin;

    // The composition of this function over inner is the result
    // of first applying inner, then this function.  Build a list
    // of all of the (source,target) path pairs that result.

    // The composed result will have the root identity if and only if both
    // the inner and outer functions have the root identity. Add this first 
    // because it'll be first in the sort order.
    if (outerPairs.hasRootIdentity && innerPairs.hasRootIdentity) {
        _PathPair &scratchPair = *scratch++;
        scratchPair.first = SdfPath::AbsoluteRootPath();
        scratchPair.second = SdfPath::AbsoluteRootPath();
    }

    // Then apply outer function to the output range of inner. 
    for (const _PathPair &pair: innerPairs) {
        _PathPair &scratchPair = *scratch++;
        scratchPair.first = pair.first;
        scratchPair.second = _Map(pair.second, outerPairs, /* invert */ false);
    }

    // This contents of scratch, as of now, will have been automatically sorted
    // as the inner function's source paths were already unique in the correct
    // order. The entries we add after this will not be sorted so we need to
    // keep track of the sorted range.
    _PathPair *scratchSortedEnd = scratch;

    // Then apply the inverse of inner to the domain of this function.
    for (const _PathPair &pair: outerPairs) {
        SdfPath source = _Map(pair.first, innerPairs, /* invert */ true);
        if (source.IsEmpty()) {
            continue;
        }

        // See if this entry's source was already added when we mapped the inner 
        // function's range through the outer so that we don't add it again. We
        // do NOT have to check for repeats within the sources we've mapped
        // through this loop as the map function is guaranteed to not map 
        // different targets to the same source.
        if (std::binary_search(scratchBegin, scratchSortedEnd, source, 
                _PathPairOrderSourceLowerBound())) {
            continue;
        }
        
        _PathPair &scratchPair = *scratch++;
        scratchPair.first = std::move(source);
        scratchPair.second = pair.second;
    }

    // The scratch may be at least partially sorted. Finish the final sorting
    // of the entire range if necessary as it must be fully sorted before 
    // we call _Canonicalize.
    if (scratchSortedEnd != scratch) {
        std::sort(scratchSortedEnd, scratch, _PathPairOrder());
        std::inplace_merge(scratchBegin, scratchSortedEnd, scratch, 
                           _PathPairOrder());
    }

    bool hasRootIdentity = _Canonicalize(scratchBegin, scratch);
    return _SourceAndTargetPathPairs(scratchBegin, scratch, hasRootIdentity);
}

PcpMapFunction
PcpMapFunction::Compose(const PcpMapFunction &inner) const
{
    TfAutoMallocTag tag("Pcp", "PcpMapFunction::Compose");
    TRACE_FUNCTION();

    // Fast path identities.  These do occur in practice and are
    // worth special-casing since it lets us avoid heap allocation.
    if (IsIdentity())
        return inner;
    if (inner.IsIdentity())
        return *this;

    _Mappings::SharedPtr composedMappings;

    const auto canComposePairs = [](const PcpMapFunction& fn) {
        return fn._mappings->IsPathPairs();
    };

    if (canComposePairs(*this) && canComposePairs(inner)) {
        composedMappings = std::make_shared<_Mappings>(
            _ComposePathPairs(
                _mappings->GetPathPairs(), inner._mappings->GetPathPairs()));
    }
    else {
        composedMappings = std::make_shared<_Mappings>(
            _Mappings::SharedPtrVector{_mappings, inner._mappings});
    }

    return PcpMapFunction(
        std::move(composedMappings), _offset * inner._offset);
}

PcpMapFunction
PcpMapFunction::ComposeOffset(const SdfLayerOffset &offset) const
{
    PcpMapFunction composed(*this);
    composed._offset = composed._offset * offset;
    return composed;
}

static _SourceAndTargetPathPairs
_InvertPathPairs(const _SourceAndTargetPathPairs& pathPairs)
{
    _PathPairVector targetToSource;
    targetToSource.reserve(pathPairs.numPairs);
    for (_PathPair const &pair: pathPairs) {
        targetToSource.emplace_back(pair.second, pair.first);
    }
    _PathPair const
        *begin = targetToSource.data(),
        *end = targetToSource.data() + targetToSource.size();
    // Sort the entries to match the source path ordering requirement.
    std::sort(targetToSource.begin(), targetToSource.end(), _PathPairOrder());
    return _SourceAndTargetPathPairs(begin, end, pathPairs.hasRootIdentity);
}

PcpMapFunction
PcpMapFunction::GetInverse() const
{
    TfAutoMallocTag tag("Pcp", "PcpMapFunction::GetInverse");
    TRACE_FUNCTION();

    _Mappings::SharedPtr inverseMappings;

    if (_mappings->IsPathPairs()) {
        inverseMappings = std::make_shared<_Mappings>(
            _InvertPathPairs(_mappings->GetPathPairs()));
    }
    else {
        // List of mappings to visit from last to first.
        // This list contains nullptrs to separate groups of mappings.
        std::vector<const _Mappings*> toVisit(1, _mappings.get());

        // Workspaces for groups of _Mappings. Each entry in the workspace
        // corresponds to a group of mappings in toVisit.
        std::vector<_Mappings::SharedPtrVector> workspace(1);

        auto getInverse = TfOverloads {
            [&workspace](const _SourceAndTargetPathPairs& pathPairs) {
                // Push inverse into workspace for current group.
                workspace.back().push_back(std::make_shared<_Mappings>(
                    _InvertPathPairs(pathPairs)));
            },
            [&workspace, &toVisit](const _Mappings::SharedPtrVector& maps) {
                // Start a new group.
                toVisit.push_back(nullptr);
                for (const _Mappings::SharedPtr& m : maps) {
                    toVisit.push_back(m.get());
                }
                workspace.push_back(_Mappings::SharedPtrVector());
            },
        };

        while (!toVisit.empty()) {
            const _Mappings* m = toVisit.back();
            toVisit.pop_back();

            if (m) {
                std::visit(getInverse, m->data);
            }
            else {
                // We've finished inverting all of the mappings in this group.
                // Consolidate the mappings in the group's workspace and push
                // it into the parent's workspace.
                TF_DEV_AXIOM(!workspace.back().empty());
                _Mappings::SharedPtr invMaps = std::make_shared<_Mappings>(
                    std::move(workspace.back()));
                workspace.pop_back();

                TF_DEV_AXIOM(!workspace.empty());
                workspace.back().push_back(std::move(invMaps));
            }
        }

        // At this point, we should have a single entry in the workspace;
        // that entry should have only one _Mappings object which is the
        // inverse of _mappings.
        TF_AXIOM(workspace.size() == 1);
        TF_AXIOM(workspace.back().size() == 1);
        inverseMappings = std::move(workspace.back().back());
    }

    return PcpMapFunction(std::move(inverseMappings), _offset.GetInverse());
}

PcpMapFunction
PcpMapFunction::_GetNormalized() const
{
    TRACE_FUNCTION();

    if (_mappings->IsPathPairs()) {
        return *this;
    }

    // List of mappings to visit from last to first.
    std::vector<const _Mappings*> toVisit(1, _mappings.get());

    std::optional<_SourceAndTargetPathPairs> normalized;

    auto getNormalized = TfOverloads {
        [&normalized](const _SourceAndTargetPathPairs& pathPairs) {
            normalized = normalized.has_value() ?
                _ComposePathPairs(pathPairs, *normalized) : pathPairs;
        },
        [&toVisit](const _Mappings::SharedPtrVector& mappings) {
            for (const _Mappings::SharedPtr& m : mappings) {
                toVisit.push_back(m.get());
            }
        }
    };

    while (!toVisit.empty()) {
        const _Mappings* m = toVisit.back();
        toVisit.pop_back();
        std::visit(getNormalized, m->data);
    }

    TF_DEV_AXIOM(normalized.has_value());
    return PcpMapFunction(
        std::make_shared<_Mappings>(std::move(normalized.value())), _offset);
}

size_t
PcpMapFunction::_GetNumMappingSets() const
{
    // For convenience, use AllOf to count the path pairs in _mappings.
    size_t result = 0;
    _mappings->AllOf(
        [&result](const _SourceAndTargetPathPairs& pathPairs) {
            ++result;
            return true;
        });

    return result;
}

PcpMapFunction::PathMap
PcpMapFunction::GetSourceToTargetMap() const
{
    const PcpMapFunction normalized = _GetNormalized();
    const _SourceAndTargetPathPairs& mappings =
        normalized._mappings->GetPathPairs();

    PathMap ret(mappings.begin(), mappings.end());
    if (mappings.hasRootIdentity) {
        ret[SdfPath::AbsoluteRootPath()] = SdfPath::AbsoluteRootPath();
    }
    return ret;
}

std::string 
PcpMapFunction::GetString() const
{
    std::vector<std::string> lines;

    if (!GetTimeOffset().IsIdentity()) {
        lines.push_back(TfStringify(GetTimeOffset()));
    }

    PathMap sourceToTargetMap = GetSourceToTargetMap();
    std::map<SdfPath, SdfPath> sortedMap(sourceToTargetMap.begin(),
                                         sourceToTargetMap.end());
    TF_FOR_ALL(it, sortedMap) {
        lines.push_back(TfStringPrintf("%s -> %s", 
                                       it->first.GetText(),
                                       it->second.GetText()));
    }

    return TfStringJoin(lines.begin(), lines.end(), "\n");
}

size_t
PcpMapFunction::Hash() const
{
    // Compute the hash based on the composed path mappings in the
    // normalized map function to match operator==.
    const PcpMapFunction normalized = _GetNormalized();
    return TfHash::Combine(
        normalized._mappings->GetPathPairs(), normalized._offset);
}

PXR_NAMESPACE_CLOSE_SCOPE
