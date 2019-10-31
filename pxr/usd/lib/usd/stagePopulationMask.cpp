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
#include "pxr/usd/usd/stagePopulationMask.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/ostreamMethods.h"

#include <boost/functional/hash.hpp>

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

UsdStagePopulationMask::UsdStagePopulationMask(std::vector<SdfPath> &&paths)
    : _paths(std::move(paths))
{
    _ValidateAndNormalize();
}

UsdStagePopulationMask
UsdStagePopulationMask::Union(UsdStagePopulationMask const &l,
                              UsdStagePopulationMask const &r)
{
    UsdStagePopulationMask result;

    result._paths.reserve(std::min(l._paths.size(), r._paths.size()));

    auto lcur = l._paths.begin(), lend = l._paths.end();
    auto rcur = r._paths.begin(), rend = r._paths.end();

    // Step through both lists in order, merging as we go, and removing paths
    // prefixed by others.
    while (lcur != lend && rcur != rend) {
        if (rcur->HasPrefix(*lcur)) {
            result._paths.push_back(*lcur);
            do { ++rcur; } while (rcur != rend && rcur->HasPrefix(*lcur));
            ++lcur;
        }
        else if (lcur->HasPrefix(*rcur)) {
            result._paths.push_back(*rcur);
            do { ++lcur; } while (lcur != lend && lcur->HasPrefix(*rcur));
            ++rcur;
        }
        else {
            if (*lcur < *rcur) {
                result._paths.push_back(*lcur++);
            }
            else {
                result._paths.push_back(*rcur++);
            }
        }
    }

    // Append any remaining tail elements.
    if (lcur != lend)
        result._paths.insert(result._paths.end(), lcur, lend);
    if (rcur != rend)
        result._paths.insert(result._paths.end(), rcur, rend);

    return result;
}

UsdStagePopulationMask
UsdStagePopulationMask::GetUnion(UsdStagePopulationMask const &other) const
{
    return Union(*this, other);
}

UsdStagePopulationMask
UsdStagePopulationMask::GetUnion(SdfPath const &path) const
{
    // This could be made faster if need-be.
    if (!path.IsAbsolutePath() || !path.IsAbsoluteRootOrPrimPath()) {
        TF_CODING_ERROR("Invalid path <%s>; must be an absolute prim path or "
                        "the absolute root path", path.GetText());
    }
    UsdStagePopulationMask other;
    other._paths.push_back(path);
    return Union(*this, other);
}

UsdStagePopulationMask
UsdStagePopulationMask::Intersection(UsdStagePopulationMask const &l,
                                     UsdStagePopulationMask const &r)
{
    UsdStagePopulationMask result;

    result._paths.reserve(std::min(l._paths.size(), r._paths.size()));

    auto lcur = l._paths.begin(), lend = l._paths.end();
    auto rcur = r._paths.begin(), rend = r._paths.end();

    // Step through both lists in order, merging as we go, and including paths
    // prefixed by others.
    while (lcur != lend && rcur != rend) {
        if (rcur->HasPrefix(*lcur)) {
            do { result._paths.push_back(*rcur++); }
            while (rcur != rend && rcur->HasPrefix(*lcur));
            ++lcur;
        }
        else if (lcur->HasPrefix(*rcur)) {
            do { result._paths.push_back(*lcur++); }
            while (lcur != lend && lcur->HasPrefix(*rcur));
            ++rcur;
        }
        else {
            if (*lcur < *rcur) {
                ++lcur;
            }
            else {
                ++rcur;
            }
        }
    }

    return result;
}

UsdStagePopulationMask
UsdStagePopulationMask
::GetIntersection(UsdStagePopulationMask const &other) const
{
    return Intersection(*this, other);
}

bool
UsdStagePopulationMask::Includes(UsdStagePopulationMask const &other) const
{
    // This could be made faster if need-be.
    return GetUnion(other) == *this;
}

