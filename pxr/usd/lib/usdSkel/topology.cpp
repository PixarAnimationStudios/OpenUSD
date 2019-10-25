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

        const bool isAbsPath = path.IsAbsolutePath();

        // XXX: A topology is typically constructed using relative
        // paths, but we make this work regardless.
        const SdfPath& end = !isAbsPath ?
            SdfPath::ReflexiveRelativePath() : SdfPath::AbsoluteRootPath();


        // Avoid infinite loops if given paths like '.', '..', etc.
        // TODO: SdfPath should provide a method that allows safe ancestor
        // traversal without risk of introducing infinite loops.
        if (path == end ||
            (!isAbsPath && path.GetName() == SdfPathTokens->parentPathElement)) {
            return -1;
        }

        // Recurse over all parent paths, not just the direct parent.
        // For instance, if the map includes only paths 'a' and 'a/b/c',
        // 'a' will be treated as the parent of 'a/b/c'.
        for (SdfPath p = path.GetParentPath();
             p != end; p = p.GetParentPath()) {

            const auto it = pathMap.find(p);
            if (it != pathMap.end()) {
                return it->second;
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
