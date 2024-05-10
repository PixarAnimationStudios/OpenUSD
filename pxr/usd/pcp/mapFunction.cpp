//
// Copyright 2016 Pixar
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

#include "pxr/pxr.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/ostreamMethods.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

namespace {
    // Order PathPairs using FastLessThan.
    struct _PathPairOrder
    {
        bool operator()(const PcpMapFunction::PathPair &lhs,
                        const PcpMapFunction::PathPair &rhs) {
            SdfPath::FastLessThan less;
            // We need to ensure that "root identity" elements appear first
            // ('/' -> '/') so we special-case those.
            SdfPath const &absRoot = SdfPath::AbsoluteRootPath();
            if (lhs == rhs) {
                return false;
            }
            if (lhs.first == absRoot && lhs.second == absRoot) {
                return true;
            }
            if (rhs.first == absRoot && rhs.second == absRoot) {
                return false;
            }
            return less(lhs.first, rhs.first) ||
                (lhs.first == rhs.first && less(lhs.second, rhs.second));
        }
    };
};

PcpMapFunction::PcpMapFunction(PathPair const *begin,
                               PathPair const *end,
                               SdfLayerOffset offset,
                               bool hasRootIdentity)
    : _data(begin, end, hasRootIdentity)
    , _offset(offset)
{
}

