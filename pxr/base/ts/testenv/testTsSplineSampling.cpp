//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/knot.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/ts/tsTest_Museum.h"
#include "pxr/base/ts/tsTest_TsEvaluator.h"

#include <iostream>
#include <fstream>

PXR_NAMESPACE_USING_DIRECTIVE

static int testCase = 0;
static bool verbose = true;

using Polyline = std::vector<GfVec2d>;
using Polylines = std::vector<Polyline>;

static
bool
FindPolyline(const double t,
             const Polylines& polylines,
             GfVec2d* v0,
             GfVec2d* v1)
{
    // Search through the polylines. Remember that due to value-blocks, there
    // may be gaps between polylines (but never within one).
    for (const Polyline& polyline : polylines)
    {
        if (t < polyline[0][0]) {
            // There must be a value-block because we've gone past t without
            // hiting it.
            return false;
        }

        if (t > polyline.back()[0]) {
            // The entire polyline is before t
            continue;
        }

        // We found it. t >= the front and <= the back. Find the specific
        // segment.
        for (size_t i = 1; i < polyline.size(); ++i)
        {
            if (polyline[i][0] >= t) {
                *v0 = polyline[i-1];
                *v1 = polyline[i];
                return true;
            }
        }
    }

    // All the polylines were before t
    return false;
}

// Return distance squared from a point to a line segment
static
double
PointToSegmentDistanceSq(const GfVec2d& point,
                         const GfVec2d& v0,
                         const GfVec2d& v1)
{
    const double segLengthSq = (v1 - v0).GetLengthSq();
    if (segLengthSq == 0) {
        // v0 and v1 are the same.
        return (point - v0).GetLengthSq();
    }
    const double alpha = GfClamp(GfDot(point - v0, v1 - v0) / segLengthSq,
                                 0.0, 1.0);
    GfVec2d closestPt = GfLerp(alpha, v0, v1);
    return (point - closestPt).GetLengthSq();
}

// Verify that points along the spline are all near polylines
static
bool
VerifySpline(std::ostream& out,
             const TsSpline& spline,
             const GfInterval& timeInterval,
             const Polylines& polylines,
             double timeScale,
             double valueScale,
             double tolerance)
{
    const double toleranceSq = tolerance * tolerance;
    const double t0 = timeInterval.GetMin();
    const double t1 = timeInterval.GetMax();
    constexpr int testPoints = 101;

    for (int i = 0; i < testPoints; ++i)
    {
        const double alpha = double(i) / (testPoints - 1);
        const double t = GfLerp(alpha, t0, t1);

        GfVec2d v0, v1;
        bool hasPolyline = FindPolyline(t, polylines, &v0, &v1);

        double v;
        bool hasValue;
        if (hasPolyline) {
            const double deltaT = v1[0] - v0[0];
            const double lineSegAlpha = (deltaT > 0)
                                        ? (t - v0[0]) / deltaT
                                        : 0.0;
            if (lineSegAlpha > 0.5) {
                // We're close to the end of the segment, use
                // the preValue to ensure that the value we
                // get is for this segment (and not the next,
                // disjoint one).
                hasValue = spline.EvalPreValue(t, &v);
            } else {
                hasValue = spline.Eval(t, &v);
            }
        } else {
            hasValue = spline.Eval(t, &v);
        }

        if (hasValue != hasPolyline) {
            // We have a spline value at t but not polyline or a polyline at t
            // but no spline value.
            const char* const dataErr = (hasValue
                                         ? "spline value but no sample"
                                         : "sample but no spline value");
            std::ostringstream msg;
            msg.precision(std::numeric_limits<double>::digits10+1);
            msg << "Error: There was a "
                << dataErr
                << " at time = "
                << t;

            // Send message to both the output and stderr so we have it in
            // context and in the error output.
            out << msg.str() << std::endl;
            std::cerr << msg.str() << std::endl;

            // Give up on this spline, it failed.
            return false;
        }

        if (!hasValue) {
            continue;
        }

        // Convert the points to error tolerance space.
        const GfVec2d toleranceScales(timeScale, valueScale);

        const GfVec2d splinePt = GfCompMult(GfVec2d(t, v), toleranceScales);
        v0 = GfCompMult(v0, toleranceScales);
        v1 = GfCompMult(v1, toleranceScales);

        const double distSq = PointToSegmentDistanceSq(splinePt, v0, v1);
        if (distSq > toleranceSq) {
            // This point is not close to a segment.
            std::ostringstream msg;
            msg.precision(std::numeric_limits<double>::digits10+1);
            msg << "Error: The spline point at time = "
                << t
                << " is not close to the sampled spline.\n"
                << "    distance = "
                << std::sqrt(distSq)
                << " (in tolerance space).";

            // Send message to both the output and stderr so we have it in
            // context and in the error output.
            out << msg.str() << std::endl;
            std::cerr << msg.str() << std::endl;

            // Give up on this spline, it failed.
            return false;
        }
    }

    return true;
}

