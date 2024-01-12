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

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_SampleBezier.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/math.h"

PXR_NAMESPACE_OPEN_SCOPE

using SData = TsTest_SplineData;

// Obtain one sample between knot0 and knot1, at parameter value t.
// Uses de Casteljau algorithm.
//
static TsTest_Sample
_ComputeSample(
    const SData::Knot &knot0,
    const SData::Knot &knot1,
    const double t)
{
    const GfVec2d p0(knot0.time, knot0.value);
    const GfVec2d tan1(knot0.postLen, knot0.postSlope * knot0.postLen);
    const GfVec2d p1 = p0 + tan1;
    const GfVec2d p3(knot1.time, knot1.value);
    const GfVec2d tan2(-knot1.preLen, -knot1.preSlope * knot1.preLen);
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
    const SData &splineData,
    const int numSamples)
{
    if (splineData.GetRequiredFeatures() != SData::FeatureBezierSegments)
    {
        TF_CODING_ERROR("SampleBezier supports only plain Beziers");
        return {};
    }

    const SData::KnotSet &knots = splineData.GetKnots();
    if (knots.size() < 2)
    {
        TF_CODING_ERROR("SampleBezier requires at least two keyframes");
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
    const SData::Knot &lastKnot = *knots.rbegin();
    result.push_back(TsTest_Sample(lastKnot.time, lastKnot.value));

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
