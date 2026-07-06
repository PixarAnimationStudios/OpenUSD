//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/tsTest_SampleBezier.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/math.h"

PXR_NAMESPACE_OPEN_SCOPE

// Obtain one sample between knot0 and knot1, at parameter value t.
// Uses de Casteljau algorithm.
//
static TsTest_Sample
_ComputeSample(
    const TsKnot& knot0,
    const TsKnot& knot1,
    const double t)
{
    double knotValue0, knotPostTanSlope0,
           knotValue1, knotPreTanSlope1;
    bool success = true;
    success &= knot0.GetValue<double>(&knotValue0);
    success &= knot0.GetPostTanSlope<double>(&knotPostTanSlope0);
    success &= knot1.GetValue<double>(&knotValue1);
    success &= knot1.GetPreTanSlope<double>(&knotPreTanSlope1);

    TF_VERIFY(success,
              "TsTest_SampleBezier supports double-valued splines only.");

    const GfVec2d p0(knot0.GetTime(), knotValue0);
    const GfVec2d tan1(knot0.GetPostTanWidth(),
                       knotPostTanSlope0 * knot0.GetPostTanWidth());
    const GfVec2d p1 = p0 + tan1;
    const GfVec2d p3(knot1.GetTime(), knotValue1);
    const GfVec2d tan2(-knot1.GetPreTanWidth(),
                       -knotPreTanSlope1 * knot1.GetPreTanWidth());
    const GfVec2d p2 = p3 + tan2;

    const GfVec2d lerp11 = GfLerp(t, p0, p1);
    const GfVec2d lerp12 = GfLerp(t, p1, p2);
    const GfVec2d lerp13 = GfLerp(t, p2, p3);

    const GfVec2d lerp21 = GfLerp(t, lerp11, lerp12);
    const GfVec2d lerp22 = GfLerp(t, lerp12, lerp13);

    const GfVec2d lerp31 = GfLerp(t, lerp21, lerp22);

    return TsTest_Sample(lerp31[0], lerp31[1]);
}

TsTest_SampleVec
TsTest_SampleBezier(
    const TsSpline& spline,
    const int numSamples)
{
    if (spline.GetCurveType() != TsCurveTypeBezier)
    {
        TF_CODING_ERROR("SampleBezier supports only plain Beziers");
        return {};
    }

    const TsKnotMap knots = spline.GetKnots();
    if (knots.size() < 2)
    {
        TF_CODING_ERROR("SampleBezier requires at least two knots");
        return {};
    }

    // Divide samples equally among segments.  Determine increment of 't'
    // (parameter value on [0, 1]) per sample.
    const int samplesPerSegment = numSamples / knots.size();
    const double tPerSample = 1.0 / (samplesPerSegment + 1);

    TsTest_SampleVec result;

    // Process each segment.
    for (auto knotIt = knots.begin(), knotNextIt = knotIt;
         ++knotNextIt != knots.end(); knotIt++)
    {
        // Divide segment into samples.
        for (int j = 0; j < samplesPerSegment; j++)
        {
            // Sample at this 't' value.
            const double t = tPerSample * j;
            result.push_back(_ComputeSample(*knotIt, *knotNextIt, t));
        }
    }

    // Add one sample at the end of the last segment.
    const TsKnot& lastKnot = *knots.rbegin();
    double lastKnotValue;
    TF_VERIFY(lastKnot.GetValue<double>(&lastKnotValue),
              "TsTest_SampleBezier supports double-valued splines only.");
    const TsTest_Sample sample(lastKnot.GetTime(), lastKnotValue);
    result.push_back(sample);

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
