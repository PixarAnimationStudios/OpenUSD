//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_COORD_SYS_H
#define PXR_IMAGING_HD_COORD_SYS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/sprim.h"

PXR_NAMESPACE_OPEN_SCOPE

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
class HdCoordSys : public HdSprim
{
public:
    HD_API
    HdCoordSys(SdfPath const & id);
    HD_API
    ~HdCoordSys() override;

    // Change tracking for HdCoordSys
    enum DirtyBits : HdDirtyBits {
        Clean                 = 0,
        DirtyName             = 1 << 0,
        DirtyTransform        = 1 << 1,
        AllDirty              = (DirtyTransform
                                |DirtyName)
    };

    /// Returns the name bound to this coordinate system.
    ///
    /// There may be multiple coordinate systems with the same
    /// name, but they must associate with disjoint sets of rprims.
    TfToken GetName() const { return _name; }

    HD_API
    void Sync(HdSceneDelegate *sceneDelegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits) override;

    HD_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

private:
    TfToken _name;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_COORD_SYS_H
