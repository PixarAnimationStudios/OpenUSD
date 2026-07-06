//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TS_TEST_MUSEUM_H
#define PXR_BASE_TS_TS_TEST_MUSEUM_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/spline.h"

#include <vector>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


// A collection of museum exhibits.  These are spline cases that can be used by
// tests to exercise various behaviors.
//
class TsTest_Museum
{
public:
    enum DataId
    {
        // Single-segment ordinary curves.
        TwoKnotBezier,
        TwoKnotBezierAutoEase,
        TwoKnotHermite,
        TwoKnotLinear,

        // Longer splines.
        FourKnotBezier,
        FourKnotHermite,

        // Looping cases.
        SimpleInnerLoop,
        InnerLoop2and2,
        InnerLoopPre,
        InnerLoopPost,
        ExtrapLoopRepeat,
        ExtrapLoopRepeatDualValued,
        ExtrapLoopRepeatBoundary,
        ExtrapLoopRepeatDualValuedBoundary,
        ExtrapLoopReset,
        ExtrapLoopResetDualValued,
        ExtrapLoopResetBoundary,
        ExtrapLoopResetInvalidBoundary,
        ExtrapLoopOscillate,
        ExtrapLoopOscillateBoundary,
        InnerAndExtrapLoops,

        // Tests of several regressive Bezier cases.
        RegressiveLoop,
        RegressiveS,
        RegressiveSStandard,
        RegressiveSPreOut,
        RegressiveSPostOut,
        RegressiveSBothOut,
        RegressivePreJ,
        RegressivePostJ,
        RegressivePreC,
        RegressivePostC,
        RegressivePreG,
        RegressivePostG,
        RegressivePreFringe,
        RegressivePostFringe,

        // Bold case: escaped tangents, but not regressive.
        BoldS,

        // Edge case: cusp.  Valid but just barely; undefined tangent.
        Cusp,

        // Edge case: vertical tangent in center.  Also a less extreme variant.
        CenterVertical,
        NearCenterVertical,

        // A case that hit an old bug.  A particular case of a single vertical.
        VerticalTorture,

        // Edge case: 4/3 + 1/3 tangents.  Vertical at 24/27.
        // Also the inverse.
        FourThirdOneThird,
        OneThirdFourThird,

        // Edge cases: single verticals at start and end.
        StartVert,
        EndVert,

        // Fringe vertical between FourThirdOneThird and EndVert.
        FringeVert,

        // N-shape, with near-vertical tangents.
        MarginalN,

        // Both tangents zero-length.
        ZeroTans,

        // Exercise many features of the object model.
        ComplexParams
    };

    // Get a case by ID.
    TS_API
    static TsSpline GetSpline(
        DataId id,
        const TfType valueType = Ts_GetType<double>());

    // Get all case names.
    TS_API
    static std::vector<std::string> GetAllNames();

    // Get a case by name.
    TS_API
    static TsSpline GetSplineByName(
        const std::string &name,
        const TfType valueType = Ts_GetType<double>());

private:
    static Ts_TypedSplineData<double> _GetData(DataId id);

    static TsSpline _SplineDataToSpline(
        const Ts_TypedSplineData<double>& data,
        const TfType valueType);
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
