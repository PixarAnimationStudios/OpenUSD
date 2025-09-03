//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
struct HdPointsReprDesc
{
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
class HdPoints: public HdRprim
{
public:
    HD_API
    ~HdPoints() override;

    inline HdDisplayStyle  GetDisplayStyle(HdSceneDelegate* delegate)  const;

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
    HdPoints(SdfPath const& id);

    using _PointsReprConfig = _ReprDescConfigs<HdPointsReprDesc>;

    HD_API
    static _PointsReprConfig::DescArray _GetReprDesc(TfToken const &reprName);

private:

    // Class can not be default constructed or copied.
    HdPoints()                             = delete;
    HdPoints(const HdPoints &)             = delete;
    HdPoints &operator =(const HdPoints &) = delete;

    static _PointsReprConfig _reprDescConfig;
};

inline HdDisplayStyle
HdPoints::GetDisplayStyle(HdSceneDelegate* delegate) const
{
    return delegate->GetDisplayStyle(GetId());
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_POINTS_H
