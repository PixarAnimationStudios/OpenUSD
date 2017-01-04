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
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/staticData.h"
#include <limits>

namespace {
    // Order PathPairs using FastLessThan.
    struct _PathPairOrder
    {
        bool operator()(const PcpMapFunction::PathPair &lhs,
                        const PcpMapFunction::PathPair &rhs) {
            if (SdfPath::FastLessThan()(lhs.first, rhs.first)) {
                return true;
            }
            if (SdfPath::FastLessThan()(rhs.first, lhs.first)) {
                return false;
            }
            if (SdfPath::FastLessThan()(lhs.second, rhs.second)) {
                return true;
            }
            return false;
        }
    };
};

PcpMapFunction::PcpMapFunction()
{
}

PcpMapFunction::~PcpMapFunction()
{
}

PcpMapFunction::PcpMapFunction(const PathPairVector &sourceToTarget,
                               SdfLayerOffset offset)
    : _data( _Data(sourceToTarget, offset) )
{
    // No malloc tags here; we've already provided tag coverage
    // in the public API.
}

PcpMapFunction::PcpMapFunction(const _DataFlyweight &data)
    : _data(data)
{
    // No malloc tags here; we're sharing the flyweight instance.
}

PcpMapFunction::PcpMapFunction(const PcpMapFunction &map)
    : _data(map._data)
{
    // No malloc tags here; we're sharing the flyweight instance.
}