// Verify that every polyline produced is near the spline.
//
// For each polyline, verify that the points at each 1/4 of the polyline
// are within tolerance of the spline.
static bool VerifySamples(std::ostream& out,
                  const TsSpline& spline,
                  const Polylines& polylines,
                  double timeScale,
                  double valueScale,
                  double tolerance)
{
    const double toleranceSq = tolerance * tolerance;
    const GfVec2d toleranceScales(timeScale, valueScale);

    // Verify that each polyline is near the spline.
    for (size_t i = 0; i < polylines.size(); ++i) {
        const Polyline& polyline = polylines[i];
        for (size_t j = 0; j < polyline.size() - 1; ++j) {
            const GfVec2d& prev = polyline[j];
            const GfVec2d& next = polyline[j + 1];

            GfVec2d samplePts[5];
            GfVec2d splinePts[5];
            for (int k = 0; k < 5; ++k) {
                const double u = k / 4.0;
                samplePts[k] = GfLerp(u, prev, next);
                splinePts[k] = samplePts[k];
                if (!spline.Eval(splinePts[k][0], &splinePts[k][1])) {
                    // Output to both out and cerr so the message is both
                    // in context and highlighted.
                    std::ostringstream msg;
                    msg << "Error: Failed to eval spline at time "
                        << samplePts[k][0];
                    out << msg.str() << std::endl;
                    std::cerr << msg.str() << std::endl;

                    // Give up, we failed
                    return false;
                }
            }

            // Measure the error to each point
            //
            // It's tempting to just check the "vertical" distance in the values
            // but that's not the error tolerance contract.  What we may need to
            // find is the closest point on the spline, not the point vertically
            // above or below the sampled polyline.
            //
            // Finding the perpendicular distance from the line segment
            // to the spline involves changing coordinate systems and
            // root finding. We don't need to find the exact match, just
            // verify that the curve is within the error tolerance of
            // the polyline. So we can start with the vertical distance,
            // but we may need to fall back to an iterative approach.
            for (size_t k = 0; k < 5; ++k) {
                const GfVec2d sampled = GfCompMult(samplePts[k],
                                                   toleranceScales);
                GfVec2d evaluated = GfCompMult(splinePts[k],
                                               toleranceScales);
                if (GfIsClose(evaluated, sampled, tolerance)) {
                    // It's already close enough
                    continue;
                }

                // Vertical distance was outside of tolerance, see if we can
                // find a closer point. We're going to check at 5 points along
                // the spline to see if any of them are close to the sampled
                // point. If so, we're within tolerance and can continue to the
                // next sampled point. If not, we take the closest of the 5
                // points to the sampled point and evaluate 5 points around the
                // closest point.  Iterate until we stop getting closer.
                // 
                // Because of the way we subdivide, the endpoints of each
                // polyline segment are on the spline so our 5 initial points
                // are the ends of the polyline, and the spline points that are
                // at the sampled time and half-way between the sampled time and
                // the two ends.
                //
                // Note that this same algorithm works for both Bezier and
                // Hermite splines because we simply use Eval on the spline.
                GfVec2d testPts[5] =
                {
                    prev,
                    GfLerp(0.5, prev, samplePts[k]),
                    splinePts[k],
                    GfLerp(0.5, next, samplePts[k]),
                    next
                };

                // Loop until we find a spline point close to the polyline
                // or return from the function with an error.
                while (true) {
                    // Correct the values of the intermediate points
                    spline.Eval(testPts[1][0], &testPts[1][1]);
                    spline.Eval(testPts[3][0], &testPts[3][1]);

                    double minErrorSq = std::numeric_limits<double>::infinity();
                    int errIndex = -1;
                    GfVec2d scaledPts[5];
                    GfVec2d scaledCenter(0, 0);
                    for (int n = 0; n < 5; ++n) {
                        scaledPts[n] = GfCompMult(testPts[n], toleranceScales);
                        scaledCenter += scaledPts[n];
                        double errorSq = (sampled - scaledPts[n]).GetLengthSq();
                        if (errorSq < minErrorSq) {
                            minErrorSq = errorSq;
                            errIndex = n;
                        }
                    }

                    if (minErrorSq <= toleranceSq) {
                        // We're already close enough
                        break;
                    }

                    // See if the spline points are already within tolerance of
                    // each other while still not within tolerance of the
                    // sampled point. This is our signal to give up.
                    scaledCenter /= 5;
                    int farIndex = -1;
                    for (int n = 0; n < 5; ++n)
                    {
                        double errorSq =
                            (scaledCenter - scaledPts[n]).GetLengthSq();
                        if (errorSq > toleranceSq) {
                            farIndex = n;
                            break;
                        }
                    }
                        
                    // If farIndex was not found, then we've zoomed in on the
                    // spline far enough that all the points on the spline are
                    // clustered within tolerance of a single center point but
                    // they are still not within tolerance of the sampled point.
                    // give up.
                    if (farIndex < 0) {
                        // We could not find a closer error. Fail
                        std::ostringstream msg;
                        msg.precision(std::numeric_limits<double>::digits10+1);
                        msg << "Error: Sample evaluation exceeds tolerance:\n"
                            << "    time        = " << samplePts[k][0] << "\n"
                            << "    sampleValue = " << samplePts[k][1] << "\n"
                            << "    evalValue   = " << splinePts[k][1] << "\n"
                            << "    valueScale  = " << valueScale << "\n"
                            << "    tolerance   = " << tolerance << "\n"
                            << "    error       = " << std::sqrt(minErrorSq);
                        
                        out << msg.str() << std::endl;
                        std::cerr << msg.str() << std::endl;

                        // Give up on this spline, it failed.
                        return false;
                    }

                    // Otherwise, decide which existing points are going to become
                    // the new first, middle, last of the spline points. We'll then
                    // calculate new second and fourth points.
                    if (errIndex < 2) {
                        // [0], [1], [2] -> [0], [2], [4]
                        testPts[4] = testPts[2];
                        testPts[2] = testPts[1];

                    } else if (errIndex == 2) {
                        // [1], [2], [3] -> [0], [2], [4]
                        testPts[0] = testPts[1];
                        testPts[4] = testPts[3];

                    } else {
                        // [2], [3], [4] -> [0], [2], [4]
                        testPts[0] = testPts[2];
                        testPts[2] = testPts[3];
                    }

                    testPts[1][0] = GfLerp(0.5, testPts[0][0], testPts[2][0]);
                    testPts[3][0] = GfLerp(0.5, testPts[2][0], testPts[4][0]);
                }
            }
        }
    }

    return true;
}

