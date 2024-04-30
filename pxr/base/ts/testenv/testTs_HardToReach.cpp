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
#include "pxr/base/ts/diff.h"
#include "pxr/base/ts/evaluator.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/typeRegistry.h"

#include "pxr/base/gf/range1d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/stringUtils.h"

#include <cassert>
#include <limits>
#include <sstream>
#include <utility>

PXR_NAMESPACE_USING_DIRECTIVE

using std::string;

static const TsTime inf = std::numeric_limits<TsTime>::infinity();

// Helper class that verifies expected diffs from spline modifications as
// reported by two sources: the intervalAffected out-param from the spline API,
// and the TsFindChangedInterval utility.
//
class _SplineTester
{
public:
    _SplineTester(const TsSpline & v) :
        spline(v)
    {}

    bool SetKeyFrame(const TsKeyFrame &keyFrame,
                         const GfInterval &expectedInterval)
    {
        // Make a copy of the previous spline.
        const TsSpline oldSpline = spline;

        // Make the modification and record intervalAffected.
        GfInterval actionInterval;
        spline.SetKeyFrame(keyFrame, &actionInterval);

        // Diff the previous and current splines.
        const GfInterval diffInterval =
            TsFindChangedInterval(oldSpline, spline);

        // Verify both intervals are as expected.
        if (actionInterval == expectedInterval
                && diffInterval == expectedInterval) {
            return true;
        } else {
            std::cerr << "Failed SetKeyFrame:\n"
                      << "  actionInterval:   " << actionInterval << "\n"
                      << "  diffInterval:     " << diffInterval << "\n"
                      << "  expectedInterval: " << expectedInterval << "\n"
                      << "Result spline was:\n" << spline
                      << std::endl;
            return false;
        }
    }

    bool RemoveKeyFrame(const TsTime time,
                            const GfInterval &expectedInterval)
    {
        // Make a copy of the previous spline.
        const TsSpline oldSpline = spline;

        // Make the modification and record intervalAffected.
        GfInterval actionInterval;
        spline.RemoveKeyFrame(time, &actionInterval);

        // Diff the previous and current splines.
        const GfInterval diffInterval =
            TsFindChangedInterval(oldSpline, spline);

        // Verify both intervals are as expected.
        if (actionInterval == expectedInterval
                && diffInterval == expectedInterval) {
            return true;
        } else {
            std::cerr << "Failed RemoveKeyFrame:\n"
                      << "  actionInterval:   " << actionInterval << "\n"
                      << "  diffInterval:     " << diffInterval << "\n"
                      << "  expectedInterval: " << expectedInterval << "\n"
                      << "Result spline was:\n" << spline
                      << std::endl;
            return false;
        }
    }

    bool SetValue(const TsSpline &newValue,
                      const GfInterval &expectedInterval)
    {
        // Make a copy of the previous spline.
        const TsSpline oldSpline = spline;

        // Record the new value.  There is no API that returns an
        // intervalAffected for whole-spline value changes.
        spline = newValue;

        // Diff the previous and current splines.
        const GfInterval diffInterval =
            TsFindChangedInterval(oldSpline, spline);

        // Verify the diff interval is as expected.
        if (diffInterval == expectedInterval) {
            return true;
        } else {
            std::cerr << "Failed SetValue:\n"
                      << "  diffInterval:     " << diffInterval << "\n"
                      << "  expectedInterval: " << expectedInterval
                      << std::endl;
            return false;
        }
    }

public:
    TsSpline spline;
};


template <typename T>
static bool
_IsClose(const double& a, const double& b,
         T eps = std::numeric_limits<T>::epsilon())
{
    return fabs(a - b) < eps;
}

template <typename T>
static bool
_IsClose(const VtValue& a, const VtValue& b,
         T eps = std::numeric_limits<T>::epsilon())
{
    return fabs(a.Get<T>() - b.Get<T>()) < eps;
}

template <>
bool
_IsClose(const VtValue& a, const VtValue& b, GfVec2d eps)
{
    GfVec2d _b = b.Get<GfVec2d>();
    GfVec2d negB(-_b[0], -_b[1]);
    GfVec2d diff = a.Get<GfVec2d>() + negB;
    return fabs(diff[0]) < eps[0] && fabs(diff[1]) < eps[1];
}

template <typename T>
static void
_AssertSamples(const TsSpline & val,
               const TsSamples & samples,
               double startTime, double endTime, T tolerance)
{
    for (size_t i = 0; i < samples.size(); ++i) {
        TF_AXIOM(!samples[i].isBlur);
        TF_AXIOM(samples[i].leftTime <= samples[i].rightTime);
        if (i != 0) {
            TF_AXIOM(samples[i - 1].rightTime <= samples[i].leftTime);
        }

        if (samples[i].leftTime >= startTime) {
            TF_AXIOM(_IsClose<T>(samples[i].leftValue,
                               val.Eval(samples[i].leftTime, TsRight),
                               tolerance));
        }
        if (samples[i].rightTime <= endTime) {
            TF_AXIOM(_IsClose<T>(samples[i].rightValue,
                               val.Eval(samples[i].rightTime, TsLeft),
                               tolerance));
        }
    }
    if (!samples.empty()) {
        TF_AXIOM(samples.front().leftTime <= startTime);
        TF_AXIOM(samples.back().rightTime >=   endTime);
    }
}

// Helper to verify that raw spline evals match values
// from the TsEvaluator.
void _VerifyEvaluator(TsSpline spline)
{
    TsEvaluator<double> evaluator(spline);
    for (TsTime sample = -2.0; sample < 2.0; sample += 0.1) {
        VtValue rawEvalValue = spline.Eval(sample);
        TF_AXIOM(_IsClose<double>(!rawEvalValue.IsEmpty() ?
                                rawEvalValue.Get<double>() :
                                TsTraits<double>::zero,
                                evaluator.Eval(sample)));
    }
}

void
_AddSingleKnotSpline(TsTime knotTime, const VtValue &knotValue,
                     std::vector<TsSpline> *splines)
{
    TsSpline spline;
    spline.SetKeyFrame(TsKeyFrame(knotTime, knotValue));
    splines->push_back(spline);
}

// Helper function to verify that a setting the value of a spline to multiple
// single knot splines with the same value but their one keyframe at different
// times will always cause the same invalidation interval.
bool _TestSetSingleValueSplines(const TsSpline &testSpline,
                                const VtValue &value,
                                const GfInterval &testInterval)
{
    // First we create a list of single knot splines with the same flat value.
    // We create a full spread a splines that hit all the case of the single 
    // knot being on each key frame, between each key frame, and before and 
    // after the first and last key frames.  This should cover every case.
    std::vector<TsSpline> singleKnotSplines;
    TsKeyFrameMap keyFrames = testSpline.GetKeyFrames();
    TsTime prevTime = 0;
    // Get all the spline's key frames.
    TF_FOR_ALL(kf, keyFrames) {
        TsTime time = kf->GetTime();
        if (singleKnotSplines.empty()) {
            // The first key frame, so add a spline with a knot before the
            // first key frame.
            _AddSingleKnotSpline(time - 5.0, value, &singleKnotSplines);
        } else {
            // Add a spline with its knot between the previous key frame and
            // this key frame.
            _AddSingleKnotSpline((time - prevTime) / 2.0, value, &singleKnotSplines);
        }
        // Add a spline with a knot a this key frame.
        _AddSingleKnotSpline(time, value, &singleKnotSplines);

        prevTime = time;
    }
    // Add the final spline with the knot after the last key frame.
    _AddSingleKnotSpline(prevTime + 5.0, value, &singleKnotSplines);

    // Test setting each of the single knot spline as the value over the 
    // given spline and make sure each one has the same given invalidation 
    // interval.
    TF_FOR_ALL(singleKnotSpline, singleKnotSplines) {
        if (!_SplineTester(testSpline).SetValue(
            *singleKnotSpline, testInterval)) {
            std::cerr << "Failed to set single value spline: " << (*singleKnotSpline) << "\n";
            return false;
        }
    }
    return true;
}


void TestEvaluator()
{
    // Empty spline case.
    TsSpline spline;
    _VerifyEvaluator(spline);

    // Single knot case
    spline.SetKeyFrame( TsKeyFrame(-1.0, -1.0, TsKnotBezier) );
    _VerifyEvaluator(spline);

    // Test evaluation with non-flat tangent.
    spline.Clear();
    spline.SetKeyFrame( TsKeyFrame(-1.0, 0.0, TsKnotBezier, 0.0, 0.0, 0.9, 0.9) );
    spline.SetKeyFrame( TsKeyFrame(0.0, 1.0, TsKnotBezier, 
                                     0.168776965344754, 0.168776965344754,
                                     1.85677, 1.85677) );
    spline.SetKeyFrame( TsKeyFrame(1.0, 0.0, TsKnotBezier, 0.0, 0.0, 0.9, 0.9) );
    _VerifyEvaluator(spline);

    // Test evaluation with long tangent that causes the spline to be clipped.
    spline.Clear();
    spline.SetKeyFrame( TsKeyFrame(-1.0, 0.0, TsKnotBezier, 0.0, 0.0, 0.9, 0.9) );
    spline.SetKeyFrame( TsKeyFrame(0.0, 1.0, TsKnotBezier,
                                     -0.0691717091793238, -0.0691717091793238,
                                     9.49162, 9.49162) );
    spline.SetKeyFrame( TsKeyFrame(1.0, 0.0, TsKnotBezier, 0.0, 0.0, 0.9, 0.9) );
    _VerifyEvaluator(spline);
}

