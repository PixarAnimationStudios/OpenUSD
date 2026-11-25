#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Makes graphs of splines for the Ts documentation (doxygen/splines.md).
#
# Unlike museumGraph.py, which graphs fixed cases from the C++ Museum, this
# script builds its splines in Python, so that documentation figures don't
# depend on test infrastructure.  It may be folded into tsTest_Museum.cpp
# someday.
#
# The commands that produce the figures in the documentation are recorded in
# script/genDocImages.py.

from pxr import Gf, Ts, Tf

from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_TsEvaluator as Evaluator
from pxr.Ts import TsTest_Grapher as Grapher

import argparse
import sys

################################################################################
# Test Splines
#
# Each test spline is a function that returns a Ts.Spline.  The docstring is
# shown by --list.

TestSplines = dict()

def TestSpline(splineFunc):
    """
    Function decorator to add a spline generating function to the TestSplines
    global dictionary, indexed by the function name.
    """
    global TestSplines
    TestSplines[splineFunc.__name__] = splineFunc
    return splineFunc

@TestSpline
def IntroExample():
    """Three Bezier knots with held extrapolation."""
    spline = Ts.Spline()
    spline.SetKnot(Ts.Knot(time=0, value=0, postTanWidth=1, postTanSlope=2,
                         nextInterp=Ts.InterpCurve))
    spline.SetKnot(Ts.Knot(time=4, value=5, preTanWidth=1.5, preTanSlope=0,
                         postTanWidth=1.5, postTanSlope=0, nextInterp=Ts.InterpCurve))
    spline.SetKnot(Ts.Knot(time=8, value=1, preTanWidth=1, preTanSlope=-2))
    return spline


@TestSpline
def InterpModes():
    """Held, linear, and curved segments, in that order."""
    spline = Ts.Spline()
    spline.SetKnot(Ts.Knot(time=0, value=0, nextInterp=Ts.InterpHeld))
    spline.SetKnot(Ts.Knot(time=3, value=3, nextInterp=Ts.InterpLinear))
    spline.SetKnot(Ts.Knot(time=6, value=1, postTanWidth=1, postTanSlope=3,
                         nextInterp=Ts.InterpCurve))
    spline.SetKnot(Ts.Knot(time=9, value=4, preTanWidth=1, preTanSlope=-3))
    return spline


@TestSpline
def DualValued():
    """A dual-valued knot: the value jumps at time 4."""
    spline = Ts.Spline()
    spline.SetKnot(Ts.Knot(time=0, value=0, postTanWidth=1, postTanSlope=1,
                         nextInterp=Ts.InterpCurve))
    spline.SetKnot(Ts.Knot(time=4, value=4, preValue=2, preTanWidth=1, preTanSlope=1,
                         postTanWidth=1, postTanSlope=1, nextInterp=Ts.InterpCurve))
    spline.SetKnot(Ts.Knot(time=8, value=6, preTanWidth=1, preTanSlope=1))
    return spline


def _ThreeKnots(preExtrap=None, postExtrap=None):
    """Three curved knots, for demonstrating extrapolation."""
    spline = Ts.Spline()
    for time, value, slope in ((0, 1, 1.0), (4, 3, 0.0), (8, 2, -1.0)):
        spline.SetKnot(Ts.Knot(time=time, value=value, preTanWidth=1,
                               preTanSlope=slope, postTanWidth=1,
                               postTanSlope=slope, nextInterp=Ts.InterpCurve))
    if preExtrap is not None:
        spline.SetPreExtrapolation(preExtrap)
    if postExtrap is not None:
        spline.SetPostExtrapolation(postExtrap)
    return spline


@TestSpline
def ExtrapLinear():
    """Linear extrapolation on both sides."""
    extrap = Ts.Extrapolation(Ts.ExtrapLinear)
    return _ThreeKnots(extrap, extrap)


@TestSpline
def ExtrapSloped():
    """Sloped extrapolation, slope 0.5, on both sides."""
    extrap = Ts.Extrapolation(Ts.ExtrapSloped)
    extrap.slope = 0.5
    return _ThreeKnots(extrap, extrap)


def _Loop(mode, boundaryTime=None):
    extrap = Ts.Extrapolation(mode)
    extrap.loopBoundaryTime = boundaryTime
    return _ThreeKnots(extrap, extrap)


@TestSpline
def LoopRepeat():
    """Repeat looping on both sides."""
    return _Loop(Ts.ExtrapLoopRepeat)


@TestSpline
def LoopReset():
    """Reset looping on both sides."""
    return _Loop(Ts.ExtrapLoopReset)


@TestSpline
def LoopOscillate():
    """Oscillate looping on both sides."""
    return _Loop(Ts.ExtrapLoopOscillate)


@TestSpline
def LoopBoundary():
    """Reset looping with loopBoundaryTime 4: only the 4..8 half repeats."""
    return _Loop(Ts.ExtrapLoopReset, 4.0)


@TestSpline
def AutoEase():
    """Three knots whose tangents are computed by the AutoEase algorithm."""
    spline = Ts.Spline()
    for time, value in ((0, 0), (4, 3), (8, 4)):
        knot = Ts.Knot(time=time, value=value, nextInterp=Ts.InterpCurve)
        knot.SetPreTanAlgorithm(Ts.TangentAlgorithmAutoEase)
        knot.SetPostTanAlgorithm(Ts.TangentAlgorithmAutoEase)
        spline.SetKnot(knot)
    return spline


################################################################################

parser = argparse.ArgumentParser(
    description="Make a graph of a spline for the Ts documentation.")

parser.add_argument("name", nargs='?', default=None,
                    help="Name of spline.  Use --list to see them.")
parser.add_argument("--list", action="store_true",
                    help="List the available splines.")
parser.add_argument("--out",
                    help="Output file.  If omitted, the graph will be shown "
                    "in a window.")
parser.add_argument("--width", type=int, default=800,
                    help="Image pixel width.  Default 800.")
parser.add_argument("--height", type=int, default=500,
                    help="Image pixel height.  Default 500.")
parser.add_argument("--title", help="Graph title.  Default is no title.")
parser.add_argument("--scales", action="store_true",
                    help="Include numeric scales.")

args = parser.parse_args()

if args.list:
    for name, func in TestSplines.items():
        print(f'{name:16} {func.__doc__}')
    sys.exit()

if args.name not in TestSplines:
    parser.error(f'Unknown name "{args.name}".  Use --list to see names.')

spline = TestSplines[args.name]()

times = STimes(spline)
times.AddStandardTimes()
samples = Evaluator().Eval(spline, times)

kwargs = dict(includeBox=False, includeScales=args.scales)
if args.title:
    kwargs["title"] = args.title

grapher = Grapher(widthPx=args.width, heightPx=args.height, **kwargs)
grapher.AddSpline("Ts", spline, samples, colorIndex=1)

if args.out:
    grapher.Write(args.out)
else:
    grapher.Display()
