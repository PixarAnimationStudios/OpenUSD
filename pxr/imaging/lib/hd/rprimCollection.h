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
#ifndef HD_RPRIM_COLLECTION_H
#define HD_RPRIM_COLLECTION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/repr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/token.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdRprimCollection
///
/// A named, semantic collection of objects.
///
/// Note that the collection object itself doesn't hold HdRprim objects, rather
/// it acts as an addressing mechanism to identify a specific group of HdRprim 
/// objects that can be requested from the HdRenderIndex.
/// 
/// HdDirtyList provides the above algorithmic functionality, while HdRenderPass
/// uses HdRprimCollection to concisely represent the HdRprim's it operates on.
/// 
/// \sa
/// HdRenderPass
/// HdDirtyList
///
class HdRprimCollection {
public:
    HD_API
    HdRprimCollection();

    /// Constructs an rprim collection with \p reprSelector. \p if forcedRepr is
    /// set to true, prims authored repr will be ignored.
    HD_API
    HdRprimCollection(TfToken const& name,
                      HdReprSelector const& reprSelector,
                      bool forcedRepr=false);

    /// Constructs an rprim collection, excluding all Rprims not prefixed by \p
    /// rootPath. \p if forcedRepr is set to true, prims authored repr will be
    /// ignored.
    HD_API
    HdRprimCollection(TfToken const& name,
                      HdReprSelector const& reprSelector,
                      SdfPath const& rootPath,
                      bool forcedRepr=false);

    /// Copy constructor.
    HD_API
    HdRprimCollection(HdRprimCollection const& col);

    // Destructor.
    HD_API
    virtual ~HdRprimCollection();

    /// Constructs and returns a collection with the root and exclude paths
    /// swapped.
    HD_API
    HdRprimCollection CreateInverseCollection() const;

    /// Returns the semantic name of this collection.
    ///
    /// The semantic name represents the entire collection of prims, for
    /// example "visible", "selected", etc.
    TfToken const& GetName() const {
        return _name;
    }

    /// Sets the semantic name of this collection.
    void SetName(TfToken const& name) {
        _name = name;
    }

    /// Returns the representation name.
    ///
    /// The repr selector corresponds to specific aspects of the requested set
    /// of Rprims, for example one can request "hullAndPoints" repr which
    /// would cause both the hull and points representations of all prims named
    /// by the collection to be included.
    HdReprSelector const& GetReprSelector() const {
        return _reprSelector;
    }

    void SetReprSelector(HdReprSelector const& reprSelector) {
        _reprSelector = reprSelector;
    }

    bool IsForcedRepr() const {
        return _forcedRepr;
    }

    void SetForcedRepr(bool flag) {
        _forcedRepr = flag;
    }

    /// Returns the paths at which all Rprims must be rooted to be included in
    /// this collection. Defaults to a vector containing only
    /// SdfPath::AbsoluteRootPath().
    ///
    /// Note that this vector is always sorted.
    HD_API
    SdfPathVector const& GetRootPaths() const;

    /// Sets all root paths for this collection, replacing any existing paths
    /// that were present previously. All paths must be absolute. Duplicate
    /// paths are allowed, but may result in performance degradation.
    HD_API
    void SetRootPaths(SdfPathVector const& rootPaths);

    /// Sets the path at which all Rprims must be rooted to be included in this
    /// collection, replacing any existing root paths that were previously
    /// specified.
    HD_API
    void SetRootPath(SdfPath const& rootPath);

    /// Sets all exclude paths for this collection. All paths must be absolute. 
    /// Duplicate paths are allowed, but may result in performance degradation.
    HD_API
    void SetExcludePaths(SdfPathVector const& excludePaths);

    /// Returns the excluded paths
    ///
    /// Note that this vector is always sorted.
    HD_API
    SdfPathVector const& GetExcludePaths() const;

    /// Sets the render tags that this collection will render.
    HD_API
    void SetRenderTags(TfTokenVector const& renderTags);

    /// Returns the render tags.
    HD_API
    TfTokenVector const& GetRenderTags() const;

    /// Returns if a tag is used by this collection
    HD_API
    bool HasRenderTag(TfToken const & renderTag) const;

    HD_API
    size_t ComputeHash() const;

    struct Hash {
        size_t operator()(HdRprimCollection const& value) const {
            return value.ComputeHash();
        }
    };

    HD_API
    bool operator==(HdRprimCollection const & lhs) const;
    HD_API
    bool operator!=(HdRprimCollection const & lhs) const;

private:
    HD_API
    friend std::ostream & operator <<(std::ostream &out, 
        HdRprimCollection const & v);

    TfToken _name;
    HdReprSelector _reprSelector;
    bool _forcedRepr;
    SdfPathVector _rootPaths;
    SdfPathVector _excludePaths;
    TfTokenVector _renderTags;
};

typedef std::vector<HdRprimCollection> HdRprimCollectionVector;

// VtValue requirements
HD_API
std::ostream& operator<<(std::ostream& out, HdRprimCollection const & v);

// Overload hash_value for HdRprimCollection.  Used by things like boost::hash.
HD_API
size_t hash_value(HdRprimCollection const &col);


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_RPRIM_COLLECTION_H
