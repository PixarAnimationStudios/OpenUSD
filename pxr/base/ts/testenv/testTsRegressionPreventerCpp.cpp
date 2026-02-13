//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/raii.h"
#include "pxr/base/ts/regressionPreventer.h"
#include "pxr/base/ts/segment.h"

#include "pxr/base/tf/stringUtils.h"

#include <iostream>
#include <iomanip>

PXR_NAMESPACE_USING_DIRECTIVE

// testTsRegressionPreventer, written in python, tests most of the high level
// functionality of the regression preventer, but internal class Ts_Segment
// is not wrapped to python, so this test is needed to verify the correct
// behavior of segment data in Ts_RegressionPreventerBatchAccess.

// Ts_RegressionPreventerBatchAccess is a bit much to type all the time
using RP = Ts_RegressionPreventerBatchAccess;

// Vectors of enum values
static std::vector<Ts_SegmentInterp> interpModes{
    Ts_SegmentInterp::ValueBlock,
    Ts_SegmentInterp::Held,
    Ts_SegmentInterp::Linear,
    Ts_SegmentInterp::Bezier,
    Ts_SegmentInterp::Hermite,
};

static std::vector<TsAntiRegressionMode> arModes{
    TsAntiRegressionNone,
    TsAntiRegressionContain,
    TsAntiRegressionKeepRatio,
    TsAntiRegressionKeepStart,
};

bool TestQueryNonRegressiveSegments()
{
    bool ok = true;

    // This segment is not regressive for any interpolation or any
    // anti-regression mode.
    Ts_Segment segment{{0.0, 0.0}, {1.0, 1.0},
                       {2.0, -1.0}, {3.0, 0.0},
                       Ts_SegmentInterp::ValueBlock};

    for (auto arMode : arModes) {
        for (auto interp : interpModes) {
            segment.interp = interp;

            if (!TF_VERIFY(!RP::IsSegmentRegressive(&segment, arMode),
                           "Segment %s was incorrectly reported as regressive"
                           " with arMode = %s.",
                           TfStringify(segment).c_str(),
                           TfEnum::GetFullName(arMode).c_str()))
            {
                ok = false;
            }
        }
    }

    std::cout << "TestQueryNonRegressiveSegments     "
              << (ok ? "Passed" : "Failed")
              << std::endl;
    return ok;
}

bool TestQueryRegressiveSegments()
{
    bool ok = true;

    // This segment would absolutely be regressive if it were interpolated as a
    // Bezier curve.
    Ts_Segment segment{{0.0, 0.0}, {4.0, 1.0},
                       {-1.0, -1.0}, {3.0, 0.0},
                       Ts_SegmentInterp::ValueBlock};

    for (auto arMode : arModes) {
        for (auto interp : interpModes) {
            segment.interp = interp;

            const bool isBezier = (interp == Ts_SegmentInterp::Bezier);
            const bool isRegressive = RP::IsSegmentRegressive(&segment, arMode);

            // Only Bezier segments should be regressive
            if (!TF_VERIFY(
                    isRegressive == isBezier,
                    "Segment %s was incorrectly reported as %s"
                    " with arMode = %s.",
                    TfStringify(segment).c_str(),
                    (isBezier ? "regressive" : "non-regressive"),
                    TfEnum::GetFullName(arMode).c_str()))
            {
                ok = false;
            }
        }
    }

    std::cout << "TestQueryRegressiveSegments        "
              << (ok ? "Passed" : "Failed")
              << std::endl;
    return ok;
}

bool TestProcessNonRegressiveSegments()
{
    bool ok = true;

    // This segment is not regressive for any interpolation or any
    // anti-regression mode.
    Ts_Segment segment{{0.0, 0.0}, {1.0, 1.0},
                       {2.0, -1.0}, {3.0, 0.0},
                       Ts_SegmentInterp::ValueBlock};

    for (auto arMode : arModes) {
        for (auto interp : interpModes) {
            segment.interp = interp;

            Ts_Segment copy = segment;
            if (!TF_VERIFY(!RP::ProcessSegment(&copy, arMode),
                           "Segment %s was incorrectly processed as regressive"
                           " with arMode = %s.",
                           TfStringify(segment).c_str(),
                           TfEnum::GetFullName(arMode).c_str()) ||
                !TF_VERIFY(copy == segment,
                           "Non-regressive segment %s was incorrectly processed"
                           " to %s.",
                           TfStringify(segment).c_str(),
                           TfStringify(copy).c_str()))
            {
                ok = false;
            }
        }
    }

    std::cout << "TestProcessNonRegressiveSegments   "
              << (ok ? "Passed" : "Failed")
              << std::endl;
    return ok;
}