void TestSplineDiff()
{
    printf("\nTest spline diffing\n");

    TsSpline initialVal;
    initialVal.SetKeyFrame( TsKeyFrame(1, VtValue( "bar" )) );
    _SplineTester tester(initialVal);

    TF_AXIOM(tester.SetKeyFrame( TsKeyFrame(0, VtValue( "blah" )), 
                                   GfInterval(-inf, 1.0, false, false)));
    TF_AXIOM(tester.SetKeyFrame( TsKeyFrame(2, VtValue( "papayas" )),
                                   GfInterval(2.0, inf, true, false) ));
    TF_AXIOM(tester.SetKeyFrame( TsKeyFrame(4, VtValue( "navel" )),
                                   GfInterval( 4.0, inf, true, false )));

    // Set a kf in the middle
    TF_AXIOM(tester.SetKeyFrame( TsKeyFrame(3, VtValue( "pippins" )),
                                   GfInterval(3.0, 4.0, true, false )));

    // Test setting and removing redundant key frames
    TF_AXIOM(tester.SetKeyFrame( TsKeyFrame(2.5, VtValue( "papayas" )),
                                   GfInterval() ));
    TF_AXIOM(tester.RemoveKeyFrame( 2.5, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( TsKeyFrame(-1.0, VtValue( "blah" )),
                                   GfInterval() ));
    TF_AXIOM(tester.RemoveKeyFrame( -1.0, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( TsKeyFrame(5.0, VtValue( "navel" )),
                                   GfInterval() ));
    TF_AXIOM(tester.RemoveKeyFrame( 5.0, GfInterval()));

    // Remove middle kf
    TF_AXIOM(tester.RemoveKeyFrame( 3, GfInterval(3.0, 4.0, true, false )));

    // Remove first kf
    TF_AXIOM(tester.RemoveKeyFrame( 0, GfInterval(-inf, 1.0, false, false )));

    // Remove last kf
    TF_AXIOM(tester.RemoveKeyFrame( 4, GfInterval(4.0, inf, true, false )));

    printf("\tpassed\n");
}

void TestSplineDiff2()
{
    printf("\nTest more spline diffing\n");

    _SplineTester tester = _SplineTester(TsSpline());

    // Set a first knot
    TF_AXIOM(tester.SetKeyFrame(
            TsKeyFrame(0.0, 0.0),
            GfInterval::GetFullInterval()));

    // Set a knot on the right side
    TF_AXIOM(tester.SetKeyFrame(
            TsKeyFrame(3.0, 1.0),
            GfInterval(0.0, inf, false, false)));

    // Set a knot in the middle of those
    TF_AXIOM(tester.SetKeyFrame(
            TsKeyFrame(2.0, 2.0),
            GfInterval(0.0, 3.0, false, false)));

    // Set another knot in the middle
    TF_AXIOM(tester.SetKeyFrame(
            TsKeyFrame(1.0, 3.0),
            GfInterval(0.0, 2.0, false, false)));

    // Set the first knot again
    TF_AXIOM(tester.SetKeyFrame(
            TsKeyFrame(0.0, 4.0),
            GfInterval(-inf, 1.0, false, false)));
}

void TestHeldThenBezier()
{
    printf("\nTest held knot followed by Bezier knot\n");

    _SplineTester tester = _SplineTester(TsSpline());

    TF_AXIOM(tester.SetKeyFrame(
            TsKeyFrame(0.0, 123.0, TsKnotHeld),
            GfInterval::GetFullInterval()));

    TF_AXIOM(tester.SetKeyFrame(
            TsKeyFrame(1.0, 1.0, TsKnotBezier),
            GfInterval(1.0, inf, true, false)));

    TF_AXIOM(tester.RemoveKeyFrame(
            1.0,
            GfInterval(1.0, inf, true, false)));

    printf("\tpassed\n");
}

void TestRedundantKnots()
{
    printf("\nTest redundant knots\n");

    _SplineTester tester = _SplineTester(TsSpline());

    // Add the first knot.
    TF_AXIOM(tester.SetKeyFrame(
               TsKeyFrame(1.0, 0.0),
               GfInterval::GetFullInterval()));

    // Add another knot.
    TF_AXIOM(tester.SetKeyFrame(
               TsKeyFrame(2.0, 1.0),
               GfInterval(1.0, inf, false, false)));

    // Re-adding the same knot should give an empty edit interval.
    TF_AXIOM(tester.SetKeyFrame(
               TsKeyFrame(2.0, 1.0),
               GfInterval()));

    // Changing an existing knot should cause changes.
    TF_AXIOM(tester.SetKeyFrame(
               TsKeyFrame(2.0, 0.0),
               GfInterval(1.0, inf, false, false)));
    TF_AXIOM(tester.SetKeyFrame(
               TsKeyFrame(2.0, 1.0),
               GfInterval(1.0, inf, false, false)));

    // Add some redundant knots.
    TF_AXIOM(tester.SetKeyFrame(
               TsKeyFrame(3.0, 1.0),
               GfInterval()));
    TF_AXIOM(tester.SetKeyFrame(
               TsKeyFrame(4.0, 1.0),
               GfInterval()));

    // Redundant knot removed, edit interval should be empty.  
    TF_AXIOM(tester.RemoveKeyFrame(3.0, GfInterval()));

    // Redundant knot removed at end of spline, interval should be empty.
    TF_AXIOM(tester.RemoveKeyFrame(4.0, GfInterval()));

    // Removing a non-redundant knot should cause changes.
    TF_AXIOM(tester.RemoveKeyFrame(2.0, GfInterval(1.0, inf, false, false)));

    // Final knot removed.  This may or may not have been redundant, depending
    // on the fallback value, which is a higher-level concept; the spline
    // diffing code conservatively reports that the (flat) value may have
    // changed.
    TF_AXIOM(tester.RemoveKeyFrame(1.0, GfInterval::GetFullInterval()));

    // Setting flat constant splines should be redundant
    TsSpline sourceSpline;
    sourceSpline.SetKeyFrame( TsKeyFrame( 2, VtValue(1.0) ) );
    TsSpline splineToSet1;
    splineToSet1.SetKeyFrame( TsKeyFrame( 1, VtValue(0.0) ) );
    TsSpline splineToSet2;
    splineToSet2.SetKeyFrame( TsKeyFrame( 3, VtValue(1.0) ) );
    TsSpline splineToSet3;
    splineToSet3.SetKeyFrame( TsKeyFrame( 1, VtValue(1.0) ) );
    splineToSet3.SetKeyFrame( TsKeyFrame( 3, VtValue(1.0) ) );
    TF_AXIOM(!sourceSpline.IsVarying());
    TF_AXIOM(!splineToSet1.IsVarying());
    TF_AXIOM(!splineToSet2.IsVarying());
    TF_AXIOM(!splineToSet3.IsVarying());

    // Flat spline where values differ, whole interval is changed.
    tester = _SplineTester(sourceSpline);
    TF_AXIOM(tester.SetValue(splineToSet1, GfInterval::GetFullInterval()));

    // Flat spline same value at different time, no change
    tester = _SplineTester(sourceSpline);
    TF_AXIOM(tester.SetValue(splineToSet2, GfInterval()));
    tester = _SplineTester(sourceSpline);
    TF_AXIOM(tester.SetValue(splineToSet3, GfInterval()));

    printf("\tpassed\n");
}

void TestChangeIntervalsOnAssignment()
{
    printf("\nTest change intervals on assignment\n");

    // Create the first spline.
    TsSpline spline;
    spline.SetKeyFrame( TsKeyFrame( 1, VtValue( 0.0 ) ) );
    spline.SetKeyFrame( TsKeyFrame( 2, VtValue( 0.0 ) ) );
    spline.SetKeyFrame( TsKeyFrame( 3, VtValue( 0.0 ) ) );
    spline.SetKeyFrame( TsKeyFrame( 4, VtValue( 0.0 ) ) );
    spline.SetKeyFrame( TsKeyFrame( 5, VtValue( 0.0 ) ) );

    // Create a second spline with only one knot different.
    TsSpline spline2;
    spline2.SetKeyFrame( TsKeyFrame( 1, VtValue( 0.0 ) ) );
    spline2.SetKeyFrame( TsKeyFrame( 2, VtValue( 0.0 ) ) );
    spline2.SetKeyFrame( TsKeyFrame( 3, VtValue( 1.0 ) ) );
    spline2.SetKeyFrame( TsKeyFrame( 4, VtValue( 0.0 ) ) );
    spline2.SetKeyFrame( TsKeyFrame( 5, VtValue( 0.0 ) ) );

    // Change from one spline to the other and verify there is a difference.
    _SplineTester tester = _SplineTester(spline);
    TF_AXIOM(tester.SetValue(spline2, GfInterval(2.0, 4.0, false, false)));

    // Make a no-op change and verify there is no difference.
    TF_AXIOM(tester.SetValue(spline2, GfInterval()));

    printf("\tpassed\n");
}

void TestChangeIntervalsForKnotEdits()
{
    printf("\nTest changed intervals for knot edits\n");

    _SplineTester tester = _SplineTester(TsSpline());

    TF_AXIOM(tester.spline.GetExtrapolation() ==
           std::make_pair(TsExtrapolationHeld, TsExtrapolationHeld));

    TsKeyFrame kf0( 0, VtValue(1.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf1( 10, VtValue(-1.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf2( 20, VtValue(0.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );

    // Add a knot at time 0, value 1
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval::GetFullInterval()));

    // Add a knot at time 20, value 0
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(0, inf, false, false)));

    // Add a knot at time 10, value -1
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 20, false, false)));

    // First knot updates
    //   Left side tangents
    kf0.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval()));
    kf0.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval()));
    //   Right side tangents
    kf0.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, false, false)));
    kf0.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, false, false)));
    //   Time only
    TF_AXIOM(tester.RemoveKeyFrame(
               kf0.GetTime(), GfInterval(-inf, 10, false, false)));
    kf0.SetTime(2);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf0.GetTime(), GfInterval(-inf, 10, false, false)));
    kf0.SetTime(-2);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf0.GetTime(), GfInterval(-inf, 10, false, false)));
    kf0.SetTime(0);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    //   Value only
    kf0.SetValue(VtValue(2.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    //   Dual value (no value change)
    kf0.SetIsDualValued(true);
    kf0.SetLeftValue(kf0.GetValue());
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval()));
    //   Set left value
    kf0.SetLeftValue(VtValue(-1.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 0, false, true)));
    //   Set right value
    kf0.SetValue(VtValue(3.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, true, false)));
    //   Remove dual valued
    kf0.SetIsDualValued(false);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 0, false, true)));
    //   Change knot type
    kf0.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, false, false)));
    kf0.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, false, false)));
    kf0.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, false, false)));

    // Middle knot updates
    //   Left side tangents
    kf1.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 10, false, false)));
    kf1.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 10, false, false)));
    //   Right side tangents
    kf1.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(10, 20, false, false)));
    kf1.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(10, 20, false, false)));
    //   Time only
    TF_AXIOM(tester.RemoveKeyFrame(
               kf1.GetTime(), GfInterval(0, 20, false, false)));
    kf1.SetTime(12);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 20, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf1.GetTime(), GfInterval(0, 20, false, false)));
    kf1.SetTime(8);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 20, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf1.GetTime(), GfInterval(0, 20, false, false)));
    kf1.SetTime(10);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 20, false, false)));
    //   Value only
    kf1.SetValue(VtValue(2.0));
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 20, false, false)));
    //   Dual value (no value change)
    kf1.SetIsDualValued(true);
    kf1.SetLeftValue(kf1.GetValue());
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval()));
    //   Set left value
    kf1.SetLeftValue(VtValue(-1.0));
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 10, false, true)));
    //   Set right value
    kf1.SetValue(VtValue(3.0));
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(10, 20, true, false)));
    //   Remove dual valued
    kf1.SetIsDualValued(false);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 10, false, true)));
    //   Change knot type
    kf1.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 20, false, false)));
    kf1.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 20, false, false)));
    kf1.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(0, 20, false, false)));

    // Last knot updates
    //   Left side tangents
    kf2.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, false)));
    kf2.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, false)));
    //   Right side tangents
    kf2.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval()));
    kf2.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval()));
    //   Time only
    TF_AXIOM(tester.RemoveKeyFrame(
               kf2.GetTime(), GfInterval(10, inf, false, false)));
    kf2.SetTime(22);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf2.GetTime(), GfInterval(10, inf, false, false)));
    kf2.SetTime(18);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf2.GetTime(), GfInterval(10, inf, false, false)));
    kf2.SetTime(20);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    //   Value only
    kf2.SetValue(VtValue(2.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    //   Dual value (no value change)
    kf2.SetIsDualValued(true);
    kf2.SetLeftValue(kf2.GetValue());
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval()));
    //   Set left value
    kf2.SetLeftValue(VtValue(-1.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, true)));
    //   Set right value
    kf2.SetValue(VtValue(3.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(20, inf, true, false)));
    //   Remove dual valued
    kf2.SetIsDualValued(false);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, true)));
    //   Change knot type
    kf2.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, false)));
    kf2.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, false)));
    kf2.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, false)));

    // Set linear extrapolation on left
    tester.spline.SetExtrapolation(
        TsExtrapolationLinear, TsExtrapolationHeld);

    // First knot updates with linear extrapolation
    //   Left side tangents
    kf0.SetLeftTangentSlope(VtValue(-1.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 0, false, false)));
    kf0.SetLeftTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval()));
    //   Right side tangents
    kf0.SetRightTangentSlope(VtValue(-1.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, false, false)));
    kf0.SetRightTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, false, false)));
    //   Time only
    TF_AXIOM(tester.RemoveKeyFrame(
               kf0.GetTime(), GfInterval(-inf, 10, false, false)));
    kf0.SetTime(2);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf0.GetTime(), GfInterval(-inf, 10, false, false)));
    kf0.SetTime(-2);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf0.GetTime(), GfInterval(-inf, 10, false, false)));
    kf0.SetTime(0);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    //   Value only
    kf0.SetValue(VtValue(2.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    //   Dual value (no value change)
    kf0.SetIsDualValued(true);
    kf0.SetLeftValue(kf0.GetValue());
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval()));
    //   Set left value
    kf0.SetLeftValue(VtValue(-1.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 0, false, true)));
    //   Set right value
    kf0.SetValue(VtValue(3.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, true, false)));
    //   Remove dual valued
    kf0.SetIsDualValued(false);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 0, false, true)));
    //   Change knot type
    kf0.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    kf0.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 10, false, false)));
    kf0.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(0, 10, false, false)));

    // Set linear extrapolation on right
    tester.spline.SetExtrapolation(
        TsExtrapolationLinear, TsExtrapolationLinear);

    // Last knot updates with linear extrapolation
    //   Left side tangents
    kf2.SetLeftTangentSlope(VtValue(-1.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, false)));
    kf2.SetLeftTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, false)));
    //   Right side tangents
    kf2.SetRightTangentSlope(VtValue(-1.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(20, inf, false, false)));
    kf2.SetRightTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval()));
    //   Time only
    TF_AXIOM(tester.RemoveKeyFrame(
               kf2.GetTime(), GfInterval(10, inf, false, false)));
    kf2.SetTime(22);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf2.GetTime(), GfInterval(10, inf, false, false)));
    kf2.SetTime(18);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf2.GetTime(), GfInterval(10, inf, false, false)));
    kf2.SetTime(20);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    //   Value only
    kf2.SetValue(VtValue(2.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    //   Dual value (no value change)
    kf2.SetIsDualValued(true);
    kf2.SetLeftValue(kf2.GetValue());
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval()));
    //   Set left value
    kf2.SetLeftValue(VtValue(-1.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, true)));
    //   Set right value
    kf2.SetValue(VtValue(3.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(20, inf, true, false)));
    //   Remove dual valued
    kf2.SetIsDualValued(false);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, true)));
    //   Change knot type
    kf2.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    kf2.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, inf, false, false)));
    kf2.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(10, 20, false, false)));

    printf("\tpassed\n");
}

