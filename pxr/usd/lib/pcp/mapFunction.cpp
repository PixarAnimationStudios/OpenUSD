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
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/ostreamMethods.h"

#include <boost/functional/hash.hpp>

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
        bool redundant = false;

        // Check for trivial dupes before doing further work.
        for (PairIter j = begin; j != i; ++j) {
            if (*i == *j) {
                redundant = true;
                break;
            }
        }

        // Find the closest enclosing mapping.  If the trailing name
        // components do not match, this pair cannot be redundant.
        if (!redundant && i->first.GetNameToken() == i->second.GetNameToken()) {
            // The tail component matches.  Walk up the prefixes.
            for (SdfPath source = i->first, target = i->second;
                 !source.IsEmpty() && !target.IsEmpty()
                 && !redundant;
                 source = source.GetParentPath(),
                 target = target.GetParentPath())
            {
                // Check for a redundant mapping.
                for (PairIter j = begin; j != end; ++j)
                {
                    // *j makes *i redundant if *j maps source to target.
                    if (i != j && j->first == source && j->second == target) {
                        // Found the closest enclosing mapping, *j.
                        // It means *i is the same as *j plus the addition
                        // of an identical series of path components on both
                        // the source and target sides -- which we verified
                        // as we peeled off trailing path components to get
                        // here.
                        redundant = true;
                        break;
                    }
                }
                if (source.GetNameToken() != target.GetNameToken()) {
                    // The trailing name components do not match,
                    // so this pair cannot be redundant.
                    break;
                }
            }
        }

        if (redundant) {
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
    TF_FOR_ALL(i, sourceToTarget) {
        // Source and target paths must be prim paths, because mappings
        // are used on arcs and arcs are only expressed between prims.
        //
        // They also must not contain variant selections.  Variant
        // selections are purely an aspect of addressing layer opinion
        // storage.  They are *not* an aspect of composed scene namespace.
        //
        // This is a coding error, because a PcpError should have been
        // emitted about these conditions before getting to this point.

        const SdfPath & source = i->first;
        const SdfPath & target = i->second;
        if (!source.IsAbsolutePath() ||
            !(source.IsAbsoluteRootOrPrimPath() ||
              source.IsPrimVariantSelectionPath()) ||
            !target.IsAbsolutePath() ||
            !(target.IsAbsoluteRootOrPrimPath() ||
              target.IsPrimVariantSelectionPath())) {
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
    return *this == Identity();
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

    // Find longest prefix that has a mapping;
    // this represents the most-specific mapping to apply.
    int bestIndex = -1;
    size_t bestElemCount = 0;
    for (int i=0; i < numPairs; ++i) {
        const SdfPath &source = invert? pairs[i].second : pairs[i].first;
        const size_t count = source.GetPathElementCount();
        if (count >= bestElemCount && path.HasPrefix(source)) {
            bestElemCount = count;
            bestIndex = i;
        }
    }
    if (bestIndex == -1 && !hasRootIdentity) {
        // No mapping found.
        return SdfPath();
    }

    SdfPath result;
    const SdfPath &target = bestIndex == -1 ? SdfPath::AbsoluteRootPath() :
        invert? pairs[bestIndex].first : pairs[bestIndex].second;
    if (bestIndex != -1) {
        const SdfPath &source =
            invert? pairs[bestIndex].second : pairs[bestIndex].first;
        result =
            path.ReplacePrefix(source, target, /* fixTargetPaths = */ false);
        if (result.IsEmpty()) {
            return result;
        }
    }
    else {
        // Use the root identity.
        result = path;
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

    // Optimistically assume the same mapping will be the best;
    // we can skip even considering any mapping that is shorter.
    bestElemCount = target.GetPathElementCount();
    for (int i=0; i < numPairs; ++i) {
        if (i == bestIndex) {
            continue;
        }
        const SdfPath &target = invert? pairs[i].first : pairs[i].second;
        const size_t count = target.GetPathElementCount();
        if (count > bestElemCount && result.HasPrefix(target)) {
            // There is a more-specific reverse mapping for this path.
            return SdfPath();
        }
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
        if (!pair.second.IsEmpty()) {
            if (std::find(scratchBegin, scratch, pair) == scratch) {
                *scratch++ = std::move(pair);
            }
        }
    }
    // If inner has a root identity, map that too.
    if (inner.HasRootIdentity()) {
        PathPair pair;
        pair.first = SdfPath::AbsoluteRootPath();
        pair.second = MapSourceToTarget(SdfPath::AbsoluteRootPath());
        if (!pair.second.IsEmpty()) {
            if (std::find(scratchBegin, scratch, pair) == scratch) {
                *scratch++ = std::move(pair);
            }
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
    size_t hash = _data.hasRootIdentity;
    boost::hash_combine(hash, _data.numPairs);
    for (PathPair const &p: _data) {
        boost::hash_combine(hash, p.first.GetHash());
        boost::hash_combine(hash, p.second.GetHash());
    }
    boost::hash_combine(hash, _offset.GetHash());
    return hash;
}

PXR_NAMESPACE_CLOSE_SCOPE