bool TestProcessRegressiveSegments()
{
    bool ok = true;

    // This segment would absolutely be regressive if it were interpolated as a
    // Bezier curve.
    Ts_Segment segment{{0.0, 0.0}, {6.0, 1.0},
                       {0.0, -1.0}, {3.0, 0.0},
                       Ts_SegmentInterp::ValueBlock};

    for (auto arMode : arModes) {
        for (auto interp : interpModes) {
            segment.interp = interp;

            Ts_Segment copy = segment;

            const bool isBezier = interp == Ts_SegmentInterp::Bezier;

            Ts_Segment expected = segment;

            if (isBezier) {
                switch (arMode) {
                  case TsAntiRegressionNone:
                    // expected is already correct.
                    break;

                  case TsAntiRegressionContain:
                    expected.t0 = GfVec2d(3.0, 0.5);
                    expected.t1 = GfVec2d(0.0, -1.0);
                    break;

                  case TsAntiRegressionKeepRatio:
                    {
                        const double ratio = 2.0;
                        const double scale = (std::sqrt(ratio) + ratio + 1) /
                                             (ratio * ratio + ratio + 1);
                        const GfVec2d postTan = expected.t0 - expected.p0;
                        const GfVec2d preTan = expected.t1 - expected.p1;

                        // We should be able to simply multiply these two
                        // tangents by scale to get adjusted, non-regressive
                        // values, but there is a constant embedded in the
                        // regression preventer to make sure that all adjusted
                        // segments are just slightly less than vertical. The
                        // numeric stability of the algorithm should probably be
                        // revisited. In the mean time, we'll check for close
                        // rather than exact. The padding value used is 1e-5 but
                        // that gets scaled by the segment width and then other
                        // math is also applied, so we'll compare to within
                        // 3.2e-5.
                        expected.t0 = expected.p0 + postTan * scale;
                        expected.t1 = expected.p1 + preTan * scale;
                    }
                    break;

                  case TsAntiRegressionKeepStart:
                    expected.t0 = GfVec2d(4.0, 2.0/3.0);
                    expected.t1 = GfVec2d(2.0, -1.0/3.0);
                    break;

                  default:
                    const bool unexpected_TsAntiRegressionMode = false;
                    TF_AXIOM(unexpected_TsAntiRegressionMode);
                }
            }

            const bool adjusted = RP::ProcessSegment(&copy, arMode);
            const double tolerance = 3.2e-5;

            if (!TF_VERIFY(
                    adjusted == (isBezier && (arMode != TsAntiRegressionNone)),
                    "%s segment %s was incorrectly adjusted to %s"
                    " with arMode = %s, expected %s",
                    (isBezier ? "Regressive" : "Non-regressive"),
                    TfStringify(segment).c_str(),
                    TfStringify(copy).c_str(),
                    TfEnum::GetFullName(arMode).c_str(),
                    TfStringify(expected).c_str()) ||
                !TF_VERIFY(
                    copy.IsClose(expected, tolerance),
                    "%s segment %s value was changed to %s"
                    " with arMode = %s, expected %s",
                    (isBezier ? "Regressive" : "Non-regressive"),
                    TfStringify(segment).c_str(),
                    TfStringify(copy).c_str(),
                    TfEnum::GetFullName(arMode).c_str(),
                    TfStringify(expected).c_str())
                )
            {
                ok = false;
            }
        }
    }

    std::cout << "TestProcessRegressiveSegments      "
              << (ok ? "Passed" : "Failed")
              << std::endl;
    return ok;
}

int main(int argc, char **argv)
{
    if (TestQueryNonRegressiveSegments() &&
        TestQueryRegressiveSegments() &&
        TestProcessNonRegressiveSegments() &&
        TestProcessRegressiveSegments())
    {
        return 0;
    }

    return 1;
}