// Canonicalize a PathMap by removing all redundant entries.
// Redundant entries are those which can be removed without
// changing the semantics of the correspondence.
static void
_Canonicalize(PcpMapFunction::PathPairVector *vec)
{
    TRACE_FUNCTION();

    for (PcpMapFunction::PathPairVector::iterator i = vec->begin();
         i != vec->end(); /* increment below */)
    {
        bool redundant = false;

        // Check for trivial dupes before doing further work.
        for (PcpMapFunction::PathPairVector::iterator j = vec->begin();
             j != i; ++j) {
            if (*i == *j) {
                redundant = true;
                break;
            }
        }

        // Find the closest enclosing mapping.  If the trailing name
        // components do not match, this pair cannot be redundant.
        if (!redundant &&
            i->first.GetNameToken() == i->second.GetNameToken()) {
            // The tail component matches.  Walk up the prefixes.
            for (SdfPath source = i->first, target = i->second;
                 !source.IsEmpty() && !target.IsEmpty()
                 && !redundant;
                 source = source.GetParentPath(),
                 target = target.GetParentPath())
            {
                // Check for a redundant mapping.
                for (PcpMapFunction::PathPairVector::iterator j = vec->begin();
                     j != vec->end(); ++j)
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
            std::swap(*i, vec->back());
            vec->erase(vec->end()-1);
        } else {
            ++i;
        }
    }

    // Final sort to canonical order.
    std::sort(vec->begin(), vec->end(), _PathPairOrder());
}

PcpMapFunction
PcpMapFunction::Create(const PathMap &sourceToTarget,
                       const SdfLayerOffset &offset)
{
    TfAutoMallocTag2 tag("Pcp", "PcpMapFunction");
    TRACE_FUNCTION();

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
    _Canonicalize(&vec);
    return PcpMapFunction(vec, offset);
}    

bool
PcpMapFunction::IsNull() const
{
    return _GetData() == _Data();
}

TF_MAKE_STATIC_DATA(PcpMapFunction, _identityMapFunction)
{
    *_identityMapFunction = PcpMapFunction::Create(
       PcpMapFunction::IdentityPathMap(), SdfLayerOffset());
}

const PcpMapFunction &
PcpMapFunction::Identity()
{
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

PcpMapFunction&
PcpMapFunction::operator=(const PcpMapFunction &map)
{
    PcpMapFunction(map).Swap(*this);
    return *this;
}

void
PcpMapFunction::Swap(PcpMapFunction& map)
{
    _data.Swap(map._data);
}

bool
PcpMapFunction::operator==(const PcpMapFunction &map) const
{
    return _data == map._data;
}

bool
PcpMapFunction::operator!=(const PcpMapFunction &map) const
{
    return !(*this == map);
}

static SdfPath
_Map(const SdfPath& path,
     const PcpMapFunction::PathPair *pairs,
     const unsigned short numPairs,
     bool invert)
{
    // Note that we explicitly do not fix target paths here. This
    // is for consistency, so that consumers can be certain of
    // PcpMapFunction's behavior. If consumers want target paths
    // to be fixed, they must be certain to recurse on target paths
    // themselves.
    //
    // XXX: It may be preferrable to have PcpMapFunction be in charge
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
    if (bestIndex == -1) {
        // No mapping found.
        return SdfPath();
    }
    const SdfPath &source =
        invert? pairs[bestIndex].second : pairs[bestIndex].first;
    const SdfPath &target =
        invert? pairs[bestIndex].first : pairs[bestIndex].second;
    SdfPath result =
        path.ReplacePrefix(source, target, /* fixTargetPaths = */ false);
    if (result.IsEmpty()) {
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
    const _Data& data = _GetData();
    return _Map(path, data._pairs, data._numPairs, /* invert */ false);
}

SdfPath
PcpMapFunction::MapTargetToSource(const SdfPath & path) const
{
    const _Data& data = _GetData();
    return _Map(path, data._pairs, data._numPairs, /* invert */ true);
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

    // The composition of this function over inner is the result
    // of first applying inner, then this function.  Build a list
    // of all of the (source,target) path pairs that result.
    std::vector<PathPair> pairs;

    // A 100k random test subset from a production
    // shot show a mean result size of 1.906050;
    // typically a root identity + other path pair.
    pairs.reserve(2);

    // Apply outer function to the output range of inner.
    const _Data& data_inner = inner._GetData();
    for (int i=0; i < data_inner._numPairs; ++i) {
        PathPair pair = data_inner._pairs[i];
        pair.second = MapSourceToTarget(pair.second);
        if (!pair.second.IsEmpty()) {
            if (std::find(pairs.begin(), pairs.end(), pair) == pairs.end()) {
                pairs.push_back(pair);
            }
        }
    }

    // Apply the inverse of inner to the domain of this function.
    const _Data& data_outer = _GetData();
    for (int i=0; i < data_outer._numPairs; ++i) {
        PathPair pair = data_outer._pairs[i];
        pair.first = inner.MapTargetToSource(pair.first);
        if (!pair.first.IsEmpty()) {
            if (std::find(pairs.begin(), pairs.end(), pair) == pairs.end()) {
                pairs.push_back(pair);
            }
        }
    }

    _Canonicalize(&pairs);

    return PcpMapFunction(pairs, data_outer._offset * data_inner._offset);
}

PcpMapFunction
PcpMapFunction::GetInverse() const
{
    TfAutoMallocTag2 tag("Pcp", "PcpMapFunction");

    PathPairVector targetToSource;
    const _Data& data = _GetData();
    for (int i=0; i < data._numPairs; ++i) {
        targetToSource.push_back( PathPair(data._pairs[i].second,
                                           data._pairs[i].first) );
    }
    return PcpMapFunction(targetToSource, data._offset.GetInverse());
}

PcpMapFunction::PathMap
PcpMapFunction::GetSourceToTargetMap() const
{
    const _Data& d = _GetData();
    return PathMap( &d._pairs[0], &d._pairs[d._numPairs] );
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

PcpMapFunction::_Data::_Data(const PathPairVector &sourceToTarget,
                             const SdfLayerOffset &offset)
    : _offset(offset),
    _pairs(new PathPair[sourceToTarget.size()]),
    _numPairs(sourceToTarget.size())
{
    for (int i=0; i < _numPairs; ++i) {
        _pairs[i] = sourceToTarget[i];
    }
}

PcpMapFunction::_Data::_Data(const _Data &d)
{
    _offset = d._offset;
    _numPairs = d._numPairs;
    _pairs = new PathPair[_numPairs];
    for (int i=0; i < _numPairs; ++i) {
        _pairs[i] = d._pairs[i];
    }
}

PcpMapFunction::_Data&
PcpMapFunction::_Data::operator=(const _Data &d)
{
    if (this != &d) {
        _offset = d._offset;
        _numPairs = d._numPairs;
        delete []_pairs;
        _pairs = new PathPair[_numPairs];
        for (int i=0; i < _numPairs; ++i) {
            _pairs[i] = d._pairs[i];
        }
    }
    return *this;
}

PcpMapFunction::_Data::~_Data()
{
    delete []_pairs;
    _pairs = 0;
    _numPairs = 0;
}

bool
PcpMapFunction::_Data::operator==(const _Data &rhs) const {
    if (_numPairs != rhs._numPairs || _offset != rhs._offset) {
        return false;
    }
    for (int i=0; i < _numPairs; ++i) {
        if (_pairs[i] != rhs._pairs[i]) {
            return false;
        }
    }
    return true;
}

size_t
PcpMapFunction::_Data::Hash::operator()(const _Data &data) const
{
    size_t hash = 0;
    boost::hash_combine(hash, data._numPairs);
    for (unsigned short i=0; i < data._numPairs; ++i) {
        boost::hash_combine(hash, data._pairs[i].first.GetHash());
        boost::hash_combine(hash, data._pairs[i].second.GetHash());
    }
    boost::hash_combine(hash, data._offset.GetHash());
    return hash;
}