static
bool
VerifySampleError(std::ostream& out,
                  const TsSpline& spline,
                  const GfInterval& timeInterval,
                  const Polylines& polylines,
                  double timeScale,
                  double valueScale,
                  double tolerance)
{
    return
        VerifySamples(out, spline, polylines,
                      timeScale, valueScale, tolerance)
        && VerifySpline(out, spline, timeInterval, polylines,
                        timeScale, valueScale, tolerance);
}

template <typename FLOAT, typename VERTEX>
static
void
DoOneSample(std::ostream& out,
            const TsTest_SplineData& data,
            std::string sampleFunc,
            const GfInterval& timeInterval,
            double timeScale,
            double valueScale,
            double tolerance)
{
    ++testCase;

    const TfType valueType = Ts_GetType<FLOAT>();

    const std::string valueTypeName = valueType.GetTypeName();
    const std::string vertexTypeName = TfType::Find<VERTEX>().GetTypeName();
    
    // Sample the spline and output the results
    out << "Test Case " << testCase << ": "
        << sampleFunc << "<"
        << valueTypeName << ", "
        << vertexTypeName << ">("
        << timeInterval << ", "
        << timeScale << ", "
        << valueScale << ", "
        << tolerance << ")\n";

    // Convert the generic spline data to an actual spline
    const TsTest_TsEvaluator evaluator;
    const TsSpline spline = evaluator.SplineDataToSpline(data, valueType);

    bool result;
    if (sampleFunc == "Sample") {
        TsSplineSamples<GfVec2d> samples;
        result = spline.Sample(timeInterval,
                               timeScale,
                               valueScale,
                               tolerance,
                               &samples);

        if (!result) {
            out << "No result!\n";
        } else {
            if (verbose) {
                for (size_t n = 0; n < samples.polylines.size(); ++n) {
                    out << n << ": (source n/a)\n";
                    for (const auto& vertex : samples.polylines[n]) {
                        out << "    " << vertex << "\n";
                    }
                }
            } else { // terse output
                size_t vertexCount = 0;
                for (const auto& polyline : samples.polylines) {
                    vertexCount += polyline.size();
                }
                out << "    Returned "
                    << vertexCount
                    << " vertices in "
                    << samples.polylines.size()
                    << " polylines.\n";
            }

            TF_AXIOM(VerifySampleError(out,
                                       spline,
                                       timeInterval,
                                       samples.polylines,
                                       timeScale,
                                       valueScale,
                                       tolerance));
            out.flush();
        }
    } else {
        TsSplineSamplesWithSources<GfVec2d> samples;

        result = spline.Sample(timeInterval,
                               timeScale,
                               valueScale,
                               tolerance,
                               &samples);
        if (!result) {
            out << "No result!\n";
        } else {
            if (verbose) {
                for (size_t n = 0; n < samples.polylines.size(); ++n) {
                    out << n
                        << ": ("
                        << TfEnum::GetName(samples.sources[n])
                        << ")\n";

                    for (const auto& vertex : samples.polylines[n]) {
                        out << "    " << vertex << "\n";
                    }
                }
            } else { // terse output
                size_t vertexCount = 0;
                for (const auto& polyline : samples.polylines) {
                    vertexCount += polyline.size();
                }
                out << "    Returned "
                    << vertexCount
                    << " vertices in "
                    << samples.polylines.size()
                    << " polylines.\n";
            }

            TF_AXIOM(VerifySampleError(out,
                                       spline,
                                       timeInterval,
                                       samples.polylines,
                                       timeScale,
                                       valueScale,
                                       tolerance));
        }

    }

    out << std::endl;
}

