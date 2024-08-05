//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TS_TEST_SPLINE_DATA_H
#define PXR_BASE_TS_TS_TEST_SPLINE_DATA_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"

#include <string>
#include <set>

PXR_NAMESPACE_OPEN_SCOPE

// A generic way of encoding spline control parameters.  Allows us to pass the
// same data to different backends (Ts, mayapy, etc) for evaluation.
//
class TsTest_SplineData
{
public:
    // Interpolation method for a spline segment.
    enum InterpMethod
    {
        InterpHeld,
        InterpLinear,
        InterpCurve
    };

    // Extrapolation method for the ends of a spline beyond the knots.
    enum ExtrapMethod
    {
        ExtrapHeld,
        ExtrapLinear,
        ExtrapSloped,
        ExtrapLoop
    };

    // Looping modes.
    enum LoopMode
    {
        LoopNone,
        LoopContinue,   // Used by inner loops.  Copy whole knots.
        LoopRepeat,     // Used by extrap loops.  Repeat with offset.
        LoopReset,      // Used by extrap loops.  Repeat identically.
        LoopOscillate   // Used by extrap loops.  Alternate forward / reverse.
    };

    // Features that may be required by splines.
    enum Feature
    {
        FeatureHeldSegments = 1 << 0,
        FeatureLinearSegments = 1 << 1,
        FeatureBezierSegments = 1 << 2,
        FeatureHermiteSegments = 1 << 3,
        FeatureAutoTangents = 1 << 4,
        FeatureDualValuedKnots = 1 << 5,
        FeatureInnerLoops = 1 << 6,
        FeatureExtrapolatingLoops = 1 << 7,
        FeatureExtrapolatingSlopes = 1 << 8
    };
    using Features = unsigned int;

    // One knot in a spline.
    struct Knot
    {
        double time = 0;
        InterpMethod nextSegInterpMethod = InterpHeld;
        double value = 0;
        bool isDualValued = false;
        double preValue = 0;
        double preSlope = 0;
        double postSlope = 0;
        double preLen = 0;
        double postLen = 0;
        bool preAuto = false;
        bool postAuto = false;

    public:
        TS_API
        bool operator==(
            const Knot &other) const;

        TS_API
        bool operator!=(
            const Knot &other) const;

        TS_API
        bool operator<(
            const Knot &other) const;
    };
    using KnotSet = std::set<Knot>;

    // Inner-loop parameters.
    //
    // The prototype interval [protoStart, protoEnd) is duplicated before and/or
    // after where it occurs.
    //
    // There must always be a knot exactly at protoStart.  The start knot is
    // copied to the end of the prototype, and to the end of every loop
    // iteration.
    //
    // A knot exactly at the end of the prototype interval is not part of the
    // prototype.  If there is post-looping, a knot at the end of the prototype
    // interval is overwritten by a copy of the knot from the start of the
    // prototype interval.
    //
    // Enabling inner looping can change the shape of the prototype interval
    // (and thus all looped copies), because the first knot is echoed as the
    // last.  Inner looping does not aim to make copies of an existing shape; it
    // aims to set up for continuity at loop joins.
    //
    // The value offset specifies the difference between the value at the starts
    // of consecutive iterations.
    //
    struct InnerLoopParams
    {
        bool enabled = false;
        double protoStart = 0;
        double protoEnd = 0;
        int numPreLoops = 0;
        int numPostLoops = 0;
        double valueOffset = 0;

    public:
        TS_API
        bool operator==(
            const InnerLoopParams &other) const;

        TS_API
        bool operator!=(
            const InnerLoopParams &other) const;

        TS_API
        bool IsValid() const;
    };

    // Extrapolation parameters for the ends of a spline beyond the knots.
    struct Extrapolation
    {
        ExtrapMethod method = ExtrapHeld;
        double slope = 0;
        LoopMode loopMode = LoopNone;

    public:
        TS_API
        Extrapolation();

        TS_API
        Extrapolation(ExtrapMethod method);

        TS_API
        bool operator==(
            const Extrapolation &other) const;

        TS_API
        bool operator!=(
            const Extrapolation &other) const;
    };

public:
    TS_API
    bool operator==(
        const TsTest_SplineData &other) const;

    TS_API
    bool operator!=(
        const TsTest_SplineData &other) const;

    TS_API
    void SetIsHermite(bool hermite);

    TS_API
    void AddKnot(
        const Knot &knot);

    TS_API
    void SetKnots(
        const KnotSet &knots);

    TS_API
    void SetPreExtrapolation(
        const Extrapolation &preExtrap);

    TS_API
    void SetPostExtrapolation(
        const Extrapolation &postExtrap);

    TS_API
    void SetInnerLoopParams(
        const InnerLoopParams &params);

    TS_API
    bool GetIsHermite() const;

    TS_API
    const KnotSet&
    GetKnots() const;

    TS_API
    const Extrapolation&
    GetPreExtrapolation() const;

    TS_API
    const Extrapolation&
    GetPostExtrapolation() const;

    TS_API
    const InnerLoopParams&
    GetInnerLoopParams() const;

    TS_API
    Features GetRequiredFeatures() const;

    TS_API
    std::string GetDebugDescription(int precision = 6) const;

private:
    bool _isHermite = false;
    KnotSet _knots;
    Extrapolation _preExtrap;
    Extrapolation _postExtrap;
    InnerLoopParams _innerLoopParams;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
