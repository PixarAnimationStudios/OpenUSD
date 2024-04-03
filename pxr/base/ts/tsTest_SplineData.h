//
// Copyright 2023 Pixar
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
    struct TS_API Knot
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
        Knot();
        Knot(
            const Knot &other);
        Knot& operator=(
            const Knot &other);
        bool operator==(
            const Knot &other) const;
        bool operator!=(
            const Knot &other) const;
        bool operator<(
            const Knot &other) const;
    };
    using KnotSet = std::set<Knot>;

    // Inner-loop parameters.
    //
    // The pre-looping interval is times [preLoopStart, protoStart).
    // The prototype interval is times [protoStart, protoEnd).
    // The post-looping interval is times [protoEnd, postLoopEnd],
    //   or, if closedEnd is false, the same interval, but open at the end.
    // To decline pre-looping or post-looping, make that interval empty.
    //
    // The value offset specifies the difference between the value at the starts
    // of consecutive iterations.
    //
    // It is common, but not required, to use a subset of functionality:
    // - Knots at the start and end of the prototype interval
    // - Whole numbers of loop iterations
    //     (sizes of looping intervals are multiples of size of proto interval)
    // - Value offset initially set to original value difference
    //     between ends of prototype interval
    // - closedEnd true
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
    // If closedEnd is true, and there is a whole number of post-iterations, and
    // there is a knot at the prototype start time, then a final copy of the
    // first prototype knot will be echoed at the end of the last
    // post-iteration.
    //
    struct TS_API InnerLoopParams
    {
        bool enabled = false;
        double protoStart = 0;
        double protoEnd = 0;
        double preLoopStart = 0;
        double postLoopEnd = 0;
        bool closedEnd = true;
        double valueOffset = 0;

    public:
        InnerLoopParams();
        InnerLoopParams(
            const InnerLoopParams &other);
        InnerLoopParams& operator=(
            const InnerLoopParams &other);
        bool operator==(
            const InnerLoopParams &other) const;
        bool operator!=(
            const InnerLoopParams &other) const;

        bool IsValid() const;
    };

    // Extrapolation parameters for the ends of a spline beyond the knots.
    struct TS_API Extrapolation
    {
        ExtrapMethod method = ExtrapHeld;
        double slope = 0;
        LoopMode loopMode = LoopNone;

    public:
        Extrapolation();
        Extrapolation(ExtrapMethod method);
        Extrapolation(
            const Extrapolation &other);
        Extrapolation& operator=(
            const Extrapolation &other);
        bool operator==(
            const Extrapolation &other) const;
        bool operator!=(
            const Extrapolation &other) const;
    };

public:
    TS_API
    TsTest_SplineData();

    TS_API
    TsTest_SplineData(
        const TsTest_SplineData &other);

    TS_API
    TsTest_SplineData&
    operator=(
        const TsTest_SplineData &other);

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
    std::string GetDebugDescription() const;

private:
    bool _isHermite = false;
    KnotSet _knots;
    Extrapolation _preExtrap;
    Extrapolation _postExtrap;
    InnerLoopParams _innerLoopParams;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