// Finds the map entry whose source best matches the given path, i.e. the entry
// with the longest source path that is a prefix of the path. If invert is true,
// the target path of the entry is used as the "source". minElementCount is used
// to only look for entries where the source path has at least that many 
// elements.
template <class PairIter>
static PairIter
_GetBestSourceMatch(
    const SdfPath &path, const PairIter &begin, const PairIter &end, 
    bool invert = false, size_t minElementCount = 0)
{
    PairIter bestIter = end;
    size_t bestElementCount = minElementCount;
    for (PairIter i=begin; i != end; ++i) {
        const SdfPath &source = invert? i->second : i->first;
        const size_t count = source.GetPathElementCount();
        if (count >= bestElementCount && path.HasPrefix(source)) {
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
    PairIter bestTargetMatch = _GetBestSourceMatch(targetPath, begin, end, 
        !invert, 
        minElementCount);
    return bestTargetMatch != end && bestTargetMatch != bestSourceMatch;
}

template <class PairIter>
static bool
_IsRedundant(const PairIter &entry, const PairIter &begin, const PairIter &end)
{
    const SdfPath &entrySource = entry->first;
    const SdfPath &entryTarget = entry->second;

    const bool isBlock = entryTarget.IsEmpty();

    // Check for trivial dupes before doing further work.
    for (PairIter j = begin; j != entry; ++j) {
        if (entry->first == j->first) {
            TF_CODING_ERROR("Map function has two entries with the "
                "same source");
            return true;
        }

        if (!isBlock && entry->second == j->second) {
            TF_CODING_ERROR("Map function has two entries with the "
                "same target");
            return true;
        }
    }

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
    // in the best matching ancestor mapping would cause the source path to 
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
// '/').  It will not appear in the resulting \p vec.
template <class PairIter>
static bool
_Canonicalize(PairIter &begin, PairIter &end)
{
    TRACE_FUNCTION();

    for (PairIter i = begin; i != end; /* increment below */) {

        if (_IsRedundant(i, begin, end)) {
            // Entries are not sorted yet so swap to back for O(1) erase.
            std::swap(*i, *(end-1));
            --end;
        } else {
            ++i;
        }
    }

    // Final sort to canonical order.
    std::sort(begin, end, _PathPairOrder());

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
    TfAutoMallocTag2 tag("Pcp", "PcpMapFunction");
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
        const _Data::PairCount maxPairCount =
            std::numeric_limits<_Data::PairCount>::max();
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

    PathPairVector vec(sourceToTarget.begin(), sourceToTarget.end());
    PathPair *begin = vec.data(), *end = vec.data() + vec.size();
    bool hasRootIdentity = _Canonicalize(begin, end);
    return PcpMapFunction(begin, end, offset, hasRootIdentity);
}

bool
PcpMapFunction::IsNull() const
{
    return _data.IsNull();
}

PcpMapFunction *Pcp_MakeIdentity()
{
    PcpMapFunction *ret = new PcpMapFunction;
    ret->_data.hasRootIdentity = true;
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
    return _data.numPairs == 0 && _data.hasRootIdentity;
}

void
PcpMapFunction::Swap(PcpMapFunction& map)
{
    using std::swap;
    swap(_data, map._data);
    swap(_offset, map._offset);
}

bool
PcpMapFunction::operator==(const PcpMapFunction &map) const
{
    return _data == map._data && _offset == map._offset;
}

bool
PcpMapFunction::operator!=(const PcpMapFunction &map) const
{
    return !(*this == map);
}

static SdfPath
_Map(const SdfPath& path,
     const PcpMapFunction::PathPair *pairs,
     const int numPairs,
     bool hasRootIdentity,
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

    const PcpMapFunction::PathPair *begin = pairs;
    const PcpMapFunction::PathPair *end = begin + numPairs;
 
    // Find longest prefix that has a mapping;
    // this represents the most-specific mapping to apply.
    const PcpMapFunction::PathPair *bestMatch =
        _GetBestSourceMatch(path, begin, end, invert);

    SdfPath result;
    // size_t minTargetElementCount = 0;
    if (bestMatch == end) {
        if (hasRootIdentity) {
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
PcpMapFunction::MapSourceToTarget(const SdfPath & path) const
{
    return _Map(path, _data.begin(), _data.numPairs, _data.hasRootIdentity,
                /* invert */ false);
}

SdfPath
PcpMapFunction::MapTargetToSource(const SdfPath & path) const
{
    return _Map(path, _data.begin(), _data.numPairs, _data.hasRootIdentity,
                /* invert */ true);
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
        return _Map(path, _data.begin(), _data.numPairs,
                    _data.hasRootIdentity, invert);
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

PcpMapFunction
PcpMapFunction::Compose(const PcpMapFunction &inner) const
{
    TfAutoMallocTag2 tag("Pcp", "PcpMapFunction");
    TRACE_FUNCTION();

    // Fast path identities.  These do occur in practice and are
    // worth special-casing since it lets us avoid heap allocation.
    if (IsIdentity())
        return inner;
    if (inner.IsIdentity())
        return *this;

    // A 100k random test subset from a production
    // shot show a mean result size of 1.906050;
    // typically a root identity + other path pair.
    constexpr int NumLocalPairs = 4;

    PathPair localSpace[NumLocalPairs];
    std::vector<PathPair> remoteSpace;
    PathPair *scratchBegin = localSpace;
    int maxRequiredPairs =
        inner._data.numPairs + int(inner._data.hasRootIdentity) +
        _data.numPairs + int(_data.hasRootIdentity);
    if (maxRequiredPairs > NumLocalPairs) {
        remoteSpace.resize(maxRequiredPairs);
        scratchBegin = remoteSpace.data();
    }
    PathPair *scratch = scratchBegin;

    // The composition of this function over inner is the result
    // of first applying inner, then this function.  Build a list
    // of all of the (source,target) path pairs that result.

    // Apply outer function to the output range of inner.
    const _Data& data_inner = inner._data;
    for (PathPair pair: data_inner) {
        pair.second = MapSourceToTarget(pair.second);
        if (std::find(scratchBegin, scratch, pair) == scratch) {
            *scratch++ = std::move(pair);
        }
    }
    // If inner has a root identity, map that too.
    if (inner.HasRootIdentity()) {
        PathPair pair;
        pair.first = SdfPath::AbsoluteRootPath();
        pair.second = MapSourceToTarget(SdfPath::AbsoluteRootPath());
        if (std::find(scratchBegin, scratch, pair) == scratch) {
            *scratch++ = std::move(pair);
        }
    }

    // Apply the inverse of inner to the domain of this function.
    const _Data& data_outer = _data;
    for (PathPair pair: data_outer) {
        pair.first = inner.MapTargetToSource(pair.first);
        if (!pair.first.IsEmpty()) {
            if (std::find(scratchBegin, scratch, pair) == scratch) {
                *scratch++ = std::move(pair);
            }
        }
    }
    // If outer has a root identity, map that too.
    if (HasRootIdentity()) {
        PathPair pair;
        pair.first = inner.MapTargetToSource(SdfPath::AbsoluteRootPath());
        pair.second = SdfPath::AbsoluteRootPath();
        if (!pair.first.IsEmpty()) {
            if (std::find(scratchBegin, scratch, pair) == scratch) {
                *scratch++ = std::move(pair);
            }
        }
    }

    bool hasRootIdentity = _Canonicalize(scratchBegin, scratch);
    return PcpMapFunction(scratchBegin, scratch,
                          _offset * inner._offset, hasRootIdentity);
}

PcpMapFunction
PcpMapFunction::ComposeOffset(const SdfLayerOffset &offset) const
{
    PcpMapFunction composed(*this);
    composed._offset = composed._offset * offset;
    return composed;
}

PcpMapFunction
PcpMapFunction::GetInverse() const
{
    TfAutoMallocTag2 tag("Pcp", "PcpMapFunction");

    PathPairVector targetToSource;
    targetToSource.reserve(_data.numPairs);
    for (PathPair const &pair: _data) {
        targetToSource.emplace_back(pair.second, pair.first);
    }
    PathPair const
        *begin = targetToSource.data(),
        *end = targetToSource.data() + targetToSource.size();
    return PcpMapFunction(
        begin, end, _offset.GetInverse(), _data.hasRootIdentity);
}

PcpMapFunction::PathMap
PcpMapFunction::GetSourceToTargetMap() const
{
    PathMap ret(_data.begin(), _data.end());
    if (_data.hasRootIdentity) {
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
    return TfHash{}(*this);
}

PXR_NAMESPACE_CLOSE_SCOPE
