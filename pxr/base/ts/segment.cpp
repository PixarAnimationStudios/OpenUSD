//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/segment.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

#include <algorithm>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(Ts_SegmentInterp::ValueBlock, "Value Block");
    TF_ADD_ENUM_NAME(Ts_SegmentInterp::Held, "Held");
    TF_ADD_ENUM_NAME(Ts_SegmentInterp::Linear, "Linear");
    TF_ADD_ENUM_NAME(Ts_SegmentInterp::Bezier, "Bezier");
    TF_ADD_ENUM_NAME(Ts_SegmentInterp::Hermite, "Hermite");
    TF_ADD_ENUM_NAME(Ts_SegmentInterp::PreExtrap, "PreExtrap");
    TF_ADD_ENUM_NAME(Ts_SegmentInterp::PostExtrap, "PostExtrap");
}

// Utility to convert a TsInterpMode value into a Ts_SegmentInterp. Because the
// TsInterpMode just includes a generic "curve" interpolation, we also need the
// spline's curve type to know if this segment is a bezier or hermite curve.
void Ts_Segment::SetInterp(TsInterpMode interpMode, TsCurveType curveType)
{
    switch (interpMode)
    {
      case TsInterpValueBlock:
        interp = Ts_SegmentInterp::ValueBlock;
        break;

      case TsInterpHeld:
        interp = Ts_SegmentInterp::Held;
        break;

      case TsInterpLinear:
        interp = Ts_SegmentInterp::Linear;
        break;

      case TsInterpCurve:
        if (curveType == TsCurveTypeHermite) {
            interp = Ts_SegmentInterp::Hermite;
        } else {
            interp = Ts_SegmentInterp::Bezier;
        }
        break;

      default:
        // Generate a nicer error message.
        constexpr bool invalid_TsInterpMode_value = false;
        TF_AXIOM(invalid_TsInterpMode_value);
    }
}


// This computes a derivative, but only for u == 0 or u == 1 for curved
// segments. Fortunately, this is all that's required for linear extrapolation.
double
Ts_Segment::_ComputeDerivative(double u) const
{
    u = std::clamp(u, 0.0, 1.0);

    // It's possible to not have a derivative due to value blocks. But we're
    // going to return 0.0 in that case just to keep things simple. The result
    // will be a value block with slope of 0.0. :-)
    switch (interp)
    {
      case Ts_SegmentInterp::ValueBlock:
      case Ts_SegmentInterp::Held:
        return 0.0;

      case Ts_SegmentInterp::Linear:
        return (p1[1] - p0[1]) / (p1[0] - p0[0]);

      case Ts_SegmentInterp::PreExtrap:
        return p0[1];

      case Ts_SegmentInterp::PostExtrap:
        return p1[1];

      case Ts_SegmentInterp::Bezier:
      case Ts_SegmentInterp::Hermite:
        TF_VERIFY(u == 0.0 || u == 1.0,
                  "Cannot yet compute derivatives at arbitrary values. u = %g",
                  u);

        // Not a generalized solution right now, but it works for 0 and 1
        if (u <= 0.5) {
            // slope is equal to first tangent direction.
            GfVec2d tangent = t0 - p0;
            return tangent[1] / tangent[0];
        } else if (u > 0.5) {
            // slope is equal to the last tangent direction. Note that
            // this direction is measured from t1 to p1.
            GfVec2d tangent = p1 - t1;
            return tangent[1] / tangent[0];
        }

      default:
        TF_CODING_ERROR("Invalid segment interp (%d) in _ComputeDerivative",
                        int(interp));
    }

    return 0.0;
}

// Output operator for testing purposes.
std::ostream& operator <<(std::ostream& out, const Ts_SegmentInterp interp)
{
    return out << TfEnum::GetFullName(interp);
}

// Output operators for testing purposes.
std::ostream& operator <<(std::ostream& out, const Ts_Segment& seg)
{
    return out << "Ts_Segment{"
               << "{" << seg.p0[0] << ", " << seg.p0[1] << "}, "
               << "{" << seg.t0[0] << ", " << seg.t0[1] << "}, "
               << "{" << seg.t1[0] << ", " << seg.t1[1] << "}, "
               << "{" << seg.p1[0] << ", " << seg.p1[1] << "}, "
               << seg.interp << "}";
}

PXR_NAMESPACE_CLOSE_SCOPE
