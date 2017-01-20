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
#ifndef HDST_POINTS_H
#define HDST_POINTS_H
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/points.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

#include <boost/shared_ptr.hpp>

/// \class HdStPointsReprDesc
///
/// Descriptor to configure a drawItem for a repr.
///
struct HdStPointsReprDesc {
    HdStPointsReprDesc(
        HdPointsGeomStyle geomStyle = HdPointsGeomStyleInvalid)
        : geomStyle(geomStyle)
        {}

    HdPointsGeomStyle geomStyle:1;
};

/// \class HdStPoints
///
/// Points.
///
class HdStPoints final : public HdPoints {
public:
    HF_MALLOC_TAG_NEW("new HdStPoints");
    HdStPoints(HdSceneDelegate* delegate, SdfPath const& id,
             SdfPath const& instancerId = SdfPath());
    virtual ~HdStPoints();

    /// Configure geometric style of drawItems for \p reprName
    static void ConfigureRepr(TfToken const &reprName,
                              const HdStPointsReprDesc &desc);

protected:
    virtual HdReprSharedPtr const & _GetRepr(
        TfToken const &reprName, HdChangeTracker::DirtyBits *dirtyBitsState);

    void _PopulateVertexPrimVars(HdDrawItem *drawItem,
                                 HdChangeTracker::DirtyBits *dirtyBitsState);

    virtual HdChangeTracker::DirtyBits _GetInitialDirtyBits() const final override;

private:
    enum DrawingCoord {
        InstancePrimVar = HdDrawingCoord::CustomSlotsBegin
    };

    void _UpdateDrawItem(HdDrawItem *drawItem,
                         HdChangeTracker::DirtyBits *dirtyBits);

    typedef _ReprDescConfigs<HdStPointsReprDesc> _PointsReprConfig;
    static _PointsReprConfig _reprDescConfig;
};

#endif // HDST_POINTS_H