void TestChangeIntervalsForKnotEdits2()
{
    /* Test six knot spline with flat portions
    //                     O---------O
    //                    /           \
    // --------O---------O             O--------O----------
    */
    printf("\nTest changed intervals for knot edits (more)\n");

    _SplineTester tester = _SplineTester(TsSpline());

    TF_AXIOM(tester.spline.GetExtrapolation() ==
           std::make_pair(TsExtrapolationHeld, TsExtrapolationHeld));

    TsKeyFrame kf0( 0, VtValue(0.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf1( 10, VtValue(0.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf2( 20, VtValue(1.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf3( 30, VtValue(1.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf4( 40, VtValue(0.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf5( 50, VtValue(0.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );

    // Add a knot at time 0, value 0
    TF_AXIOM(tester.SetKeyFrame( kf0, GfInterval::GetFullInterval()));

    // Add a knot at time 10, value 0
    TF_AXIOM(tester.SetKeyFrame( kf1, GfInterval()));

    // Add a knot at time 20, value 1
    TF_AXIOM(tester.SetKeyFrame( kf2, GfInterval(10, inf, false, false)));

    // Add a knot at time 30, value 1
    TF_AXIOM(tester.SetKeyFrame( kf3, GfInterval()));

    // Add a knot at time 40, value 0
    TF_AXIOM(tester.SetKeyFrame( kf4, GfInterval(30, inf, false, false)));

    // Add a knot at time 50, value 0
    TF_AXIOM(tester.SetKeyFrame( kf5, GfInterval()));

    // Test adding redundant knots in static sections
    TsKeyFrame kf0_1( 5, VtValue(0.0), TsKnotBezier, 
                        VtValue(0.0), VtValue(0.0), 1, 1 );
    // Adding redundant flat knot shouldn't invalidate anything
    TF_AXIOM(tester.SetKeyFrame( kf0_1, GfInterval()));

    TsKeyFrame kf2_3( 25, VtValue(1.0), TsKnotBezier, 
                        VtValue(0.0), VtValue(0.0), 1, 1 );
    // Adding redundant flat knot shouldn't invalidate anything
    TF_AXIOM(tester.SetKeyFrame( kf2_3, GfInterval()));

    TsKeyFrame kf4_5( 45, VtValue(0.0), TsKnotBezier, 
                        VtValue(0.0), VtValue(0.0), 1, 1 );
    // Adding redundant flat knot shouldn't invalidate anything
    TF_AXIOM(tester.SetKeyFrame( kf4_5, GfInterval()));

    // Remove the redundant knots we just added
    // Removing redundant flat knot shouldn't invalidate anything
    TF_AXIOM(tester.RemoveKeyFrame( kf0_1.GetTime(), GfInterval()));

    // Removing redundant flat knot shouldn't invalidate anything
    TF_AXIOM(tester.RemoveKeyFrame( kf2_3.GetTime(), GfInterval()));

    // Removing redundant flat knot shouldn't invalidate anything
    TF_AXIOM(tester.RemoveKeyFrame( kf4_5.GetTime(), GfInterval()));

    // Change tangent lengths on each knot (flat segments shouldn't change 
    // while non flat segments should
    kf0.SetLeftTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf0, GfInterval()));
    kf0.SetRightTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf0, GfInterval()));

    kf1.SetLeftTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf1, GfInterval()));
    kf1.SetRightTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf1, GfInterval(10, 20, false, false)));

    kf2.SetLeftTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf2, GfInterval(10, 20, false, false)));

    kf2.SetRightTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf2, GfInterval()));

    kf3.SetLeftTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf3, GfInterval()));

    kf3.SetRightTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf3, GfInterval(30, 40, false, false)));

    kf4.SetLeftTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf4, GfInterval(30, 40, false, false)));
    kf4.SetRightTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf4, GfInterval()));

    kf5.SetLeftTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf5, GfInterval()));
    kf5.SetRightTangentLength(3);
    TF_AXIOM(tester.SetKeyFrame( kf5, GfInterval()));

    // Move the whole spline forward five frames
    TF_AXIOM(tester.RemoveKeyFrame(
               kf0.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf1.GetTime(), GfInterval(-inf, 20, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf2.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf3.GetTime(), GfInterval(-inf, 40, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf4.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf5.GetTime(), GfInterval::GetFullInterval()));
    kf0.SetTime(kf0.GetTime() + 5);
    kf1.SetTime(kf1.GetTime() + 5);
    kf2.SetTime(kf2.GetTime() + 5);
    kf3.SetTime(kf3.GetTime() + 5);
    kf4.SetTime(kf4.GetTime() + 5);
    kf5.SetTime(kf5.GetTime() + 5);
    TF_AXIOM(tester.SetKeyFrame( kf0, GfInterval::GetFullInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf1, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf2, GfInterval(15, inf, false, false)));
    TF_AXIOM(tester.SetKeyFrame( kf3, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf4, GfInterval(35, inf, false, false)));
    TF_AXIOM(tester.SetKeyFrame( kf5, GfInterval()));

    // Move the whole spline back five frames
    TF_AXIOM(tester.RemoveKeyFrame(
               kf5.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf4.GetTime(), GfInterval(35, inf, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf3.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf2.GetTime(), GfInterval(15, inf, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf1.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf0.GetTime(), GfInterval::GetFullInterval()));
    kf0.SetTime(kf0.GetTime() - 5);
    kf1.SetTime(kf1.GetTime() - 5);
    kf2.SetTime(kf2.GetTime() - 5);
    kf3.SetTime(kf3.GetTime() - 5);
    kf4.SetTime(kf4.GetTime() - 5);
    kf5.SetTime(kf5.GetTime() - 5);
    TF_AXIOM(tester.SetKeyFrame( kf5, GfInterval::GetFullInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf4, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf3, GfInterval(-inf, 40, false, false)));
    TF_AXIOM(tester.SetKeyFrame( kf2, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf1, GfInterval(-inf, 20, false, false)));
    TF_AXIOM(tester.SetKeyFrame( kf0, GfInterval()));

    // Change tangent slopes on the outer flat segments
    kf0.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame( kf0, GfInterval(0, 10, false, false)));
    kf4.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame( kf4, GfInterval(40, 50, false, false)));

    // Move the whole spline forward five frames again
    TF_AXIOM(tester.RemoveKeyFrame(
               kf0.GetTime(), GfInterval(0, 10, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf1.GetTime(), GfInterval(-inf, 20, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf2.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf3.GetTime(), GfInterval(-inf, 40, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf4.GetTime(), GfInterval(40, 50, false, false)));
    TF_AXIOM(tester.RemoveKeyFrame(
               kf5.GetTime(), GfInterval::GetFullInterval()));
    kf0.SetTime(kf0.GetTime() + 5);
    kf1.SetTime(kf1.GetTime() + 5);
    kf2.SetTime(kf2.GetTime() + 5);
    kf3.SetTime(kf3.GetTime() + 5);
    kf4.SetTime(kf4.GetTime() + 5);
    kf5.SetTime(kf5.GetTime() + 5);
    TF_AXIOM(tester.SetKeyFrame( kf0, GfInterval::GetFullInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf1, GfInterval(5, 15, false, false)));
    TF_AXIOM(tester.SetKeyFrame( kf2, GfInterval(15, inf, false, false)));
    TF_AXIOM(tester.SetKeyFrame( kf3, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf4, GfInterval(35, inf, false, false)));
    TF_AXIOM(tester.SetKeyFrame( kf5, GfInterval(45, 55, false, false)));

    printf("\tpassed\n");
}

void TestChangeIntervalsForMixedKnotEdits()
{
    printf("\nTest changed intervals for knot edits (mixed knot types)\n");

    _SplineTester tester = _SplineTester(TsSpline());

    TF_AXIOM(tester.spline.GetExtrapolation() ==
           std::make_pair(TsExtrapolationHeld, TsExtrapolationHeld));

    TsKeyFrame kf0( 0, VtValue(0.0), TsKnotHeld, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf1( 10, VtValue(0.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf2( 20, VtValue(0.0), TsKnotLinear, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf3( 30, VtValue(0.0), TsKnotHeld, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf4( 40, VtValue(0.0), TsKnotLinear, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf5( 50, VtValue(0.0), TsKnotBezier, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );
    TsKeyFrame kf6( 60, VtValue(0.0), TsKnotHeld, 
                      VtValue(0.0), VtValue(0.0), 1, 1 );

    // Add a knot at time 0, value 0
    TF_AXIOM(tester.SetKeyFrame( kf0, GfInterval::GetFullInterval()));

    // Add a knot at time 10, value 0
    TF_AXIOM(tester.SetKeyFrame( kf1, GfInterval()));

    // Add a knot at time 20, value 0
    TF_AXIOM(tester.SetKeyFrame( kf2, GfInterval()));

    // Add a knot at time 30, value 0
    TF_AXIOM(tester.SetKeyFrame( kf3, GfInterval()));

    // Add a knot at time 40, value 0
    TF_AXIOM(tester.SetKeyFrame( kf4, GfInterval()));

    // Add a knot at time 50, value 0
    TF_AXIOM(tester.SetKeyFrame( kf5, GfInterval()));

    // Add a knot at time 60, value 0
    TF_AXIOM(tester.SetKeyFrame( kf6, GfInterval()));


    // Move knots in time only
    TF_AXIOM(tester.RemoveKeyFrame( kf0.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame( kf1.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame( kf2.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame( kf3.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame( kf4.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame( kf5.GetTime(), GfInterval()));
    TF_AXIOM(tester.RemoveKeyFrame(
            kf6.GetTime(), GfInterval::GetFullInterval()));
    kf0.SetTime(kf0.GetTime() + 5);
    kf1.SetTime(kf1.GetTime() + 5);
    kf2.SetTime(kf2.GetTime() + 5);
    kf3.SetTime(kf3.GetTime() + 5);
    kf4.SetTime(kf4.GetTime() + 5);
    kf5.SetTime(kf5.GetTime() + 5);
    kf6.SetTime(kf6.GetTime() + 5);
    TF_AXIOM(tester.SetKeyFrame( kf0, GfInterval::GetFullInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf1, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf2, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf3, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf4, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf5, GfInterval()));
    TF_AXIOM(tester.SetKeyFrame( kf6, GfInterval()));

    // Current key frames
    // 5 : 0.0 (held)
    // 15: 0.0 (bezier)
    // 25: 0.0 (linear)
    // 35: 0.0 (held)
    // 45: 0.0 (linear)
    // 55: 0.0 (bezier)
    // 65: 0.0 (held)

    // Set tangent slopes
    kf0.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval()));
    kf0.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval()));
    kf0.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval()));
    kf0.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval()));

    kf1.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval()));
    kf1.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval()));
    kf1.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(15, 25, false, false)));
    kf1.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(15, 25, false, false)));

    kf2.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval()));
    kf2.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval()));
    kf2.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval()));
    kf2.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval()));

    kf3.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval()));
    kf3.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval()));
    kf3.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval()));
    kf3.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval()));

    kf4.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf4, GfInterval()));
    kf4.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf4, GfInterval()));
    kf4.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf4, GfInterval()));
    kf4.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf4, GfInterval()));

    kf5.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf5, GfInterval(45, 55, false, false)));
    kf5.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf5, GfInterval(45, 55, false, false)));
    kf5.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf5, GfInterval(55, 65, false, false)));
    kf5.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf5, GfInterval(55, 65, false, false)));

    kf6.SetLeftTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf6, GfInterval()));
    kf6.SetLeftTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf6, GfInterval()));
    kf6.SetRightTangentSlope(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf6, GfInterval()));
    kf6.SetRightTangentLength(2);
    TF_AXIOM(tester.SetKeyFrame(kf6, GfInterval()));

    // Set values
    kf0.SetValue(VtValue(1.0));
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(-inf, 15, false, false)));
    kf1.SetValue(VtValue(2.0));
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(15, 25, true, false)));
    kf2.SetValue(VtValue(3.0));
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(15, 35, false, false)));
    kf3.SetValue(VtValue(4.0));
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval(25, 45, false, false)));
    kf4.SetValue(VtValue(5.0));
    TF_AXIOM(tester.SetKeyFrame(kf4, GfInterval(45, 55, true, false)));
    kf5.SetValue(VtValue(6.0));
    TF_AXIOM(tester.SetKeyFrame(kf5, GfInterval(45, 65, false, false)));
    kf6.SetValue(VtValue(7.0));
    TF_AXIOM(tester.SetKeyFrame(kf6, GfInterval(55, inf, false, false)));

    // Change knot types
    kf0.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(5, 15, false, false)));
    kf0.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(5, 15, false, false)));
    kf0.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(5, 15, false, false)));
    kf0.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(5, 15, false, false)));
    kf0.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(5, 15, false, false)));
    kf0.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf0, GfInterval(5, 15, false, false)));

    kf1.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(15, 25, false, false)));
    kf1.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(15, 25, false, false)));
    kf1.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(15, 25, false, false)));
    kf1.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(15, 25, false, false)));
    kf1.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(15, 25, false, false)));
    kf1.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf1, GfInterval(15, 25, false, false)));

    kf2.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(15, 35, false, false)));
    kf2.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(15, 35, false, false)));
    kf2.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(15, 35, false, false)));
    kf2.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(15, 35, false, false)));
    kf2.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(15, 35, false, false)));
    kf2.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf2, GfInterval(15, 35, false, false)));

    kf3.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval(25, 45, false, false)));
    kf3.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval(25, 45, false, false)));
    kf3.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval(25, 45, false, false)));
    kf3.SetKnotType(TsKnotLinear);
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval(25, 45, false, false)));
    kf3.SetKnotType(TsKnotBezier);
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval(25, 45, false, false)));
    kf3.SetKnotType(TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(kf3, GfInterval(25, 45, false, false)));
}

