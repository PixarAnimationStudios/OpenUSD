#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Ts

from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_SampleBezier as SampleBezier
from pxr.Ts import TsTest_TsEvaluator as Evaluator
from pxr.Ts import TsTest_Grapher as Grapher

import argparse

################################################################################

parser = argparse.ArgumentParser(
    description = "Make a graph of a Museum spline.")

parser.add_argument("case",
                    help = "Name of Museum case.")

group = parser.add_argument_group("Evaluators (choose one or more)")
group.add_argument("--bez", action = "store_true",
                   help = "Sample with de Casteljau.")
group.add_argument("--ts", action = "store_true",
                   help = "Evaluate with Ts.")
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

group = parser.add_argument_group("Bells and whistles (off by default)")
group.add_argument("--title",
                   help = "Graph title.")
group.add_argument("--box", action = "store_true",
                   help = "Include box around image.")
group.add_argument("--scales", action = "store_true",
                   help = "Include numeric scales.")

args = parser.parse_args()

################################################################################

data = Museum.GetDataByName(args.case)
times = STimes(data)
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

def EvalWithAntiRegression(name, mode, colorIndex):
    spline = Evaluator().SplineDataToSpline(data)
    with Ts.AntiRegressionAuthoringSelector(mode):
        spline.AdjustRegressiveTangents()
    adjustedData = Evaluator().SplineToSplineData(spline)
    samples = Evaluator().Eval(adjustedData, times)
    grapher.AddSpline(name, adjustedData, samples, colorIndex = colorIndex)

if args.bez:
    samples = SampleBezier(data, numSamples = 200)
    grapher.AddSpline("Bezier", data, samples, colorIndex = 0)
if args.ts:
    samples = Evaluator().Eval(data, times)
    grapher.AddSpline("Ts", data, samples, colorIndex = 1)
if args.contain:
    EvalWithAntiRegression(
        "Contain", Ts.AntiRegressionContain, colorIndex = 2)
if args.keepRatio:
    EvalWithAntiRegression(
        "Keep Ratio", Ts.AntiRegressionKeepRatio, colorIndex = 3)
if args.keepStart:
    EvalWithAntiRegression(
        "Keep Start", Ts.AntiRegressionKeepStart, colorIndex = 4)

if args.out:
    grapher.Write(args.out)
else:
    grapher.Display()