bool
UsdStagePopulationMask::Includes(SdfPath const &path) const
{
    if (_paths.empty())
        return false;
    
    // If this path is in _paths, or if this path prefixes elements of _paths,
    // or if an element _paths prefixes path, it's included.
    auto iter = lower_bound(_paths.begin(), _paths.end(), path);

    SdfPath const *prev = iter == _paths.begin() ? nullptr : &iter[-1];
    SdfPath const *cur = iter == _paths.end() ? nullptr : &iter[0];

    return (prev && path.HasPrefix(*prev)) || (cur && cur->HasPrefix(path));
}

namespace
{
// Return pair where the first element is true if the mask represented by
// paths includes the subtree rooted at path, false otherwise. The second
// element is the result of calling lower_bound on paths with path.
std::pair<bool, std::vector<SdfPath>::const_iterator>
_IncludesSubtree(std::vector<SdfPath> const& paths, SdfPath const& path)
{
    if (paths.empty())
        return { false, paths.end() };

    // If this path is in paths, or if an element in paths prefixes path, then
    // the subtree rooted at path is included.
    auto iter = lower_bound(paths.begin(), paths.end(), path);

    SdfPath const *prev = iter == paths.begin() ? nullptr : &iter[-1];
    SdfPath const *cur = iter == paths.end() ? nullptr : &iter[0];

    return { (cur && *cur == path) || (prev && path.HasPrefix(*prev)), iter };
}
}

bool
UsdStagePopulationMask::IncludesSubtree(SdfPath const &path) const
{
    return _IncludesSubtree(_paths, path).first;
}    

namespace
{
// Return the name of the child prim that appears in \p fullPath
// immediately after the prefix \p path.
TfToken 
_GetChildNameBeneathPath(SdfPath const& fullPath, SdfPath const& path)
{
    for (SdfPath p = fullPath; !p.IsEmpty(); p = p.GetParentPath()) {
        if (p.GetParentPath() == path) {
            return p.GetNameToken();
        }
    }
    return TfToken();
}
}

bool 
UsdStagePopulationMask::GetIncludedChildNames(SdfPath const &path, 
                                              std::vector<TfToken> *names) const
{
    names->clear();

    auto includesSubtree = _IncludesSubtree(_paths, path);
    if (includesSubtree.first)
        return true;

    for (auto it = includesSubtree.second; 
         it != _paths.end() && it->HasPrefix(path); ++it) {

        const SdfPath& maskPath = *it;
        const TfToken& childName = _GetChildNameBeneathPath(maskPath, path);
        if (!TF_VERIFY(!childName.IsEmpty())) {
            // Should never happen because all paths in the range are prefixed
            // by path, and if path was in the range then the earlier call to
            // _IncludesSubtree would have returned true.
            continue;
        }

        // Because the range is sorted, we only need to check the last
        // element to see if childName has been added already.
        if (names->empty() || names->back() != childName) {
            names->push_back(childName);
        }
    }

    return !names->empty();
}

std::vector<SdfPath>
UsdStagePopulationMask::GetPaths() const
{
    return _paths;
}

void
UsdStagePopulationMask::_ValidateAndNormalize()
{
    // Check that all paths are valid.
    for (auto const &path: _paths) {
        if (!path.IsAbsolutePath() || !path.IsAbsoluteRootOrPrimPath()) {
            TF_CODING_ERROR("Invalid path <%s>; must be an absolute prim path "
                            "or the absolute root path", path.GetText());
            return;
        }
    }
    SdfPath::RemoveDescendentPaths(&_paths);
}

std::ostream &
operator<<(std::ostream &os, UsdStagePopulationMask const &mask)
{
    return os << "UsdStagePopulationMask(" << mask.GetPaths() << ')';
}

size_t
hash_value(UsdStagePopulationMask const &mask)
{
    boost::hash<std::vector<SdfPath> > h;
    return h(mask._paths);
}

PXR_NAMESPACE_CLOSE_SCOPE