void TestChangedIntervalHeld()
{
    printf("\nTest changed interval with held knot\n");

    TsSpline spline;
    spline.SetKeyFrame(TsKeyFrame(0, VtValue(1.0), TsKnotHeld));
    spline.SetKeyFrame(TsKeyFrame(1, VtValue(2.0), TsKnotHeld));
    spline.SetKeyFrame(TsKeyFrame(2, VtValue(3.0), TsKnotBezier));
    spline.SetKeyFrame(TsKeyFrame(3, VtValue(4.0), TsKnotBezier));

    _SplineTester tester = _SplineTester(spline);

    // Verify that we get the correct range when we change the value of the held
    // knot.  This should be the interval from the held knot to the next knot,
    // open at the end.  The open end means that the left value of the next knot
    // is affected, but the right value is not.
    const TsKeyFrame newKf(1, VtValue(2.5), TsKnotHeld);
    TF_AXIOM(tester.SetKeyFrame(newKf, GfInterval(1, 2, true, false)));
}

void
TestIteratorAPI()
{
    printf("\nTest iterator API\n");

    TsSpline spline;

    // Test initial conditions
    TF_AXIOM(spline.empty());
    TF_AXIOM(spline.begin() == spline.end());

    // Add a bunch of keyframes
    spline.SetKeyFrame(TsKeyFrame(1.0, 1.0));
    spline.SetKeyFrame(TsKeyFrame(3.0, 2.0));
    spline.SetKeyFrame(TsKeyFrame(7.0, 3.0));
    spline.SetKeyFrame(TsKeyFrame(10.0, 4.0));
    spline.SetKeyFrame(TsKeyFrame(15.0, 5.0));
    spline.SetKeyFrame(TsKeyFrame(20.0, 6.0));

    // Test basic container emptyness/size
    TF_AXIOM(spline.size() == 6);
    TF_AXIOM(!spline.empty());
    TF_AXIOM(spline.begin() != spline.end());

    // Test finding iterators at a specific time
    TF_AXIOM(spline.find(3.0) != spline.end());
    TF_AXIOM(spline.find(4.0) == spline.end());
    TF_AXIOM(++spline.find(7.0) == spline.find(10.0));
    TF_AXIOM(--spline.find(7.0) == spline.find(3.0));
    
    // Test lower_bound
    TF_AXIOM(spline.lower_bound(25.0) == spline.end());
    TF_AXIOM(spline.lower_bound(3.0) == spline.find(3.0));
    TF_AXIOM(spline.lower_bound(4.0) == spline.find(7.0));
    TF_AXIOM(spline.lower_bound(-inf) == spline.begin());
    TF_AXIOM(spline.lower_bound(+inf) == spline.end());
    
    // Test upper_bound
    TF_AXIOM(spline.upper_bound(3.0) == spline.find(7.0));
    TF_AXIOM(spline.upper_bound(4.0) == spline.find(7.0));
    TF_AXIOM(spline.upper_bound(25.0) == spline.end());
    TF_AXIOM(spline.upper_bound(-inf) == spline.begin());
    TF_AXIOM(spline.upper_bound(+inf) == spline.end());
        
    // Test dereferencing
    TF_AXIOM(spline.find(7.0)->GetValue().Get<double>() == 3.0);
    TF_AXIOM(spline.find(10.0)->GetValue().Get<double>() == 4.0);

    // Clear and re-test initial conditions
    spline.Clear();
    TF_AXIOM(spline.empty());
    TF_AXIOM(spline.begin() == spline.end());

    printf("\tpassed\n");
}

