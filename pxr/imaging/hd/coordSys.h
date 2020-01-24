//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_HD_COORD_SYS_H
#define PXR_IMAGING_HD_COORD_SYS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;

/// \class HdCoordSys
///
/// HdCoordSys representes a coordinate system as a Hydra state prim.
///
/// Coordinate systems may be referred to by name from a shader network.
/// Following the convention in UsdShadeCoordSysAPI, we use the Hydra
/// id to establish the name, where the id is a namespaced property
/// path of the form <tt><.../prim.coordSys:NAME></tt>.  GetName()
/// will retrieve the name.
///
/// Each rprim may have a set of bound coordinate systems, which
/// may be retrieved via the <tt>HdTokens->coordSysBindings</tt> key.
/// The returned value is of type HdIdVectorSharedPtr, a reference-
/// counted pointer to a vector of ids of coordinate systems.
/// The intention of this design is to make it efficient for scene
/// delegates to communicate to renderer delegates the common
/// mappings of bound coordinate systems across groups of rprims.
///
/// The transform value of an HdCoordSys is the matrix representation
/// of the transform from its local space to world space.  In other
/// words, it has the same interpretation as the transform for rprims.
///
class HdCoordSys : public HdSprim {
public:
    HD_API
    HdCoordSys(SdfPath const & id);
    HD_API
    virtual ~HdCoordSys();

    // Change tracking for HdCoordSys
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        DirtyTransform        = 1 << 0,
        AllDirty              = DirtyTransform
    };

    /// Returns the name bound to this coordinate system.
    ///
    /// There may be multiple coordinate systems with the same
    /// name, but they must associate with disjoint sets of rprims.
    TfToken GetName() const { return _name; }

private:
    TfToken _name;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_COORD_SYS_H
