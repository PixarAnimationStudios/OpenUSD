//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hd/points.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


HdPoints::HdPoints(SdfPath const& id)
    : HdRprim(id)
{
    /*NOTHING*/
}

HdPoints::~HdPoints() = default;

/* virtual */
TfTokenVector const &
HdPoints::GetBuiltinPrimvarNames() const
{
    static const TfTokenVector primvarNames = {
        HdTokens->points,
        HdTokens->normals,
        HdTokens->widths
    };
    return primvarNames;
}

// static repr configuration
HdPoints::_PointsReprConfig HdPoints::_reprDescConfig;

/* static */
void
HdPoints::ConfigureRepr(TfToken const &reprName,
                        const HdPointsReprDesc &desc)
{
    HD_TRACE_FUNCTION();

    _reprDescConfig.AddOrUpdate(reprName, _PointsReprConfig::DescArray{desc});
}

/* static */
HdPoints::_PointsReprConfig::DescArray
HdPoints::_GetReprDesc(TfToken const &reprName)
{
    return _reprDescConfig.Find(reprName);
}

PXR_NAMESPACE_CLOSE_SCOPE

