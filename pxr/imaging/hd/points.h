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
#ifndef PXR_IMAGING_HD_POINTS_H
#define PXR_IMAGING_HD_POINTS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdPointsReprDesc
///
/// Descriptor to configure a drawItem for a repr.
///
struct HdPointsReprDesc {
    HdPointsReprDesc(
        HdPointsGeomStyle geomStyle = HdPointsGeomStyleInvalid)
        : geomStyle(geomStyle)
        {}
    
    bool IsEmpty() const {
        return geomStyle == HdPointsGeomStyleInvalid;
    }

    HdPointsGeomStyle geomStyle;
};

/// Hydra Schema for a point cloud.
///
class HdPoints: public HdRprim {
public:
    HD_API
    virtual ~HdPoints();

    HD_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;

    /// Configure geometric style of drawItems for \p reprName
    HD_API
    static void ConfigureRepr(TfToken const &reprName,
                              const HdPointsReprDesc &desc);

protected:
    /// Constructor. instancerId, if specified, is the instancer which uses
    /// this point cloud as a prototype.
    HD_API
    HdPoints(SdfPath const& id,
             SdfPath const& instancerId = SdfPath());

    typedef _ReprDescConfigs<HdPointsReprDesc> _PointsReprConfig;

    HD_API
    static _PointsReprConfig::DescArray _GetReprDesc(TfToken const &reprName);

private:

    // Class can not be default constructed or copied.
    HdPoints()                             = delete;
    HdPoints(const HdPoints &)             = delete;
    HdPoints &operator =(const HdPoints &) = delete;

    static _PointsReprConfig _reprDescConfig;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_POINTS_H
