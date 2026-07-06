#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Gf, Ts, Tf

from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_SampleBezier as SampleBezier
from pxr.Ts import TsTest_TsEvaluator as Evaluator
from pxr.Ts import TsTest_Grapher as Grapher

import argparse
import sys

################################################################################

parser = argparse.ArgumentParser(
    description = "Make a graph of a Museum spline.")

parser.add_argument("case", nargs='?', default=None,
                    help = "Name of Museum case.")

group = parser.add_argument_group("Evaluators (choose one or more)")
group.add_argument("--bez", action = "store_true",
                   help = "Sample with de Casteljau.")
group.add_argument("--ts", action = "store_true",
                   help = "Evaluate with Ts.")
group.add_argument("--sample", action = "store_true",
                   help = "Evaluate with Ts.Spline.Sample")
group.add_argument("--sampleWithSources", action = "store_true",
                   help = "Evaluate with Ts.Spline.SampleWithSources")
group.add_argument("--contain", action = "store_true",
                   help = "De-regress with Contain, then evaluate with Ts.")
group.add_argument("--keepRatio", action = "store_true",
                   help = "De-regress with Keep Ratio, then evaluate with Ts.")
group.add_argument("--keepStart", action = "store_true",
                   help = "De-regress with Keep Start, then evaluate with Ts.")

group = parser.add_argument_group("Output")
group.add_argument("--out",
                   help = "Output file.  If omitted, the graph "
                   "will be shown in a window.")

group = parser.add_argument_group("Size")
group.add_argument("--width", type = int, default = 1000,
                   help = "Image pixel width.  Default 1000.")
group.add_argument("--height", type = int, default = 750,
                   help = "Image pixel height.  Default 750.")

group = parser.add_argument_group("Sampling")
group.add_argument("--minTime", type=float, default=None,
                   help = "Minimum time to sample.")
group.add_argument("--maxTime", type=float, default=None,
                   help = "Maximum time to sample.")
group.add_argument("--timeScale", type=float, default=None,
                   help = ("Set the timeScale for sampling. If unset, the scale"
                           " will be calculated from the knot times in the"
                           " spline and the width of the output"))
group.add_argument("--valueScale", type=float, default=None,
                   help = ("Set the valueScale for sampling. If unset, the scale"
                           " will be calculated from the knot values in the"
                           " spline and the width of the output"))
group.add_argument("--tolerance", type=float, default=1.0,
                   help = "Set the max error tolerance for sampling.")

group = parser.add_argument_group("Info")
group.add_argument("--list", action="store_true", default=False,
                   help="List the available splines in the museum.")

group = parser.add_argument_group("Bells and whistles (off by default)")
group.add_argument("--title",
                   help = "Graph title.")
group.add_argument("--box", action = "store_true",
                   help = "Include box around image.")
group.add_argument("--scales", action = "store_true",
                   help = "Include numeric scales.")

args = parser.parse_args()

if not (args.bez or args.ts or args.sample or args.sampleWithSources or
        args.contain or args.keepRatio or args.keepStart):
    args.ts = True

if not args.case:
    args.case = '0'

if args.case.isdigit():
    index = int(args.case)
    allNames = Museum.GetAllNames()
    if 0 <= index < len(allNames):
        args.case = allNames[index]
    else:
        parser.error("Case index {case} is out of range. Use --list to see"
                     " all available cases.")
        # parser.error does not return

if not args.title:
    args.title = args.case
print(f'{args.title = }')
    
################################################################################

if args.list:
    allNames = Museum.GetAllNames()
    print('Available spline cases in the museum:')
    print('=====================================')
    for i, name in enumerate(allNames):
        print(f'{i:3}: {name}')

    sys.exit()

        
spline = Museum.GetSplineByName(args.case, Tf.Type.FindByName('double'))

times = STimes(spline)
times.AddStandardTimes()

kwargs = dict()
if args.title:
    kwargs["title"] = args.title
if not args.scales:
    kwargs["includeScales"] = False
if not args.box:
    kwargs["includeBox"] = False

grapher = Grapher(
    widthPx = args.width, heightPx = args.height,
    **kwargs)

if args.minTime is None:
    args.minTime = times.GetMinTime()

if args.maxTime is None:
    args.maxTime = times.GetMaxTime()
    
if args.timeScale is None:
    timeSpan = max(args.maxTime - args.minTime, .001)
    args.timeScale = args.width / timeSpan

if args.valueScale is None:
    knotMap = spline.GetKnots()
    minValue = min(knot.GetValue() for knot in knotMap.values())
    maxValue = max(knot.GetValue() for knot in knotMap.values())
    valueSpan = max(maxValue - minValue, .001)
    args.valueScale = args.height / valueSpan

def GetSamples(spline, sampler="eval", antiRegressor=None):
    global times, args

    splineCopy = Ts.Spline(spline)

    if antiRegressor is not None:
        with Ts.AntiRegressionAuthoringSelector(antiRegressor):
            splineCopy.AdjustRegressiveTangents()

    if sampler == "bez":
        return SampleBezier(splineCopy, numSamples=200)
    if sampler == "eval":
        return Evaluator().Eval(splineCopy, times)
    if sampler == "sample":
        return Evaluator().Sample(splineCopy,
                                  Gf.Interval(args.minTime,
                                              args.maxTime),
                                  args.timeScale,
                                  args.valueScale,
                                  args.tolerance,
                                  withSources=False)
    if sampler == "sampleWithSources":
        return Evaluator().Sample(splineCopy,
                                  Gf.Interval(args.minTime,
                                              args.maxTime),
                                  args.timeScale,
                                  args.valueScale,
                                  args.tolerance,
                                  withSources=True)
    assert (sampler in ("bez", "eval", "sample", "sampleWithSources"))
            
    
def EvalWithAntiRegression(name, mode, spline, colorIndex):
    splineCopy = Ts.Spline(spline)
    with Ts.AntiRegressionAuthoringSelector(mode):
        splineCopy.AdjustRegressiveTangents()
    samples = Evaluator().Eval(splineCopy, times)
    grapher.AddSpline(name, splineCopy, samples, colorIndex = colorIndex)


if args.bez:
    samples = GetSamples(spline, "bez");
    grapher.AddSpline("Bezier", spline, samples, colorIndex = 0)

if args.ts:
    samples = GetSamples(spline, "eval");
    grapher.AddSpline("Ts", spline, samples, colorIndex = 1)

if args.contain:
    samples = GetSamples(spline, "eval", Ts.AntiRegressionContain)
    grapher.AddSpline("Contain", spline, samples, colorIndex = 2)

if args.keepRatio:
    samples = GetSamples(spline, "eval", Ts.AntiRegressionKeepRatio)
    grapher.AddSpline("Contain", spline, samples, colorIndex = 3)

if args.keepStart:
    samples = GetSamples(spline, "eval", Ts.AntiRegressionKeepStart)
    grapher.AddSpline("Contain", spline, samples, colorIndex = 4)
    
if args.sample:
    # Note that using "samples" returns a Ts.SplineSamples instead of a list of
    # TsTest_Sample objects.
    samples = GetSamples(spline, "sample")
    grapher.AddSpline("Sample", spline, samples, colorIndex = 5)
    
if args.sampleWithSources:
    # Note that using "samplesWithSources returns a Ts.SplineSamplesWithSources
    # instead of a list of TsTest_Sample objects.
    samples = GetSamples(spline, "sampleWithSources")
    grapher.AddSpline("Sources", spline, samples, colorIndex = 6)

if args.out:
    grapher.Write(args.out)
else:
    grapher.Display()
