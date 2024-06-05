//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/topology.h"

#include "pxr/base/trace/trace.h"

#include <unordered_map>


PXR_NAMESPACE_OPEN_SCOPE

namespace {


using _PathIndexMap = std::unordered_map<SdfPath,int,SdfPath::Hash>;


int
_GetParentIndex(const _PathIndexMap& pathMap, const SdfPath& path)
{
    if (path.IsPrimPath()) {
        // Recurse over all ancestor paths, not just the direct parent.
        // For instance, if the map includes only paths 'a' and 'a/b/c',
        // 'a' will be treated as the parent of 'a/b/c'.
        const auto range = path.GetAncestorsRange();
        auto it = range.begin();
        for (++it; it != range.end(); ++it) {
            const auto mapIt = pathMap.find(*it);
            if (mapIt != pathMap.end()) {
                return mapIt->second;
            }
        }
    }
    return -1;
}


VtIntArray
_ComputeParentIndicesFromPaths(TfSpan<const SdfPath> paths)
{
    TRACE_FUNCTION();

    _PathIndexMap pathMap;
    for (size_t i = 0; i < paths.size(); ++i) {
        pathMap[paths[i]] = static_cast<int>(i);
    }

    VtIntArray parentIndices;
    parentIndices.assign(paths.size(), -1);
    
    const auto parentIndicesSpan = TfMakeSpan(parentIndices);
    for (size_t i = 0; i < paths.size(); ++i) {
        parentIndicesSpan[i] = _GetParentIndex(pathMap, paths[i]);
    }
    return parentIndices;
}


VtIntArray
_ComputeParentIndicesFromTokens(TfSpan<const TfToken> tokens)
{
    // Convert tokens to paths.
    SdfPathVector paths(tokens.size());
    for (size_t i = 0; i < tokens.size(); ++i) {
        paths[i] = SdfPath(tokens[i].GetString());
    }
    return _ComputeParentIndicesFromPaths(paths);
}


} // namespace


/// TODO: It's convenient to provide this constructor, but
/// do we require any common methods to handle the token->path
/// conversion?
UsdSkelTopology::UsdSkelTopology(TfSpan<const TfToken> paths)
    : UsdSkelTopology(_ComputeParentIndicesFromTokens(paths))
{}


UsdSkelTopology::UsdSkelTopology(TfSpan<const SdfPath> paths)
    : UsdSkelTopology(_ComputeParentIndicesFromPaths(paths))
{}


UsdSkelTopology::UsdSkelTopology(const VtIntArray& parentIndices)
    : _parentIndices(parentIndices)
{}


bool
UsdSkelTopology::Validate(std::string* reason) const
{
    TRACE_FUNCTION();

    for (size_t i = 0; i < size(); ++i) {
        const int parent = _parentIndices[i];
        if (parent >= 0) {   
            if (ARCH_UNLIKELY(static_cast<size_t>(parent) >= i)) {
                if (static_cast<size_t>(parent) == i) {
                    if (reason) {
                        *reason = TfStringPrintf(
                            "Joint %zu has itself as its parent.", i);
                    }
                    return false;
                }

                if (reason) {
                    *reason = TfStringPrintf(
                        "Joint %zu has mis-ordered parent %d. Joints are "
                        "expected to be ordered with parent joints always "
                        "coming before children.", i, parent);

                    // XXX: Note that this ordering restriction is a schema
                    // requirement primarily because it simplifies hierarchy
                    // evaluation (see UsdSkelConcatJointTransforms)
                    // But a nice side effect for validation purposes is that
                    // it also ensures that topology is non-cyclic.
                }
                return false;
            }
        }
    }
    return true;
}


bool
UsdSkelTopology::operator==(const UsdSkelTopology& o) const
{
    return _parentIndices == o._parentIndices;
}


PXR_NAMESPACE_CLOSE_SCOPE
