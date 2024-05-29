//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_GEOM_SUBSET_H
#define PXR_IMAGING_HD_GEOM_SUBSET_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdGeomSubset
///
/// Describes a subset of a piece of geometry as a set of indices.
///
struct HdGeomSubset {
    enum Type {
        /// A subset of faces
        TypeFaceSet,

        // For now, there is only one type, but this may grow in the future
        // to accomodate edges, points, etc.
    };

    /// The type of elements this geometry subset includes.
    Type type;
    /// The path used to identify this subset in the scene.
    SdfPath id;
    /// The path used to identify this material bound to the subset.
    SdfPath materialId;
    /// The list of element indices contained in the subset.
    VtIntArray indices;
};

/// A group of geometry subsets.
typedef std::vector<HdGeomSubset> HdGeomSubsets;

HD_API
bool operator==(const HdGeomSubset& lhs, const HdGeomSubset& rhs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_GEOM_SUBSET_H
