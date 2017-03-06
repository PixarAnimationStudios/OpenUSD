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
#ifndef USD_STAGEPOPULATIONMASK_H
#define USD_STAGEPOPULATIONMASK_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/sdf/path.h"

#include <iosfwd>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdStagePopulationMask
///
/// This class represents a mask that may be applied to a UsdStage to limit the
/// set of UsdPrim s it populates.  This is useful in cases where clients have a
/// large scene but only wish to view or query a single or a handful of objects.
/// For example, suppose we have a city block with buildings, cars, crowds of
/// people, and a couple of main characters.  Some tasks might only require
/// looking at a single main character and perhaps a few props.  We can create a
/// population mask with the paths to the character and props of interest and
/// open a UsdStage with that mask.  Usd will avoid populating the other objects
/// in the scene, saving time and memory.  See UsdStage::OpenMasked() for more.
///
/// A mask is defined by a set of SdfPath s with the following qualities: they
/// are absolute prim paths (or the absolute root path), and no path in the set
/// is an ancestor path of any other path in the set other than itself.  For
/// example, the set of paths ['/a/b', '/a/c', '/x/y'] is a valid mask, but the
/// set of paths ['/a/b', '/a/b/c', '/x/y'] is redundant, since '/a/b' is an
/// ancestor of '/a/b/c'.  The path '/a/b/c' may be removed.  The GetUnion() and
/// Add() methods ensure that no redundant paths are added.
///
/// Default-constructed UsdStagePopulationMask s are considered empty
/// (IsEmpty()) and include no paths.  A population mask containing
/// SdfPath::AbsoluteRootPath() includes all paths.
class UsdStagePopulationMask
{
public:
    /// Return a mask that includes all paths.  This is the mask that contains
    /// the absolute root path.
    static UsdStagePopulationMask
    All() {
        return UsdStagePopulationMask().Add(SdfPath::AbsoluteRootPath());
    }

    /// Construct an empty mask that includes no paths.
    UsdStagePopulationMask() = default;

    UsdStagePopulationMask(UsdStagePopulationMask const &) = default;
    UsdStagePopulationMask(UsdStagePopulationMask &&) = default;
    UsdStagePopulationMask &operator=(UsdStagePopulationMask const &) = default;
    UsdStagePopulationMask &operator=(UsdStagePopulationMask &&) = default;

    /// Construct a mask from the range of paths [f, l).  All paths in the range
    /// must be absolute prim paths or the absolute root path. (See
    /// SdfPath::IsAbsolutePath, SdfPath::IsAbsoluteRootOrPrimPath).
    template <class Iter>
    explicit UsdStagePopulationMask(Iter f, Iter l) : _paths(f, l) {
        _ValidateAndNormalize();
    }

    /// Construct a mask from \p paths.  All paths must be absolute prim paths
    /// or the absolute root path.  (See SdfPath::IsAbsolutePath,
    /// SdfPath::IsAbsoluteRootOrPrimPath).
    explicit UsdStagePopulationMask(std::vector<SdfPath> const &paths) :
        UsdStagePopulationMask(std::vector<SdfPath>(paths)) {};

    /// Construct a mask from \p paths.  All paths must be absolute prim paths
    /// or the absolute root path.  (See SdfPath::IsAbsolutePath,
    /// SdfPath::IsAbsoluteRootOrPrimPath).
    USD_API
    explicit UsdStagePopulationMask(std::vector<SdfPath> &&paths);
    
    /// Return a mask that is the union of \p l and \p r.
    USD_API
    static UsdStagePopulationMask
    Union(UsdStagePopulationMask const &l, UsdStagePopulationMask const &r);

    /// Return a mask that is the union of this and \p other.
    USD_API
    UsdStagePopulationMask GetUnion(UsdStagePopulationMask const &other) const;

    /// Return a mask that is the union of this and a mask containing the single
    /// \p path.
    USD_API
    UsdStagePopulationMask GetUnion(SdfPath const &path) const;
 
    /// Return a mask that is the intersection of \p l and \p r.
    USD_API
    static UsdStagePopulationMask
    Intersection(UsdStagePopulationMask const &l,
                 UsdStagePopulationMask const &r);

    /// Return a mask that is the intersection of this and \p other.
    USD_API
    UsdStagePopulationMask
    GetIntersection(UsdStagePopulationMask const &other) const;

    /// Return true if this mask is a superset of \p other.  That is, if this
    /// mask includes at least every path that \p other includes.
    USD_API
    bool Includes(UsdStagePopulationMask const &other) const;
    
    /// Return true if this mask includes \p path.  This is true if \p path is
    /// one of the paths in this mask, or if it is either a descendant or an
    /// ancestor of one of the paths in this mask.
    USD_API
    bool Includes(SdfPath const &path) const;

    /// Return true if this mask includes \p path and all paths descendant to
    /// \p path.  For example, consider a mask containing the path '/a/b'.
    /// Then the following holds:
    ///
    /// \code
    /// mask.Includes('/a') -> true
    /// mask.Includes('/a/b') -> true
    /// mask.IncludesSubtree('/a') -> false
    /// mask.IncludesSubtree('/a/b') -> true
    /// \endcode
    USD_API
    bool IncludesSubtree(SdfPath const &path) const;

    /// Return true if this mask contains no paths.  Empty masks include no
    /// paths.
    bool IsEmpty() const {
        return _paths.empty();
    }

    /// Return the set of paths that define this mask.
    USD_API
    std::vector<SdfPath> GetPaths() const;

    /// Assign this mask to be its union with \p other and return a reference to
    /// this mask.
    UsdStagePopulationMask &Add(UsdStagePopulationMask const &other) {
        *this = GetUnion(other);
        return *this;
    }
        
    /// Assign this mask to be its union with \p path and return a reference to
    /// this mask.
    UsdStagePopulationMask &Add(SdfPath const &path) {
        *this = GetUnion(path);
        return *this;
    }

    /// Return true if this mask is equivalent to \p other.
    bool operator==(UsdStagePopulationMask const &other) const {
        return _paths == other._paths;
    }

    /// Return true if this mask is not equivalent to \p other.
    bool operator!=(UsdStagePopulationMask const &other) const {
        return !(*this == other);
    }

    /// Swap the content of this mask with \p other.
    void swap(UsdStagePopulationMask &other) {
        _paths.swap(other._paths);
    }

private:
    USD_API void _ValidateAndNormalize();
    
    std::vector<SdfPath> _paths;
};

/// Stream a text representation of a mask.
USD_API
std::ostream &operator<<(std::ostream &, UsdStagePopulationMask const &);

/// Swap the contents of masks \p l and \p r.
inline void swap(UsdStagePopulationMask &l, UsdStagePopulationMask &r)
{
    l.swap(r);
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_STAGEPOPULATIONMASK_H
