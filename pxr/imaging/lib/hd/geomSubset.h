//
// Copyright 2018 Pixar
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
#ifndef HD_GEOM_SUBSET_H
#define HD_GEOM_SUBSET_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/usd/sdf/path.h"
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
    /// The list of element indices contained in the subset.
    std::vector<int> indices;
};

/// A group of geometry subsets.
typedef std::vector<HdGeomSubset> HdGeomSubsets;

HD_API
bool operator==(const HdGeomSubset& lhs, const HdGeomSubset& rhs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_GEOM_SUBSET_H