void
TestSwapKeyFrames()
{
    printf("\nTest SwapKeyFrames\n");

    TsSpline spline;
    std::vector<TsKeyFrame> keyFrames;
    TsKeyFrame kf0(0.0, 0.0);
    TsKeyFrame kf1(1.0, 5.0);
    TsKeyFrame kf1Second(1.0, 9.0);

    // Test trivial case - both empty
    spline.SwapKeyFrames(&keyFrames);

    TF_AXIOM(spline.empty());
    TF_AXIOM(keyFrames.empty());

    // Test empty spline, single item vector
    keyFrames.push_back(kf0);
    spline.SwapKeyFrames(&keyFrames);
    TF_AXIOM(spline.size() == 1);
    TF_AXIOM(spline.find(0.0) != spline.end());
    TF_AXIOM(keyFrames.empty());

    // Test empty vector, single item spline
    spline.SwapKeyFrames(&keyFrames);
    TF_AXIOM(spline.empty());
    TF_AXIOM(keyFrames.size() == 1);

    // Test items in both, including a frame in each at
    // same frame
    spline.SetKeyFrame(kf0);
    spline.SetKeyFrame(kf1);
    keyFrames.clear();
    keyFrames.push_back(kf1Second);
    TF_AXIOM(spline.size() == 2);
    TF_AXIOM(spline.find(0.0) != spline.end());
    TF_AXIOM(spline.find(1.0) != spline.end());
    TF_AXIOM(spline.find(1.0)->GetValue().Get<double>() == 5.0);
    TF_AXIOM(keyFrames.size() == 1);

    spline.SwapKeyFrames(&keyFrames);
    TF_AXIOM(spline.size() == 1);
    TF_AXIOM(keyFrames.size() == 2);
    TF_AXIOM(spline.find(1.0)->GetValue().Get<double>() == 9.0);
}