static
void DoTest(std::ostream& out, const std::string& sampleFunc)
{
    const std::vector<std::string> names = TsTest_Museum::GetAllNames();
    const TsTest_TsEvaluator evaluator;

    // Assume a 500x500 resolution.
    const int xPixels = 500, yPixels = 500;

    out << std::string(72, '#') << "\n"
        << "Testing " << sampleFunc << "\n"
        << std::string(72, '=')
        << std::endl;

    for (const std::string& name : names) {
        const TsTest_SplineData data = TsTest_Museum::GetDataByName(name);

        // Convert the generic spline data to an actual spline
        const TsSpline spline = evaluator.SplineDataToSpline(data);

        // Figure out the time and approximate value range of the spline
        const TsKnotMap knots = spline.GetKnots();
        GfInterval knotSpan = knots.GetTimeSpan();

        // Check for inner looping
        if (spline.HasInnerLoops()) {
            // The looped interval may or may not expand knotSpan
            knotSpan |= spline.GetInnerLoopParams().GetLoopedInterval();
        }
        double knotSpanSize = knotSpan.GetSize();

        // Calculate an extended time range that will include at least one
        // extrapolating loop pre and post (if there are extrapolating loops).
        GfInterval longSpan(knotSpan.GetMin() - 1.5 * knotSpanSize,
                            knotSpan.GetMax() + 1.5 * knotSpanSize);

        // Calculate a small span that is just the middle 50% of knotSpan
        GfInterval shortSpan(knotSpan.GetMin() + 0.25 * knotSpanSize,
                             knotSpan.GetMax() - 0.25 * knotSpanSize);

        // We would like to use spline.GetValueRange() but it is
        // "not yet implemented." Estimate by scanning through
        // the knot times and calling eval.  We're only using it
        // to compute reasonable scale factors.
        double minValue = std::numeric_limits<double>::infinity();
        double maxValue = -std::numeric_limits<double>::infinity();

        for (const TsKnot& knot : knots) {
            double value;
            if (spline.Eval(knot.GetTime(), &value)) {
                minValue = std::min(minValue, value);
                maxValue = std::max(maxValue, value);
            }
        }
        double valueRangeSize = maxValue - minValue;

        // Compute scales but don't divide by 0
        double timeScale = xPixels / std::max(knotSpanSize, 1.0);
        double valueScale = yPixels / std::max(valueRangeSize, 1.0);

        out << "Spline: " << name << "\n"
            << spline << "\n"
            << std::string(72, '-')
            << std::endl;

        // Sample the knots.
        DoOneSample<float, GfVec2f>(out, data, sampleFunc,
                                    knotSpan, timeScale, valueScale, 1.0);

        // Sample the extended range but with less rigor
        DoOneSample<GfHalf, GfVec2h>(out, data, sampleFunc,
                                     longSpan, timeScale, valueScale, 10.0);

        // Sample the short range but more rigor
        DoOneSample<double, GfVec2d>(out, data, sampleFunc,
                                     shortSpan, timeScale, valueScale, 0.5);
    }
}

void TestSample(std::ostream& out)
{
    DoTest(out, "Sample");
}

void TestSampleWithSources(std::ostream& out)
{
    DoTest(out, "SampleWithSources");
}

int main(int argc, const char **argv)
{
    std::ofstream outFile;
    std::ostream* outPtr = nullptr;
    const char* outName = "testTsSplineSampling.txt";
    if (argc > 1) {
        if (strcmp(argv[1], "-") == 0) {
            outPtr = &std::cout;
        } else {
            outName = argv[1];
        }
    }

    if (!outPtr) {
        outFile.open(outName);
        if (outFile.good()) {
            outPtr = &outFile;
        } else {
            std::cerr << "Error: Cannot open output file "
                      << '"' << argv[1] << '"'
                      << std::endl;
            return 1;
        }
    }

    TestSample(*outPtr);
    TestSampleWithSources(*outPtr);

    return 0;
}
