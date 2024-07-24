#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Ts, Gf

EPSILON = 1e-6

def CreateBeforeSpline():
    beforeSpline = Ts.Spline()

    # Set up the before spline
    keyFramesBefore = [
        Ts.KeyFrame(0.0, 0.0, Ts.KnotBezier, 
                      leftSlope=0, 
                      rightSlope=0, 
                      leftLen=0.3333333333333333, 
                      rightLen=0.3333333333333333),
        Ts.KeyFrame(1, 2.491507846458525, Ts.KnotBezier, 
                      leftSlope=2.992427434859666, 
                      rightSlope=2.992427434859666, 
                      leftLen=0.3333333333333333, 
                      rightLen=0.3333333333333333),
        Ts.KeyFrame(2, 5.38290365439421, Ts.KnotBezier, 
                      leftSlope=3.152460322228518, 
                      rightSlope=3.152460322228518, 
                      leftLen=0.3333333333333333, 
                      rightLen=0.3333333333333333),
        Ts.KeyFrame(3, 9.043661089113483, Ts.KnotBezier, 
                      leftSlope=4.410088992919291, 
                      rightSlope=4.410088992919291, 
                      leftLen=0.3333333333333333, 
                      rightLen=0.3333333333333333),
        Ts.KeyFrame(4, 14.723785002052717, Ts.KnotBezier, 
                      leftSlope=6.333864006799226, 
                      rightSlope=6.333864006799226, 
                      leftLen=0.3333333333333333, 
                      rightLen=0.3333333333333333),
        Ts.KeyFrame(5, 20.71267999005244, Ts.KnotBezier, 
                      leftSlope=4.866350481683961, 
                      rightSlope=4.866350481683961, 
                      leftLen=0.3333333333333333, 
                      rightLen=0.3333333333333333),
        Ts.KeyFrame(6, 24.01397216143913, Ts.KnotBezier, 
                      leftSlope=0.4713087936971686, 
                      rightSlope=0.4713087936971686, 
                      leftLen=0.3333333333333333, 
                      rightLen=0.3333333333333333),
        Ts.KeyFrame(7, 24.231251085355073, Ts.KnotBezier, 
                      leftSlope=0, 
                      rightSlope=0, 
                      leftLen=0.3333333333333333, 
                      rightLen=0.3333333333333333)]

    for kf in keyFramesBefore:
        beforeSpline.SetKeyFrame(kf)

    return beforeSpline

def CompareWithResults(beforeSpline):
    # Set up the after keys
    keyFramesAfter = [
        Ts.KeyFrame(0.0, 0.0, Ts.KnotBezier,
                    leftSlope=2.491507846458525,
                    rightSlope=2.491507846458525,
                    leftLen=0.3333333333333333,
                    rightLen=0.10008544555664062),
        Ts.KeyFrame(3.0, 9.043661089113483, Ts.KnotBezier,
                    leftSlope=4.670440673829253,
                    rightSlope=4.670440673829253,
                    leftLen=0.8931911022949219,
                    rightLen=0.8875199291992186),
        Ts.KeyFrame(5.0, 20.71267999005244, Ts.KnotBezier,
                    leftSlope=4.645093579693207,
                    rightSlope=4.645093579693207,
                    leftLen=0.6600031860351563,
                    rightLen=0.9999050708007813),
        Ts.KeyFrame(7.0, 24.231251085355073, Ts.KnotBezier,
                    leftSlope=0.2172789239159414,
                    rightSlope=0.2172789239159414,
                    leftLen=0.6857100512695311,
                    rightLen=0.3333333333333333)]

    # Check that the passed-in simplified spline matches the expected key frames
    for i, t in enumerate(beforeSpline.keys()):
        assert beforeSpline[t].time == keyFramesAfter[i].time
        assert beforeSpline[t].knotType == keyFramesAfter[i].knotType
        assert Gf.IsClose(
            beforeSpline[t].value, keyFramesAfter[i].value, EPSILON)
        assert Gf.IsClose(
            beforeSpline[t].leftLen, keyFramesAfter[i].leftLen, EPSILON)
        assert Gf.IsClose(
            beforeSpline[t].rightLen, keyFramesAfter[i].rightLen, EPSILON)

# Test Simplify API.  This is not meant to be a deep test of Simplify.
def TestSimplify():

    beforeSpline0 = CreateBeforeSpline()

    # Test non parallel api
    Ts.SimplifySpline(beforeSpline0, 
                        Gf.MultiInterval(Gf.Interval(0, 7)), 0.003)

    CompareWithResults(beforeSpline0)

    # Test parallel api
    beforeSpline0 = CreateBeforeSpline()
    beforeSpline1 = CreateBeforeSpline()

    Ts.SimplifySplinesInParallel([beforeSpline0, beforeSpline1], 
                                   [Gf.MultiInterval(Gf.Interval(0, 7)),
                                    Gf.MultiInterval(Gf.Interval(0, 7))],
                                   0.003)

    CompareWithResults(beforeSpline0)
    CompareWithResults(beforeSpline1)

    # Test parallel api, with an empty interval list (should process the
    # wholes splines)
    beforeSpline0 = CreateBeforeSpline()
    beforeSpline1 = CreateBeforeSpline()

    Ts.SimplifySplinesInParallel([beforeSpline0, beforeSpline1], 
                                   [],
                                   0.003)

    CompareWithResults(beforeSpline0)
    CompareWithResults(beforeSpline1)

TestSimplify()

print('\nTest SUCCEEDED')