int
main(int argc, char **argv)
{
    TsSpline val;
    TsKeyFrame kf;

    TsTypeRegistry &typeRegistry = TsTypeRegistry::GetInstance();
    
    printf("\nTest supported types\n");
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<double>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<float>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<int>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<bool>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<GfVec2d>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<GfVec2f>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<GfVec3d>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<GfVec3f>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<GfVec4d>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<GfVec4f>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<GfMatrix2d>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<GfMatrix3d>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<GfMatrix4d>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find<string>()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find< VtArray<double> >()));
    TF_AXIOM(typeRegistry.IsSupportedType(TfType::Find< VtArray<float> >()));
    TF_AXIOM(!typeRegistry.IsSupportedType(TfType::Find<char>()));
    TF_AXIOM(!typeRegistry.IsSupportedType(TfType::Find<GfRange1d>()));
    printf("\tpassed\n");

    printf("\nTest that setting left value of an uninterpolatable knot does "
        "not work:\n\t\terror expected\n");
    kf = TsKeyFrame(0.0, string("foo"));
    TF_AXIOM( kf.GetValue().Get<string>() == "foo" );
    kf.SetLeftValue( VtValue("bar") );
    TF_AXIOM( kf.GetValue().Get<string>() == "foo" );
    printf("\tpassed\n");

    printf("\nTest that setting left value of non-dual valued knot does not "
        "work:\n\t\terror expected\n");
    kf = TsKeyFrame(0.0, 1.0);
    TF_AXIOM( kf.GetValue().Get<double>() == 1.0 );
    kf.SetLeftValue( VtValue(123.0) );
    TF_AXIOM( kf.GetValue().Get<double>() == 1.0 );
    printf("\tpassed\n");

    printf("\nTest that initializing a keyframe with an unsupported knot type "
        "for the given value type causes a supported knot type to be used\n");
    // GfVec2d is interpolatable but does not support tangents. Expect Linear.
    kf = TsKeyFrame(0.0, GfVec2d(0), TsKnotBezier);
    TF_AXIOM(kf.GetKnotType() == TsKnotLinear);

    // std::string is neither interpolatable nor support tangents. Expect Held.
    kf = TsKeyFrame(0.0, std::string(), TsKnotBezier);
    TF_AXIOM(kf.GetKnotType() == TsKnotHeld);
    
    printf("\nTest removing bogus keyframe: errors expected\n");
    val.Clear();
    val.RemoveKeyFrame( 123 );
    printf("\tpassed\n");

    printf("\nTest creating non-held dual-value keyframe for "
           "non-interpolatable type\n");
    kf = TsKeyFrame(0.0, string("left"), string("right"), TsKnotLinear);
    TF_AXIOM(kf.GetKnotType() == TsKnotHeld);

    printf("\nTest interpolation of float\n");
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0, float(0), TsKnotLinear) );
    val.SetKeyFrame( TsKeyFrame(10, float(20), TsKnotLinear) );
    TF_AXIOM( val.Eval(5).Get<float>() == float(10) );
    TF_AXIOM( val.Eval(5.5).Get<float>() == float(11) );
    TF_AXIOM( val.EvalDerivative(5, TsLeft).Get<float>() == float(2) );
    TF_AXIOM( val.EvalDerivative(5, TsRight).Get<float>() == float(2) );
    TF_AXIOM( val.EvalDerivative(5.5, TsLeft).Get<float>() == float(2) );
    TF_AXIOM( val.EvalDerivative(5.5, TsRight).Get<float>() == float(2) );
    printf("\tpassed\n");

    // Coverage for breakdown of float
    GfInterval affectedRange;
    val.Breakdown(5, TsKnotBezier, false, 1.0, VtValue(), &affectedRange);

    // Coverage for constructor
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0, float(0), TsKnotLinear) );
    val.SetKeyFrame( TsKeyFrame(10, float(20), TsKnotLinear) );
    TF_AXIOM( val.Eval(5).Get<float>() == float(10) );
    TF_AXIOM( val.Eval(5.5).Get<float>() == float(11) );
    TF_AXIOM( val.EvalDerivative(0, TsLeft).Get<float>() == float(0) );
    TF_AXIOM( val.EvalDerivative(0, TsRight).Get<float>() == float(2) );
    TF_AXIOM( val.EvalDerivative(5, TsLeft).Get<float>() == float(2) );
    TF_AXIOM( val.EvalDerivative(5, TsRight).Get<float>() == float(2) );
    TF_AXIOM( val.EvalDerivative(5.5, TsLeft).Get<float>() == float(2) );
    TF_AXIOM( val.EvalDerivative(5.5, TsRight).Get<float>() == float(2) );
    TF_AXIOM( val == TsSpline(val.GetKeyFrames()) );
    TF_AXIOM( val != TsSpline() );

    // Coverage for operator<<().
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0, float(0), TsKnotLinear) );
    val.SetKeyFrame( TsKeyFrame(10, float(20), TsKnotLinear) );
    TF_AXIOM( !TfStringify(val).empty() );

    // Coverage for float types
    printf("\nTest GetRange() of float\n");
    std::pair<VtValue, VtValue> range = val.GetRange(-1, 11);
    TF_AXIOM( range.first.Get<float>() == 0.0 );
    TF_AXIOM( range.second.Get<float>() == 20.0 );
    printf("\tpassed\n");

    printf("\nTest interpolation of int\n");
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0, int(0), TsKnotHeld) );
    val.SetKeyFrame( TsKeyFrame(10, int(20), TsKnotHeld) );
    TF_AXIOM( val.Eval(5).Get<int>() == int(0) );
    TF_AXIOM( val.EvalDerivative(5, TsLeft).Get<int>() == int(0) );
    TF_AXIOM( val.EvalDerivative(5, TsRight).Get<int>() == int(0) );
    printf("\tpassed\n");

    printf("\nTest construction of various types of keyframes\n");
    TsKeyFrame(0, VtValue( double(0.123) ) );
    TsKeyFrame(0, VtValue( float(0.123) ) );
    TsKeyFrame(0, VtValue( int(0) ) );
    // For code coverage of unknown types
    kf = TsKeyFrame(0, 0.123, /* bogus knot type */ TsKnotType(-12345) );
    printf("\t%s\n", TfStringify(kf).c_str());
    printf("\tpassed\n");

    printf("\nTest querying left side of non-first held keyframe\n");
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0, VtValue( "foo" )) );
    val.SetKeyFrame( TsKeyFrame(1, VtValue( "bar" )) );
    val.SetKeyFrame( TsKeyFrame(2, VtValue( "mangoes" )) );
    val.SetKeyFrame( TsKeyFrame(3, VtValue( "apples" )) );
    val.SetKeyFrame( TsKeyFrame(4, VtValue( "oranges" )) );
    TF_AXIOM( val.Eval(1, TsLeft) == "foo" );
    TF_AXIOM( val.EvalDerivative(1, TsLeft) == "" );
    printf("\tpassed\n");

    printf("\nTests for code coverage: errors expected\n");
    GfVec2d vec2dEps = GfVec2d(std::numeric_limits<double>::epsilon(),
                               std::numeric_limits<double>::epsilon());
    val.Clear();
    val.SetKeyFrame( TsKeyFrame( 0, GfVec2d(0, 0), TsKnotHeld ) );
    val.SetKeyFrame( TsKeyFrame( 10, GfVec2d(1, 1), TsKnotHeld ) );
    TF_AXIOM(_IsClose(VtValue(val.Eval(0, TsLeft)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.Eval(0, TsRight)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.Eval(1, TsLeft)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.Eval(1, TsRight)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.EvalDerivative(0, TsLeft)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.EvalDerivative(0, TsRight)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.EvalDerivative(1, TsLeft)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.EvalDerivative(1, TsRight)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));

    val.Clear();
    val.SetKeyFrame( TsKeyFrame( 0, GfVec2d(0, 0), TsKnotLinear ) );
    val.SetKeyFrame( TsKeyFrame( 10, GfVec2d(1, 1), TsKnotLinear ) );
    TF_AXIOM(_IsClose(VtValue(val.Eval(0, TsLeft)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.Eval(0, TsRight)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.Eval(1, TsLeft)), 
                    VtValue(GfVec2d(0.1, 0.1)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.Eval(1, TsRight)), 
                    VtValue(GfVec2d(0.1, 0.1)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.EvalDerivative(0, TsLeft)), 
                    VtValue(GfVec2d(0.0, 0.0)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.EvalDerivative(0, TsRight)), 
                    VtValue(GfVec2d(0.1, 0.1)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.EvalDerivative(1, TsLeft)), 
                    VtValue(GfVec2d(0.1, 0.1)), vec2dEps));
    TF_AXIOM(_IsClose(VtValue(val.EvalDerivative(1, TsRight)), 
                    VtValue(GfVec2d(0.1, 0.1)), vec2dEps));

    val.Clear();
    val.SetKeyFrame( TsKeyFrame( 0, 0.0, TsKnotHeld ) );
    val.SetKeyFrame( TsKeyFrame( 10, 10.0, TsKnotHeld ) );
    TF_AXIOM(_IsClose<double>(val.Eval(0,  TsRight).Get<double>(), 0.0));
    TF_AXIOM(_IsClose<double>(val.Eval(0,  TsLeft).Get<double>(),  0.0));
    TF_AXIOM(_IsClose<double>(val.Eval(10, TsRight).Get<double>(), 10.0));
    TF_AXIOM(_IsClose<double>(val.Eval(10, TsLeft).Get<double>(),  0.0));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(0,  TsRight).Get<double>(), 0.0));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(0,  TsLeft).Get<double>(),  0.0));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(10, TsRight).Get<double>(), 0.0));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(10, TsLeft).Get<double>(),  0.0));
    printf("\tpassed\n");

    val.Clear();
    val.SetKeyFrame( TsKeyFrame( 0, double(0.0), TsKnotLinear ) );
    val.SetKeyFrame( TsKeyFrame( 10, double(10.0), TsKnotLinear ) );
    TF_AXIOM(_IsClose<double>(val.Eval(0,  TsRight).Get<double>(), 0.0));
    TF_AXIOM(_IsClose<double>(val.Eval(0,  TsLeft).Get<double>(),  0.0));
    TF_AXIOM(_IsClose<double>(val.Eval(10, TsRight).Get<double>(), 10.0));
    TF_AXIOM(_IsClose<double>(val.Eval(10, TsLeft).Get<double>(),  10.0));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(0,  TsRight).Get<double>(), 1.0));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(0,  TsLeft).Get<double>(),  0.0));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(10, TsRight).Get<double>(), 0.0));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(10, TsLeft).Get<double>(),  1.0));
    printf("\tpassed\n");

    val.Clear();
    val.SetKeyFrame( TsKeyFrame( 0, VtValue(0.0), TsKnotLinear ) );
    val.SetKeyFrame( TsKeyFrame( 10, VtValue(10.0), TsKnotLinear ) );
    TF_AXIOM(_IsClose<double>(val.Eval(0,  TsRight), VtValue(0.0)));
    TF_AXIOM(_IsClose<double>(val.Eval(0,  TsLeft),  VtValue(0.0)));
    TF_AXIOM(_IsClose<double>(val.Eval(10, TsRight), VtValue(10.0)));
    TF_AXIOM(_IsClose<double>(val.Eval(10, TsLeft),  VtValue(10.0)));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(0,  TsRight), VtValue(1.0)));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(0,  TsLeft),  VtValue(0.0)));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(10, TsRight), VtValue(0.0)));
    TF_AXIOM(_IsClose<double>(val.EvalDerivative(10, TsLeft),  VtValue(1.0)));
    printf("\tpassed\n");

    // Test evaluation of cached segments.
    TestEvaluator();

    // Test spline diffing
    TestSplineDiff();
    TestSplineDiff2();
    TestHeldThenBezier();

    // Test redundant knot detection
    TestRedundantKnots();
    
    // Test intervals generated when assigning new splines.
    TestChangeIntervalsOnAssignment();

    // Test change intervals for edits.
    TestChangeIntervalsForKnotEdits();
    TestChangeIntervalsForKnotEdits2();
    TestChangeIntervalsForMixedKnotEdits();
    TestChangedIntervalHeld();

    // Sample to within this error tolerance
    static const double tolerance = 1.0e-3;

    // Maximum allowed error is not tolerance, it's much larger.  This
    // is because Eval() samples differently between frames than at
    // frames and will yield slightly incorrect results but avoid
    // problems with large derivatives.  Sample() does not do that.
    static const double maxError  = 0.15;
    
    TsSamples samples;

    // Can't test from Python since we can't set float knots.
    printf("\nTest float Sample() with held knots\n");
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0, float(0.0), TsKnotHeld) );
    val.SetKeyFrame( TsKeyFrame(10, float(10.0), TsKnotHeld) );
    samples = val.Sample(-1, 11, 1.0, 1.0, tolerance);
    _AssertSamples<float>(val, samples, -1, 11, maxError);
    // Test sampling out of range
    samples = val.Sample(-300, -200, 1.0, 1.0, tolerance);
    _AssertSamples<float>(val, samples, -300, -200, maxError);
    samples = val.Sample(300, 400, 1.0, 1.0, tolerance);
    _AssertSamples<float>(val, samples, 300, 400, maxError);
    printf("\tpassed\n");

    printf("\nTest float Eval() on left of keyframe with held knots\n");
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0.5, float(0.0), TsKnotHeld) );
    val.SetKeyFrame( TsKeyFrame(5.5, float(5.0), TsKnotHeld) );
    val.SetKeyFrame( TsKeyFrame(10.5, float(10.0), TsKnotHeld) );
    TF_AXIOM( val.Eval(5.5, TsLeft).Get<float>() == float(0.0) );
    TF_AXIOM( val.EvalDerivative(5.5, TsLeft).Get<float>() == float(0.0) );
    TF_AXIOM( val.EvalDerivative(5.5, TsRight).Get<float>() == float(0.0) );
    printf("\tpassed\n");

    printf("\nTest double tangent symmetry\n");
    kf = TsKeyFrame(0.0, 0.0, TsKnotBezier, 1.0, 1.0, 1.0, 1.0);
    TF_AXIOM(!kf.GetTangentSymmetryBroken());
    kf = TsKeyFrame(0.0, 0.0, TsKnotBezier, 1.0, 1.0, 1.0, 2.0);
    TF_AXIOM(!kf.GetTangentSymmetryBroken());
    kf = TsKeyFrame(0.0, 0.0, TsKnotBezier, 1.0, 1.1, 1.0, 1.0);
    TF_AXIOM(kf.GetTangentSymmetryBroken());
    TF_AXIOM(kf.GetLeftTangentSlope() != kf.GetRightTangentSlope());
    kf.SetTangentSymmetryBroken(false);
    TF_AXIOM(!kf.GetTangentSymmetryBroken());
    TF_AXIOM(kf.GetLeftTangentSlope() == kf.GetRightTangentSlope());
    printf("\tpassed\n");

    printf("\nTest float tangent symmetry\n");
    kf = TsKeyFrame(0.0f, 0.0f, TsKnotBezier, 1.0f, 1.0f, 1.0, 1.0);
    TF_AXIOM(!kf.GetTangentSymmetryBroken());
    kf = TsKeyFrame(0.0f, 0.0f, TsKnotBezier, 1.0f, 1.0f, 1.0, 2.0);
    TF_AXIOM(!kf.GetTangentSymmetryBroken());
    kf = TsKeyFrame(0.0f, 0.0f, TsKnotBezier, 1.0f, 1.1f, 1.0, 1.0);
    TF_AXIOM(kf.GetTangentSymmetryBroken());
    TF_AXIOM(kf.GetLeftTangentSlope() != kf.GetRightTangentSlope());
    kf.SetTangentSymmetryBroken(false);
    TF_AXIOM(!kf.GetTangentSymmetryBroken());
    TF_AXIOM(kf.GetLeftTangentSlope() == kf.GetRightTangentSlope());
    printf("\tpassed\n");

    // Coverage for ResetTangentSymmetryBroken
    kf = TsKeyFrame(0.0f, string("foo"), TsKnotHeld);
    kf.ResetTangentSymmetryBroken();

    // Coverage tests for blur samples
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0, 0.0, TsKnotBezier,
                                   1.0, -1.0, 0.5, 0.5) );
    val.SetKeyFrame( TsKeyFrame(5 - 2.0 * tolerance, 50.0, TsKnotBezier,
                                   0.0, 0.0, 0.5, 0.5) );
    val.SetKeyFrame( TsKeyFrame(5, 5.0, TsKnotBezier,
                                   0.0, 0.0, 0.5, 0.5) );
    val.SetKeyFrame( TsKeyFrame(5 + 0.5 * tolerance, 10.0, TsKnotBezier,
                                   -1.0, 1.0, 0.5, 0.5) );
    samples = val.Sample(-1, 16, 1.0, 1.0, tolerance);
    val.Clear();

    // Get a blur sample due to closely spaced keyframes.
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0, 0.0, TsKnotBezier,
                                  -1e9, 1e9,
                                  tolerance/2.0, tolerance/2.0) );
    val.SetKeyFrame( TsKeyFrame(1e-3, 1.0, TsKnotBezier,
                                  -1e9, 1e9,
                                  tolerance/2.0, tolerance/2.0) );
    samples = val.Sample(-1.0, 1.0,
                         1.0, 1.0, 1e-9);

    // Coverage of segment blending
    val.Clear();
    val.SetKeyFrame( TsKeyFrame(0, 0.0, TsKnotLinear,
                                  -1e9, 1e9,
                                  tolerance/2.0, tolerance/2.0) );
    val.SetKeyFrame( TsKeyFrame(1e-3, 1.0, TsKnotBezier,
                                  -1e9, 1e9,
                                  tolerance/2.0, tolerance/2.0) );
    samples = val.Sample(-1.0, 1.0,
                         1.0, 1.0, 1e-9);

    // Coverage of degenerate/extreme tangent handles
    val.Clear();
    // Long tangent handles
    val.SetKeyFrame( TsKeyFrame(0, 0.0, TsKnotBezier,
                                  0.0, 0.0,
                                  10.0, 10.0) );
    // 0-length tangent handles
    val.SetKeyFrame( TsKeyFrame(1, 0.0, TsKnotBezier,
                                  0.0, 0.0,
                                  0.0, 0.0) );
    samples = val.Sample(-1.0, 2.0,
                         1.0, 1.0, 1e-9);

    string testStrValue("some_string_value");

    // Coverage for OstreamMethods
    std::ostringstream ss;
    kf = TsKeyFrame(0.0f, 0.0f, TsKnotBezier, 1.0f, 1.1f, 1.0, 1.0);
    ss << kf;
    kf = TsKeyFrame(0.0f, testStrValue, TsKnotHeld);
    ss << kf;

    // Coverage for operator==()
    TsKeyFrame kf1 = TsKeyFrame(0.0f, testStrValue, TsKnotHeld);
    TsKeyFrame kf2 = TsKeyFrame(0.0f, testStrValue, TsKnotHeld);
    // Different but equal objects, to bypass the *lhs==*rhs test
    // in operator==()
    assert (kf1 == kf2);

    TestIteratorAPI();

    // Verify TsFindChangedInterval behavior for dual-valued knots
    {
        TsSpline s1;
        s1.SetKeyFrame( TsKeyFrame(1.0, VtValue(-1.0), VtValue(1.0),
                                     TsKnotLinear,
                                     VtValue(0.9), VtValue(0.9),
                                     TsTime(1.0), TsTime(1.0)) );

        TsSpline s2;
        s2.SetKeyFrame( TsKeyFrame(1.0, VtValue(14.0), VtValue(1.0),
                                     TsKnotLinear,
                                     VtValue(0.9), VtValue(0.9),
                                     TsTime(1.0), TsTime(1.0)) );

        _SplineTester tester = _SplineTester(TsSpline());

        // 2 splines with a single dual-valued knot at 1.0 that differs on the
        // left-side value should be detected as different over (-inf, 1.0]
        tester = _SplineTester(s1);
        TF_VERIFY(tester.SetValue(
                      s2, GfInterval(-inf, 1.0, false, true)));

        s1.SetKeyFrame( TsKeyFrame(1.0, VtValue(1.0), VtValue(-14.0),
                                     TsKnotLinear,
                                     VtValue(0.9), VtValue(0.9),
                                     TsTime(1.0), TsTime(1.0)) );
        s2.SetKeyFrame( TsKeyFrame(1.0, VtValue(1.0), VtValue(-1.0),
                                     TsKnotLinear,
                                     VtValue(0.9), VtValue(0.9),
                                     TsTime(1.0), TsTime(1.0)) );

        // 2 splines with a single dual-valued knot at 1.0 that differs on the
        // right-side value should be detected as different over [1.0, inf)
        tester = _SplineTester(s1);
        TF_VERIFY(tester.SetValue(
                      s2, GfInterval(1.0, inf, true, false)));
    }

    // Verify TsFindChangedInterval behavior in the presence of
    // redundant held knots.
    {
        TsSpline held1;
        held1.SetKeyFrame( TsKeyFrame(1.0, VtValue(1.0), TsKnotHeld) );
        held1.SetKeyFrame( TsKeyFrame(3.0, VtValue(1.0), TsKnotHeld) );
        held1.SetKeyFrame( TsKeyFrame(12.0, VtValue(1.0), TsKnotHeld) );

        TsSpline held2;
        held2.SetKeyFrame( TsKeyFrame(1.0, VtValue(1.0), TsKnotHeld) );
        held2.SetKeyFrame( TsKeyFrame(3.0, VtValue(1.0), TsKnotHeld) );
        held2.SetKeyFrame( TsKeyFrame(12.0, VtValue(1.0), TsKnotHeld) );
        held2.SetKeyFrame( TsKeyFrame(6.0, VtValue(2.0), TsKnotHeld) );

        // Authoring a new knot in the middle of 2 redundant held knots should
        // invalidate the interval between the new knot and the next authored
        // knot.
        _SplineTester tester = _SplineTester(held1);
        TF_VERIFY(tester.SetValue(
                      held2, GfInterval(6.0, 12.0, true, false)));
    }

    // Test spline invalidation against flat single knot splines (essentially
    // default value invalidation).
    {
        TsSpline spline;

        spline.Clear();
        // All held knots, flat spline by default
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(1.0), TsKnotHeld) );
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(1.0), TsKnotHeld) );
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(1.0), TsKnotHeld) );
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(1.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));

        // Set first knot to a different value 0
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(0.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(-inf, 20, false, false)));
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(0.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(-inf, 20, false, false)));
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(0.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(-inf, 20, false, false)));
        // Set first knot to dual valued, left 0, right 1
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(0.0), VtValue(1.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(-inf, 10, false, true)));
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(0.0), VtValue(1.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(-inf, 10, false, true)));
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(0.0), VtValue(1.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(-inf, 10, false, true)));
        // Set first knot to dual valued, left 1, right 0
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(1.0), VtValue(0.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(10, 20, true, false)));
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(1.0), VtValue(0.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(10, 20, true, false)));
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(1.0), VtValue(0.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(10, 20, true, false)));
        // Set first knot to dual valued, both 1
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(1.0), VtValue(1.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(1.0), VtValue(1.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));
        spline.SetKeyFrame( TsKeyFrame(10.0, VtValue(1.0), VtValue(1.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));

        // Set second knot to a different value 0
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(0.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(10, 30, false, false)));
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(0.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(10, 30, false, false)));
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(0.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(10, 30, false, false)));
        // Set second knot to dual valued, left 0, right 1
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(0.0), VtValue(1.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(10, 20, false, true)));
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(0.0), VtValue(1.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(10, 20, false, true)));
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(0.0), VtValue(1.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(10, 20, false, true)));
        // Set second knot to dual valued, left 1, right 0
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(1.0), VtValue(0.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(20, 30, true, false)));
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(1.0), VtValue(0.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(20, 30, true, false)));
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(1.0), VtValue(0.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(20, 30, true, false)));
        // Set second knot to dual valued, both 1
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(1.0), VtValue(1.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(1.0), VtValue(1.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));
        spline.SetKeyFrame( TsKeyFrame(20.0, VtValue(1.0), VtValue(1.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));

        // Set third knot to a different value 0
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(0.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(20, 40, false, false)));
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(0.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(20, 40, false, false)));
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(0.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(20, 40, false, false)));
        // Set third knot to dual valued, left 0, right 1
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(0.0), VtValue(1.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(20, 30, false, true)));
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(0.0), VtValue(1.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(20, 30, false, true)));
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(0.0), VtValue(1.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(20, 30, false, true)));
        // Set third knot to dual valued, left 1, right 0
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(1.0), VtValue(0.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(30, 40, true, false)));
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(1.0), VtValue(0.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(30, 40, true, false)));
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(1.0), VtValue(0.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(30, 40, true, false)));
        // Set third knot to dual valued, both 1
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(1.0), VtValue(1.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(1.0), VtValue(1.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));
        spline.SetKeyFrame( TsKeyFrame(30.0, VtValue(1.0), VtValue(1.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));

        // Set last knot to a different value 0
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(0.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(30, inf, false, false)));
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(0.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(30, inf, false, false)));
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(0.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(30, inf, false, false)));
        // Set last knot to dual valued, left 0, right 1
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(0.0), VtValue(1.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(30, 40, false, true)));
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(0.0), VtValue(1.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(30, 40, false, true)));
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(0.0), VtValue(1.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(30, 40, false, true)));
        // Set last knot to dual valued, left 1, right 0
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(1.0), VtValue(0.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(40, inf, true, false)));
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(1.0), VtValue(0.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(40, inf, true, false)));
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(1.0), VtValue(0.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval(40, inf, true, false)));
        // Set last knot to dual valued, both 1
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(1.0), VtValue(1.0), TsKnotHeld) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(1.0), VtValue(1.0), TsKnotLinear) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));
        spline.SetKeyFrame( TsKeyFrame(40.0, VtValue(1.0), VtValue(1.0), TsKnotBezier) );
        TF_AXIOM(_TestSetSingleValueSplines(spline, VtValue(1.0), GfInterval()));
    }

    TestSwapKeyFrames();

    printf("\nTest SUCCEEDED\n");
    return 0;
}
