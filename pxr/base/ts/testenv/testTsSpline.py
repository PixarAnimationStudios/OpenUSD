#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

from pxr import Ts, Tf, Gf
import contextlib

EPSILON = 1e-6

@contextlib.contextmanager
def _RequiredException(exceptionType):
    try:
        # Execute code block.
        yield
    except exceptionType:
        # Got expected exception, continue.
        pass
    except:
        # Got unexpected exception, re-raise.
        raise
    else:
        # Got no exception, raise exception.
        raise Exception("required exception not raised")

def quatIsClose(a, b):
    assert(Gf.IsClose(a.real, b.real, EPSILON))
    assert(Gf.IsClose(a.imaginary, b.imaginary, EPSILON))
    return True

def createSpline(keys, values, types, tangentSlope=[], tangentLength=[]):
    lenKeys = len(keys)
    assert(lenKeys == len(values))
    assert(lenKeys == len(types))
    assert(lenKeys == len(types))

    hasTangents = len(tangentSlope) > 0 and len(tangentLength) > 0
    if hasTangents:
        assert(2 * lenKeys == len(tangentSlope))
        assert(2 * lenKeys == len(tangentLength))

    s = Ts.Spline()
    # add keyframes
    for index in range(len(keys)):
        kf = Ts.KeyFrame(keys[index], values[index], types[index])
        if hasTangents and kf.supportsTangents:
            kf.leftSlope  = tangentSlope[2*index]
            kf.rightSlope = tangentSlope[2*index+1]

            kf.leftLen    = tangentLength[2*index]
            kf.rightLen   = tangentLength[2*index+1]
        s.SetKeyFrame(kf)

    # check Eval() to make sure keyframes were added correctly
    for index in range(len(keys)):
        assert(s.Eval(keys[index]) == values[index])

    return s

default_knotType = Ts.KnotBezier

def verifyNegativeCanResult(res):
    assert bool(res) == False
    assert type(res.reasonWhyNot) == str
    assert len(res.reasonWhyNot) > 0

########################################################################
# Test API unique to Ts.Spline:

def TestTsSpline():
    print('\nTest setting an TsSpline as a dict')
    v = Ts.Spline()
    v1 = Ts.Spline( {0:0.0, 100:10.0}, Ts.KnotLinear )
    v2 = Ts.Spline( \
        [ Ts.KeyFrame(0,   0.0, Ts.KnotLinear),
          Ts.KeyFrame(100, 10.0, Ts.KnotLinear) ] )
    assert v1 == v2
    assert v1 != v
    assert v2 != v
    del v1
    del v2
    with _RequiredException(TypeError):
        # Test incorrect time type
        Ts.Spline( {'blah':0}, Ts.KnotLinear )
    with _RequiredException(Tf.ErrorException):
        # Test value type mismatch
        Ts.Spline( {0:0.0, 1:'blah'}, Ts.KnotLinear )
    print('\tPassed')

    print('\nTest copy constructor, and constructor w/ extrapolation')
    v = Ts.Spline( \
        [ Ts.KeyFrame(0,   0.0, Ts.KnotLinear),
          Ts.KeyFrame(100, 10.0, Ts.KnotLinear) ],
          Ts.ExtrapolationHeld,
          Ts.ExtrapolationLinear )
    assert v.extrapolation[0] == Ts.ExtrapolationHeld
    assert v.extrapolation[1] == Ts.ExtrapolationLinear
    assert v == Ts.Spline(v)
    assert v == eval(repr(v))

    print('\nTest float Breakdown() on empty spline')
    del v[:]
    v.Breakdown(1, Ts.KnotHeld, True, 1.0)
    assert(1 in v)
    assert(v.Eval(1) == 0.0)
    assert v == eval(repr(v))
    print('\tPassed')

TestTsSpline()

########################################################################

def TestBezierDerivative():
    range1 = list(range(2,6))
    range2 = list(range(6,10))
    rangeTotal = list(range(2, 10))

    print("Start bezier derivative test:")
    spline = createSpline([1, 10],
                          [0.0, 0.0],
                          [Ts.KnotBezier, Ts.KnotBezier],
                          [1.0, 1.0, -1.0, -1.0], [1.0, 1.0, 1.0, 1.0])

    print("\tTest EvalDerivative at keyframes")
    spline.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
    assert(spline.EvalDerivative(1, Ts.Right) == 1.0)
    assert(spline.EvalDerivative(1, Ts.Left) == 1.0)
    assert(spline.EvalDerivative(10, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(10, Ts.Left) == -1.0)
    spline.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
    assert(spline.EvalDerivative(1, Ts.Right) == 1.0)
    assert(spline.EvalDerivative(1, Ts.Left) == 1.0)
    assert(spline.EvalDerivative(10, Ts.Right) == -1.0)
    assert(spline.EvalDerivative(10, Ts.Left) == -1.0)

    print("\tTest EvalDerivative between keyframes")
    # generate the calculated derivatives
    calcDerivs = []
    for time in range1:
        calcDerivs.append(spline.EvalDerivative(time, Ts.Right))
        calcDerivs.append(spline.EvalDerivative(time, Ts.Left))
    for time in range2:
        calcDerivs.append(0.0)
        calcDerivs.append(0.0)

    # perform explicit breakdowns at the desired points
    for time in range1:
        spline.Breakdown(time, Ts.KnotBezier, False, 1.0)

    for time in range2:
        spline.Breakdown(time, Ts.KnotBezier, True, 0.9)

    # generate the derivates calculated from breakdowns
    breakdownDerivs = []
    for time in rangeTotal:
        breakdownDerivs.append(spline.EvalDerivative(time, Ts.Right))
        breakdownDerivs.append(spline.EvalDerivative(time, Ts.Left))

    # compare values
    assert(len(calcDerivs) == len(breakdownDerivs))
    for index in range(len(calcDerivs)):
        assert(Gf.IsClose(calcDerivs[index], breakdownDerivs[index], EPSILON))

    print("\tPassed")

TestBezierDerivative()

# ################################################################################

def TestLinearDerivative():
    print("Start linear derivative test:")

    range1 = list(range(2,5))
    range2 = list(range(6,10))
    rangeTotal = (2, 3, 4, 6, 7, 8, 9)

    spline = createSpline([1, 5, 10],
                          [0.0, 1.0, 0.0],
                          [Ts.KnotLinear, Ts.KnotLinear, Ts.KnotLinear])

    print("\tTest EvalDerivative at keyframes")
    spline.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
    assert(spline.EvalDerivative(1, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(1, Ts.Left) == 0.0)
    assert(spline.EvalDerivative(5, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(5, Ts.Left) == 0.0)
    assert(spline.EvalDerivative(10, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(10, Ts.Left) == 0.0)
    spline.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
    assert(spline.EvalDerivative(1, Ts.Right) == 0.25)
    assert(spline.EvalDerivative(1, Ts.Left) == 0.25)
    assert(spline.EvalDerivative(5, Ts.Right) == 0.25)
    assert(spline.EvalDerivative(5, Ts.Left) == -0.2)
    assert(spline.EvalDerivative(10, Ts.Right) == -0.2)
    assert(spline.EvalDerivative(10, Ts.Left) == -0.2)

    # generate the calculated derivatives
    calcDerivs = []
    for time in range1:
        calcDerivs.append(spline.EvalDerivative(time, Ts.Right))
        calcDerivs.append(spline.EvalDerivative(time, Ts.Left))
    for time in range2:
        calcDerivs.append(-0.2)
        calcDerivs.append(-0.2)

    # perform explicit breakdowns at the desired points
    for time in range1:
        spline.Breakdown(time, Ts.KnotLinear, False, 1.0)
    for time in range2:
        spline.Breakdown(time, Ts.KnotLinear, True, 1.0)

    # generate the derivates calculated from breakdowns
    breakdownDerivs = []
    for time in rangeTotal:
        breakdownDerivs.append(spline.EvalDerivative(time, Ts.Right))
        breakdownDerivs.append(spline.EvalDerivative(time, Ts.Left))

    # compare values
    assert(len(calcDerivs) == len(breakdownDerivs))
    for index in range(len(calcDerivs)):
        assert(Gf.IsClose(calcDerivs[index], breakdownDerivs[index], EPSILON))

    print("\tPASSED")

TestLinearDerivative()

# ################################################################################

def TestHeldDerivative():
    print("Start held derivative test:")

    range1 = list(range(2,5))
    range2 = list(range(6,10))
    rangeTotal = (2, 3, 4, 6, 7, 8, 9)

    spline = createSpline([1, 5, 10],
                          [0.0, 1.0, 2.0],
                          [Ts.KnotHeld, Ts.KnotHeld, Ts.KnotHeld])

    print("\tTest EvalDerivative at keyframes")
    spline.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
    assert(spline.EvalDerivative(1, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(1, Ts.Left) == 0.0)
    assert(spline.EvalDerivative(5, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(5, Ts.Left) == 0.0)
    assert(spline.EvalDerivative(10, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(10, Ts.Left) == 0.0)
    spline.extrapolation = (Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
    assert(spline.EvalDerivative(1, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(1, Ts.Left) == 0.0)
    assert(spline.EvalDerivative(5, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(5, Ts.Left) == 0.0)
    assert(spline.EvalDerivative(10, Ts.Right) == 0.0)
    assert(spline.EvalDerivative(10, Ts.Left) == 0.0)

    print("\tTest EvalDerivative between keyframes")
    # generate the calculated derivatives
    calcDerivs = []
    for time in range1:
        calcDerivs.append(spline.EvalDerivative(time, Ts.Left))
        calcDerivs.append(spline.EvalDerivative(time, Ts.Right))
    for time in range2:
        calcDerivs.append(0.0)
        calcDerivs.append(0.0)

    # perform explicit breakdowns at the desired points
    for time in range1:
        spline.Breakdown(time, Ts.KnotHeld, False, 1.0)
    for time in range2:
        spline.Breakdown(time, Ts.KnotHeld, True, 1.0)

    # generate the derivates calculated from breakdowns
    breakdownDerivs = []
    for time in rangeTotal:
        breakdownDerivs.append(spline.EvalDerivative(time, Ts.Left))
        breakdownDerivs.append(spline.EvalDerivative(time, Ts.Right))

    # compare values
    assert(len(calcDerivs) == len(breakdownDerivs))
    for index in range(len(calcDerivs)):
        assert(Gf.IsClose(calcDerivs[index], breakdownDerivs[index], EPSILON))

    print("\tPASSED")

TestHeldDerivative()

################################################################################

def TestBlendDerivative():
    range1 = list(range(2,5))
    range2 = list(range(6,10))
    rangeTotal = (2, 3, 4, 6, 7, 8, 9)

    print("Start blend derivative test:")
    spline = createSpline([1, 10], [0.0, 0.0], [Ts.KnotBezier, Ts.KnotHeld], [1.0, 1.0, -1.0, -1.0], [1.0, 1.0, 1.0, 1.0])

    print("\tTest EvalDerivative at a keyframe")
    assert(spline.EvalDerivative(1) == 1.0)
    assert(spline.EvalDerivative(10) == 0.0)

    print("\tTest EvalDerivative between keyframes")
    # generate the calculated derivatives
    calcDerivs = []
    for time in range1:
        calcDerivs.append(spline.EvalDerivative(time, Ts.Left))
        calcDerivs.append(spline.EvalDerivative(time, Ts.Right))
    for time in range2:
        calcDerivs.append(0.0)
        calcDerivs.append(0.0)

    # perform explicit breakdowns at the desired points
    for time in range1:
        spline.Breakdown(time, Ts.KnotBezier, False, 1.0)
    for time in range2:
        spline.Breakdown(time, Ts.KnotBezier, True, 1.0)

    # generate the derivates calculated from breakdowns
    breakdownDerivs = []
    for time in rangeTotal:
        breakdownDerivs.append(spline.EvalDerivative(time, Ts.Left))
        breakdownDerivs.append(spline.EvalDerivative(time, Ts.Right))

    # XXX There might be a bug in Breakdown, so this test is disabled for now
    # # compare values
    # assert(len(calcDerivs) == len(breakdownDerivs))
    # for index in range(len(calcDerivs)):
    #     assert(Gf.IsClose(calcDerivs[index], breakdownDerivs[index]))

    print("\tTest EvalDerivative before first keyframe")
    assert(spline.EvalDerivative(0) == 0.0)

    print("\tTest EvalDerivative after last keyframe")
    assert(spline.EvalDerivative(10) == 0.0)

    print("\tPASSED")

TestBlendDerivative()

########################################################################
def TestQuaternion():
    print("Start quaternion test:")

    quat1 = Gf.Quatd(0,1,2,3)
    quat2 = Gf.Quatd(4,5,6,7)
    quat3 = Gf.Quatd(-1,-2,-3,-4)

    quatZero = Gf.Quatd(0, 0, 0, 0)

    quats = [quat1, quat2, quat3]

    evalFrames = [0, 0.1, 0.5, 1.0, 2.0, 2.3875, 3.0]
    correctValuesHeld = [
        quat1,
        quat1,
        quat1,
        quat2,
        quat2,
        quat2,
        quat3]
    correctValuesLinear = [
        quat1,
        Gf.Slerp(0.1, quat1, quat2),
        Gf.Slerp(0.5, quat1, quat2),
        quat2,
        Gf.Slerp(0.5, quat2, quat3),
        Gf.Slerp((2.3875 - 1.0) / (3.0 - 1.0), quat2, quat3),
        quat3]

    quatSplineHeld = createSpline([0, 1, 3],
                          [quat1, quat2, quat3],
                          [Ts.KnotHeld, Ts.KnotHeld, Ts.KnotHeld])

    heldSplineValues = list(quatSplineHeld.values())
    assert(len(heldSplineValues) == len(quats))

    # we expect to receive an exception when accesing slope values on
    # non-tangential keyframe types
    with _RequiredException(Tf.ErrorException):
        heldSplineValues[0].leftSlope
    with _RequiredException(Tf.ErrorException):
        heldSplineValues[0].rightSlope

    for ind, key in enumerate(heldSplineValues):
        assert(key.value == quats[ind])

    for ind in range(len(evalFrames)):
        frame = evalFrames[ind]
        assert(quatIsClose(quatSplineHeld.Eval(frame), correctValuesHeld[ind]))
        assert(quatIsClose(quatSplineHeld.EvalDerivative(frame), quatZero))

    quatSplineLinear = createSpline([0, 1, 3],
                          [quat1, quat2, quat3],
                          [Ts.KnotLinear, Ts.KnotLinear, Ts.KnotLinear])
    linearSplineValues = list(quatSplineLinear.values())
    assert(len(linearSplineValues) == len(quats))
    for ind, key in enumerate(heldSplineValues):
        assert(key.value == quats[ind])

    for ind, frame in enumerate(evalFrames):
        assert(quatIsClose(quatSplineLinear.Eval(frame), correctValuesLinear[ind]))
        assert(quatIsClose(quatSplineLinear.EvalDerivative(frame), quatZero))

    # The current behavior of KeyFrame initialization is that if instantiated with
    # knots unsupported by the key frame's type, then it will auto-correct it to
    # a supported type.
    #
    # see Ts/KeyFrame.cpp:94
    quat4 = Gf.Quatd(0,1,2,3)
    bezierKf = Ts.KeyFrame(0.0, quat4, Ts.KnotBezier)
    assert(bezierKf.knotType == Ts.KnotLinear)

    # test linear extrapolation
    kfs = [Ts.KeyFrame(1, Gf.Quatd(1,2,3,4), Ts.KnotLinear), Ts.KeyFrame(2, Gf.Quatd(2,3,4,5), Ts.KnotLinear)]
    linearExtrapolationSpline = Ts.Spline(kfs, Ts.ExtrapolationLinear, Ts.ExtrapolationLinear)
    quatExpectedResult = Gf.Quatd(1,2,3,4)
    assert(quatIsClose(linearExtrapolationSpline.Eval(0), quatExpectedResult))
    quatExpectedResult = Gf.Quatd(2,3,4,5)
    assert(quatIsClose(linearExtrapolationSpline.Eval(20), quatExpectedResult))

    # test held extrapolation
    kfs = [Ts.KeyFrame(1, Gf.Quatd(1,2,3,4), Ts.KnotHeld), Ts.KeyFrame(2, Gf.Quatd(2,3,4,5), Ts.KnotHeld)]
    heldExtrapolationSpline = Ts.Spline(kfs, Ts.ExtrapolationHeld, Ts.ExtrapolationHeld)
    quatExpectedResult = Gf.Quatd(1,2,3,4)
    assert(quatIsClose(heldExtrapolationSpline.Eval(0), quatExpectedResult))
    quatExpectedResult = Gf.Quatd(2,3,4,5)
    assert(quatIsClose(linearExtrapolationSpline.Eval(20), quatExpectedResult))

    print("\tPASSED")

TestQuaternion()
    
########################################################################

def TestBreakdown():
    print("Start breakdown test:")

    def VerifyBreakdown(t, flatTangents, value=None):
        spline = createSpline([0, 10],
                              [0.0, 1.0],
                              [Ts.KnotBezier, Ts.KnotBezier],
                              [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0, 1.0])

        # Save some values for later verification.
        ldy = spline.EvalDerivative(t, Ts.Left)
        rdy = spline.EvalDerivative(t, Ts.Right)

        # Build arg list for Breakdown() call.
        args = [t, Ts.KnotBezier, flatTangents, 3.0]
        if (value):
            args.append(value)
        else:
            value = spline.Eval(t)

        # Breakdown at a new frame.
        assert t not in spline
        newKf = spline.Breakdown(*args)
        assert t in spline

        # Verify resulting keyframe value.
        assert spline[t] == newKf
        assert spline[t].value == value

        # Verify tangents.
        if (flatTangents):
            assert spline[t].leftLen == 3.0
            assert spline[t].rightLen == 3.0
            assert spline.EvalDerivative(t, Ts.Left) == 0.0
            assert spline.EvalDerivative(t, Ts.Right) == 0.0
        else:
            assert Gf.IsClose(spline[t].leftSlope, ldy, EPSILON)
            assert Gf.IsClose(spline[t].rightSlope, rdy, EPSILON)

    print("\tTest breakdown with flat tangents")
    VerifyBreakdown(7.5, flatTangents=True)

    print("\tTest breakdown with flat tangents, specifying value")
    VerifyBreakdown(7.5, flatTangents=True, value=12.34)

    print("\tTest breakdown with auto tangents")
    VerifyBreakdown(2.5, flatTangents=False)

    print("\tTest breakdown with auto tangents, specifying value")
    VerifyBreakdown(2.5, flatTangents=False, value=12.34)

TestBreakdown()

def TestBreakdownWithExtrapolation():
    print("Start breakdown with linear extrapolation test:")

    for leftExtrapolation in [
            Ts.ExtrapolationHeld,
            Ts.ExtrapolationLinear]:
        for rightExtrapolation in [
            Ts.ExtrapolationHeld,
            Ts.ExtrapolationLinear]:
            
            spline = Ts.Spline(
                [Ts.KeyFrame(100.0, 20.0, Ts.KnotBezier,
                               leftSlope=-3.0, rightSlope=-5.0,
                               leftLen=0.9, rightLen=0.9),
                 Ts.KeyFrame(200.0, 30.0, Ts.KnotBezier,
                               leftSlope=4.0, rightSlope=6.0,
                               leftLen=0.9, rightLen=0.9)],
                leftExtrapolation,
                rightExtrapolation,
                Ts.LoopParams())
            
            for frame in [0.0, 300.0]:
                breakdownSpline = Ts.Spline(spline)
                breakdownSpline.Breakdown(
                    frame, Ts.KnotBezier, False, 0.9)

                # Test that the broken down spline evaluates to the same
                # number as the original spline.

                # Caveat: When we are in held extrapolation and do a breakdown
                # to left of all knots and the left most knot had a non-flat
                # tangent, the spline will actually change. It used to be
                # constant on the interval between the time the breakdown and
                # the left most knot due to held extrapolation, but now is
                # influenced by the non-flat tangent of that knot.
                # This behaviour seems to be desired.

                # Testing for points outside the interval where the splines
                # might differ.
                for t in [-100.0, 0.0, 100.0, 150.0,
                          200.0, 300.0, 400.0]:

                    assert(spline.Eval(t) == breakdownSpline.Eval(t))

                # Testing points inside that interval when we don't have held
                # extrapolation.
                if leftExtrapolation == Ts.ExtrapolationLinear:
                    t = 50.0
                    assert(spline.Eval(t) == breakdownSpline.Eval(t))

                if rightExtrapolation == Ts.ExtrapolationLinear:
                    t = 250.0
                    assert(spline.Eval(t) == breakdownSpline.Eval(t))

TestBreakdownWithExtrapolation()

def TestBreakdownMultiple():
    print("Start breakdown multiple test:")

    spline = createSpline([0, 10],
                          [0.0, 1.0],
                          [Ts.KnotBezier, Ts.KnotBezier],
                          [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0, 1.0])
    times = [3.0, 6.0, 12.0, 19.0]
    values = [0.5, 0.8, 2.0, -9.0]
    tangentLength = 1.0
    flatTangents = False
    spline.Breakdown(times, Ts.KnotBezier, flatTangents, tangentLength, values)

    for i, time in enumerate(times):
        assert time in spline
        assert spline[time].value == values[i]

TestBreakdownMultiple()

def TestBreakdownMultipleKnotTypes():
    print("Start breakdown multiple knot types test:")

    spline = createSpline([0, 10],
                          [0.0, 1.0],
                          [Ts.KnotBezier, Ts.KnotBezier],
                          [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0, 1.0])
    times = [3.0, 6.0, 19.0]
    values = [0.5, 0.8, -9.0]
    types = [Ts.KnotBezier, Ts.KnotLinear, Ts.KnotHeld]
    tangentLength = 1.0
    flatTangents = False
    spline.Breakdown(times, types, flatTangents, tangentLength, values)

    for i, time in enumerate(times):
        assert time in spline
        assert spline[time].value == values[i]
        assert spline[time].knotType == types[i]

TestBreakdownMultipleKnotTypes()


def TestBug12502():
    # Verify breakdown behavior on empty splines.
    print("Start test for breakdown on empty splines:")

    def VerifyFirstBreakdown(s, t, flatTangents, length):
        s.Breakdown(t, Ts.KnotBezier, flatTangents, length)
        assert t in spline
        assert s[t].leftLen == length
        assert s[t].rightLen == length

    for ft in [True, False]:
        print("\tTest breakdown with flat tangents:", ft)

        spline = Ts.Spline()
        assert len(list(spline.keys())) == 0

        # Break down an empty spline with flat tangents. This should create a
        # keyframe with tangents of the given length.
        VerifyFirstBreakdown(spline, 5, flatTangents=ft, length=12.34)

        # Subsequent breakdowns should work similarly.
        VerifyFirstBreakdown(spline, -5, flatTangents=ft, length=34.56)
        VerifyFirstBreakdown(spline, 10, flatTangents=ft, length=45.67)

    print("\tPASSED")

TestBug12502()

########################################################################

def TestRedundantKeyFrames():
    def __AssertSplineKeyFrames(spline, baseline):
        keys = list(spline.keys())
        assert len(keys) == len(baseline)
        for index, (key, redundant) in enumerate(baseline):
            assert keys[index] == key
            assert spline.IsKeyFrameRedundant(keys[index]) == redundant

    print("Start test for redundant keyframes " \
          "(frames that don't change the animation).")
    
    print("\tTest spline with some redundant knots.")
    spline = createSpline([0, 1, 2, 3, 4, 5],
                          [0.0, 0.0, 1.0, 1.00000001, 1.0, 1.00001],
                          [Ts.KnotBezier, Ts.KnotBezier, Ts.KnotBezier, 
                           Ts.KnotBezier, Ts.KnotBezier, Ts.KnotBezier])
    keys = list(spline.keys())
    assert spline.IsKeyFrameRedundant(keys[0]) == True
    assert spline.IsKeyFrameRedundant(keys[1]) == False
    assert spline.IsKeyFrameRedundant(keys[2]) == False
    # diff < the hard coded epsilon
    assert spline.IsKeyFrameRedundant(keys[3]) == True
    # diff to next > the hard coded epsilon
    assert spline.IsKeyFrameRedundant(keys[4]) == False
    assert spline.IsKeyFrameRedundant(keys[5]) == False
    assert spline.IsSegmentFlat(keys[0], keys[1]) == True
    assert spline.IsSegmentFlat(keys[1], keys[2]) == False
    assert spline.IsSegmentFlat(keys[2], keys[3]) == True
    assert spline.IsSegmentFlat(keys[3], keys[4]) == True
    assert spline.IsSegmentFlat(keys[4], keys[5]) == False
    assert spline.HasRedundantKeyFrames() == True
    assert spline.IsVarying() == True

    print("\tTest key frame versions of functions")
    assert spline.IsKeyFrameRedundant(spline[0]) == True
    assert spline.IsKeyFrameRedundant(spline[1]) == False
    assert spline.IsKeyFrameRedundant(spline[2]) == False
    assert spline.IsKeyFrameRedundant(spline[3]) == True
    assert spline.IsKeyFrameRedundant(spline[4]) == False
    assert spline.IsKeyFrameRedundant(spline[5]) == False
    assert spline.IsSegmentFlat(spline[0], spline[1]) == True
    assert spline.IsSegmentFlat(spline[1], spline[2]) == False
    assert spline.IsSegmentFlat(spline[2], spline[3]) == True
    assert spline.IsSegmentFlat(spline[3], spline[4]) == True
    assert spline.IsSegmentFlat(spline[4], spline[5]) == False
    assert spline.HasRedundantKeyFrames() == True
    spline.ClearRedundantKeyFrames()
    __AssertSplineKeyFrames(spline, ((1, False), (2, False), (4, False),
        (5,False)))
    assert spline.IsVarying() == True

    print("\tTest passing in an interval to ClearRedundantKeyFrames.")
    spline = createSpline([0, 1, 2, 3],
                          [0.0, 0.0, 0.0, 0.0],
                          [Ts.KnotBezier, Ts.KnotBezier, Ts.KnotBezier, 
                           Ts.KnotBezier])
    keys = list(spline.keys())
    assert spline.IsKeyFrameRedundant(keys[0]) == True
    assert spline.IsKeyFrameRedundant(keys[1]) == True
    assert spline.IsKeyFrameRedundant(keys[2]) == True
    assert spline.IsKeyFrameRedundant(keys[3]) == True
    spline.ClearRedundantKeyFrames(intervals=Gf.MultiInterval(
        Gf.Interval(1, 2)))
    __AssertSplineKeyFrames(spline, ((0, True), (3, True)))

    print("\tTest that for looping spline first/last in master interval "\
          "not removed, even though redundant.")
    spline = createSpline([0, 10, 20, 30],
                          [0.0, 0.0, 0.0, 0.0],
                          [Ts.KnotBezier, Ts.KnotBezier, Ts.KnotBezier, 
                           Ts.KnotBezier])
    params = Ts.LoopParams(True, 10, 25, 0, 25, 0.0)
    spline.loopParams = params

    keys = list(spline.keys())
    assert spline.IsKeyFrameRedundant(keys[0]) == True
    assert spline.IsKeyFrameRedundant(keys[1]) == False
    assert spline.IsKeyFrameRedundant(keys[2]) == True
    assert spline.IsKeyFrameRedundant(keys[3]) == False
    assert spline.HasRedundantKeyFrames() == True
    assert spline.IsVarying() == False

    print("\tTest key frame versions of functions")
    assert spline.IsKeyFrameRedundant(spline[0]) == True
    assert spline.IsKeyFrameRedundant(spline[10]) == False
    assert spline.IsKeyFrameRedundant(spline[20]) == True
    assert spline.IsKeyFrameRedundant(spline[30]) == False
    spline.ClearRedundantKeyFrames()
    # The last two will be redundant, but will not have been removed
    # since they are in the (non-writable) echo region
    __AssertSplineKeyFrames(spline, ((10, False), (30, False), (35, True), 
        (55, True)))
    assert spline.IsVarying() == False

    print("\tTest empty spline.")
    spline = Ts.Spline()
    assert spline.HasRedundantKeyFrames() == False
    assert spline.IsVarying() == False

    print("\tTest spline with one knot that is redundant.")
    spline = createSpline([0], [0.0], [Ts.KnotBezier])
    keys = list(spline.keys())
    assert spline.IsKeyFrameRedundant(keys[0]) == False
    assert spline.IsKeyFrameRedundant(keys[0], 0.0) == True
    assert spline.IsKeyFrameRedundant(spline[0], 0.0) == True

    assert spline.HasRedundantKeyFrames() == False
    assert spline.HasRedundantKeyFrames(0.0) == True
    assert spline.IsVarying() == False
    spline.ClearRedundantKeyFrames()
    __AssertSplineKeyFrames(spline, ((0, False),))
    assert spline.IsVarying() == False

    print("\tTest that tangent slopes are respected when checking for redundancy.")
    spline = createSpline([0, 10], [0.0, 0.0], 
                          [Ts.KnotBezier, Ts.KnotBezier],
                          [1.0, 1.0, 1.0, 1.0], [1.0, 1.0, 1.0, 1.0])
    keys = list(spline.keys())
    assert spline.IsSegmentFlat(keys[0], keys[1]) == False
    assert spline.IsKeyFrameRedundant(keys[0]) == False
    assert spline.IsKeyFrameRedundant(keys[1]) == False
    assert spline.IsKeyFrameRedundant(keys[1], 0.0) == False

    assert spline.HasRedundantKeyFrames() == False
    assert spline.HasRedundantKeyFrames(0.0) == False
    assert spline.IsVarying() == True
    spline.ClearRedundantKeyFrames()
    __AssertSplineKeyFrames(spline, ((0, False), (10, False)))
    assert spline.IsVarying() == True

    print("\tTest that tangent slopes that are almost flat are treated as flat.")
    spline = createSpline([0, 10], [0.0, 0.0], 
                          [Ts.KnotBezier, Ts.KnotBezier],
                          [1e-10, 1e-10, 1e-10, 1e-10], 
                          [1e-10, 1e-10, 1e-10, 1e-10])
    keys = list(spline.keys())
    assert spline.IsSegmentFlat(keys[0], keys[1]) == True
    assert spline.IsKeyFrameRedundant(keys[0]) == True
    assert spline.IsKeyFrameRedundant(keys[1]) == True
    spline.ClearRedundantKeyFrames()
    __AssertSplineKeyFrames(spline, ((0, False),))

    print("\tTest dual valued knots.")
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(1, 0.0, 0.0, Ts.KnotBezier))
    spline.SetKeyFrame(Ts.KeyFrame(5, 0.0, 0.0, Ts.KnotBezier))
    assert spline.IsKeyFrameRedundant(spline[5], 0.0) == True
    spline.SetKeyFrame(Ts.KeyFrame(6, 0.0, 1.0, Ts.KnotBezier))
    assert spline.IsKeyFrameRedundant(spline[6]) == False
    assert spline.IsVarying() == True

    assert spline.HasRedundantKeyFrames() == True
    assert spline.HasRedundantKeyFrames(0.0) == True
    spline.ClearRedundantKeyFrames()
    __AssertSplineKeyFrames(spline, ((6, False),))

    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(0, 0.0, 0.0, Ts.KnotBezier))
    assert spline.IsKeyFrameRedundant(spline[0], 0.0) == True
    assert spline.IsKeyFrameRedundant(spline[0]) == False
    assert spline.IsVarying() == False

    assert spline.HasRedundantKeyFrames() == False
    assert spline.HasRedundantKeyFrames(0.0) == True
    spline.ClearRedundantKeyFrames()
    __AssertSplineKeyFrames(spline, ((0, False),))
    assert spline.IsVarying() == False

    # single knot with tangents, but held extrapolation
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(0, 1.0, Ts.KnotBezier,
                                     1.0, 1.0, 1.0, 1.0))
    assert spline.HasRedundantKeyFrames() == False
    assert spline.IsVarying() == False

    # same knot value with mixed knot types.
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(0, 1.0, Ts.KnotLinear))
    spline.SetKeyFrame(Ts.KeyFrame(1, 1.0, Ts.KnotBezier,
                                     1.0, 1.0, 1.0, 1.0))
    assert spline.HasRedundantKeyFrames() == False
    assert spline.IsVarying() == True

    # single knot with broken tangents, left side
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(0, 1.0, Ts.KnotBezier,
                                     -1.0, 0.0, 1.0, 0.0))
    spline.extrapolation = (Ts.ExtrapolationLinear,
                            Ts.ExtrapolationLinear)
    assert spline.HasRedundantKeyFrames() == False
    assert spline.IsVarying() == True

    # single knot with broken tangents, right side
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(0, 1.0, Ts.KnotBezier,
                                     0.0, 1.0, 0.0, 1.0))
    spline.extrapolation = (Ts.ExtrapolationLinear,
                            Ts.ExtrapolationLinear)
    assert spline.HasRedundantKeyFrames() == False
    assert spline.IsVarying() == True

    # single knot with broken tangents, held extrapolation
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(0, 1.0, Ts.KnotBezier,
                                     0.0, 1.0, 0.0, 1.0))
    assert spline.HasRedundantKeyFrames() == False
    assert spline.IsVarying() == False

    # dual-valued knot, same values
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(0, 1.0, 1.0, Ts.KnotLinear))
    assert spline.HasRedundantKeyFrames() == False
    assert spline.IsVarying() == False

    # dual-valued knot, different values
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(0, -1.0, 1.0, Ts.KnotLinear))
    assert spline.HasRedundantKeyFrames() == False
    assert spline.IsVarying() == True

    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(0, 0.0, 1.0, Ts.KnotBezier))
    assert spline.IsKeyFrameRedundant(spline[0]) == False
    assert spline.IsVarying() == True

    assert spline.HasRedundantKeyFrames() == False
    spline.ClearRedundantKeyFrames()
    __AssertSplineKeyFrames(spline, ((0, False),))
    assert spline.IsVarying() == True

    print("\tTest non-interpolatable key frame types.")
    spline = createSpline([0, 10, 20], ["foo", "bar", "bar"], 
                          [Ts.KnotLinear, Ts.KnotLinear, Ts.KnotLinear])
    keys = list(spline.keys())
    assert spline.IsSegmentFlat(keys[0], keys[1]) == False
    assert spline.IsSegmentFlat(keys[1], keys[2]) == True
    assert spline.IsKeyFrameRedundant(keys[0]) == False
    assert spline.IsKeyFrameRedundant(keys[1]) == False
    assert spline.IsKeyFrameRedundant(keys[2]) == True
    assert spline.IsVarying() == True

    assert spline.HasRedundantKeyFrames() == True
    spline.ClearRedundantKeyFrames()
    __AssertSplineKeyFrames(spline, ((0, False),(10, False)))
    assert spline.IsVarying() == True


    print("\tPASSED")

TestRedundantKeyFrames()

def TestMonotonicSegments():
    print("Start test for monotonic segments of keyframes")
    
    #test parabolic shape of the curve
    print("\tTest monotonic segments.")
    #Parabolic shape of the curve (a = 0)
    #10: 10.3684184945756 (bezier, 2.015878, 2.015878, 6.37363, 6.37363),
    #20: 10.8064368221441 (bezier, -5.034222, -5.034222, 1.42624, 1.42624),
    spline = createSpline([10, 20], [10.368, 10.806],                       
                          [Ts.KnotBezier, Ts.KnotBezier],              
                          [2.0159, 2.015878, -5.0342, -5.0342], 
                          [6.37363, 6.37363, 1.42624, 1.42624])
    keys = list(spline.keys())
    assert spline.IsSegmentValueMonotonic(keys[0], keys[1]) == False 



    #Minimum and a Maximum (hyperbola) first and second knots' values 
    # are relatively close
    #10: 10.3684184945756 (bezier, 8.001244, 8.001244, 1.92862, 1.92862),
    #20: 10.8064368221441 (bezier, 3.0872149, 3.0872149, 5.47192, 5.47192),
    spline = createSpline([10, 20], [10.368, 10.806],                       
                          [Ts.KnotBezier, Ts.KnotBezier],              
                          [8.001, 8.001, 3.0872, 3.0872], 
                          [1.929, 1.929, 5.4719, 5.4719])
    keys = list(spline.keys())
    assert spline.IsSegmentValueMonotonic(keys[0], keys[1]) == False 

    #Minimum and a Maximum (hyperbola) first and second knots' values 
    # are far apart 
    #10: -0.370986(bezier, 8.00124, 8.00124, 1.9286, 1.9286),
    #20: 10.806437(bezier, 3.08721, 3.08721, 5.4719, 5.4719),
    spline = createSpline([10, 20], [-0.370986, 10.806],                       
                          [Ts.KnotBezier, Ts.KnotBezier],              
                          [8.001, 8.001, 3.0872, 3.0872], 
                          [1.929, 1.929, 5.4719, 5.4719])
    keys = list(spline.keys())
    assert spline.IsSegmentValueMonotonic(keys[0], keys[1]) == False 

    #Almost vertical line, although value increases monotonically
    # the slope does change 
    #10: -0.37098(bezier, 4.7153, 4.7153, 1.0074, 1.0074),
    #11: 17.46758(bezier, 4.3937, 4.3937, 0.9674, 0.9674),
    spline = createSpline([10, 11], [-0.370986, 17.4676],                       
                          [Ts.KnotBezier, Ts.KnotBezier],              
                          [4.7153, 4.7153, 4.3937, 4.3937], 
                          [1.0074, 1.0074, 0.9674, 0.9674])
    keys = list(spline.keys())
    assert spline.IsSegmentValueMonotonic(keys[0], keys[1]) == True


    #Almost vertical line, although there's a dip at the beginning 
    # the minimum does exist 
    #10: -0.370985(bezier, -10.78057, -10.78057, 0.796848, 0.796848),
    #11: 17.467586(bezier, 4.3936, 4.3936, 0.9673, 0.9673),
    spline = createSpline([10, 11], [-0.370986, 17.4676],                       
                          [Ts.KnotBezier, Ts.KnotBezier],              
                          [-10.78057, -10.78057, 4.3937, 4.3937], 
                          [0.796848, 0.796848, 0.9674, 0.9674])
    keys = list(spline.keys())
    assert spline.IsSegmentValueMonotonic(keys[0], keys[1]) == False 

    # Almost horizontal line, (about a thosand of difference in time
    #  and only 0.1 difference in value), but still monotonically 
    #  increasing
    #-415: 8.8928(bezier, 0.3213, 0.3213, 0.01, 0.01),
    #795:  8.9012(bezier, 0.747, 0.747,   0.01, 0.01),
    spline = createSpline([-415, 795], [8.8928, 8.9012],                       
                          [Ts.KnotBezier, Ts.KnotBezier],              
                          [0.3213, 0.3213, 0.747, 0.747,], 
                          [0.01, 0.01, 0.01, 0.01])
    keys = list(spline.keys())
    assert spline.IsSegmentValueMonotonic(keys[0], keys[1]) == True 

    #Line that loops back on itself (in time) and does have a maximum
    # and minimum
    #10: -0.37098(bezier, 4.536694, 4.536694, 7.16636, 7.16636),
    #11: 17.46758(bezier, 13.002, 13.002, 2.5768, 2.5768),
    spline = createSpline([10, 11], [-0.370986, 17.4676],                       
                          [Ts.KnotBezier, Ts.KnotBezier],              
                          [4.5366, 4.5366, 13.002, 13.002], 
                          [7.166, 7.166, 2.5768, 2.5768])
    keys = list(spline.keys())
    assert spline.IsSegmentValueMonotonic(keys[0], keys[1]) == False
    
    #Line that loops back on itself (in time) but the value of 
    # which is monotonically increasing
    #10: -140.99  (bezier, 4.5367, 4.5367, 7.1663, 7.1663),
    #11: 17.4675  (bezier, 13.002, 13.002, 2.5768, 2.5768),
    spline = createSpline([10, 11], [-140.99, 17.4675],                       
                          [Ts.KnotBezier, Ts.KnotBezier],              
                          [4.5366, 4.5366, 13.002, 13.002], 
                          [7.166, 7.166, 2.5768, 2.5768])
    keys = list(spline.keys())
    assert spline.IsSegmentValueMonotonic(keys[0], keys[1]) == True

    print("\tPASSED")

TestMonotonicSegments()

def TestVarying():
    print("Start test for varying")

    print("\tTest value changes.")

    # An empty spline is not varying
    spline = Ts.Spline()
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()

    # Nor is a spline with a single knot
    spline = createSpline([0], [4.5], [Ts.KnotBezier])
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()

    # A spline with two knots is not varying if the values don't change
    spline = createSpline([0,1,2], [10.0,10.0,10.0],
                          [Ts.KnotBezier, Ts.KnotBezier, Ts.KnotBezier])
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()

    # But is varying if the values do change
    spline = createSpline([0,1,2], [10.0,12.0,10.0],
                          [Ts.KnotBezier, Ts.KnotBezier, Ts.KnotBezier])
    assert spline.IsVarying()
    assert spline.IsVaryingSignificantly()

    # Test specifying that knots close in value should be considered the same
    spline = createSpline([0,1,2], [10.0,10.00000001,10.0],
                          [Ts.KnotBezier, Ts.KnotBezier, Ts.KnotBezier])
    assert spline.IsVarying();
    assert not spline.IsVaryingSignificantly();

    # Similar, but the difference is further apart in time
    spline = createSpline([0,1,2,3], [10.0,10.00000001,10.0,10.00000002],
                          [Ts.KnotBezier, Ts.KnotBezier, Ts.KnotBezier,
                           Ts.KnotBezier])
    assert spline.IsVarying();
    assert not spline.IsVaryingSignificantly();

    print("\tTest tangent changes.")

    # Varying can also be achieved by tangent changes
    spline = createSpline([1, 10],
                          [10.0, 10.0],
                          [Ts.KnotBezier, Ts.KnotBezier],
                          [1, 1, -1, -1],
                          [1, 1, 1, 1])
    assert spline.IsVarying()
    assert spline.IsVaryingSignificantly()
    # But not if the tangents are flat
    spline = createSpline([1, 10],
                          [10.0, 10.0],
                          [Ts.KnotBezier, Ts.KnotBezier],
                          [0, 0, 0, 0],
                          [1, 1, 1, 1])
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()

    print("\tTest dual-valued knots.")

    # A spline with a single dual-valued knot with the same values
    # on both sides does not vary
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(1, 0.0, 0.0, Ts.KnotBezier))
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()

    # But does vary if the values differ
    spline = Ts.Spline()
    spline.SetKeyFrame(Ts.KeyFrame(1, 0.0, 1.0, Ts.KnotBezier))
    assert spline.IsVarying()
    assert spline.IsVaryingSignificantly()

    # Test some variations
    spline.SetKeyFrame(Ts.KeyFrame(1, 1.0, 1.0, Ts.KnotBezier))
    spline.SetKeyFrame(Ts.KeyFrame(2, 1.0, 0.0, Ts.KnotBezier))
    spline.SetKeyFrame(Ts.KeyFrame(3, 0.0, 0.0, Ts.KnotBezier))
    assert spline.IsVarying()
    assert spline.IsVaryingSignificantly()

    spline.SetKeyFrame(Ts.KeyFrame(1, 0.0, 1.0, Ts.KnotBezier))
    spline.SetKeyFrame(Ts.KeyFrame(2, 1.0, 1.0, Ts.KnotBezier))
    spline.SetKeyFrame(Ts.KeyFrame(3, 1.0, 0.0, Ts.KnotBezier))
    assert spline.IsVarying()
    assert spline.IsVaryingSignificantly()

    spline.SetKeyFrame(Ts.KeyFrame(1, 1.0, 1.0, Ts.KnotBezier))
    spline.SetKeyFrame(Ts.KeyFrame(2, 1.0, 1.0, Ts.KnotBezier))
    spline.SetKeyFrame(Ts.KeyFrame(3, 1.0, 1.0, Ts.KnotBezier))
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()

    print("\tTest extrapolation.")

    # Create a spline with a single knot and non-flat tangents
    spline = createSpline([1],
                          [10.0],
                          [Ts.KnotBezier],
                          [1, 1],
                          [1, 1])
    spline.extrapolation = (Ts.ExtrapolationHeld,
                            Ts.ExtrapolationHeld)

    # With held extrapoluation, this does not vary
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()
    spline.extrapolation = (Ts.ExtrapolationLinear,
                            Ts.ExtrapolationLinear)
    assert spline.IsVarying()
    assert spline.IsVaryingSignificantly()

    # Make sure this works on a per-side basis
    spline = createSpline([1],
                          [10.0],
                          [Ts.KnotBezier],
                          [0, 1],
                          [1, 1])
    spline.extrapolation = (Ts.ExtrapolationLinear,
                            Ts.ExtrapolationHeld)
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()
    spline.extrapolation = (Ts.ExtrapolationHeld,
                            Ts.ExtrapolationLinear)
    assert spline.IsVarying()
    assert spline.IsVaryingSignificantly()

    spline = createSpline([1],
                          [10.0],
                          [Ts.KnotBezier],
                          [1, 0],
                          [1, 1])
    spline.extrapolation = (Ts.ExtrapolationLinear,
                            Ts.ExtrapolationHeld)
    assert spline.IsVarying()
    assert spline.IsVaryingSignificantly()
    spline.extrapolation = (Ts.ExtrapolationHeld,
                            Ts.ExtrapolationLinear)
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()

    # And only if tangents aren't flat
    spline = createSpline([1],
                          [10.0],
                          [Ts.KnotBezier],
                          [0, 0],
                          [1, 1])
    spline.extrapolation = (Ts.ExtrapolationLinear,
                            Ts.ExtrapolationLinear)
    assert not spline.IsVarying()
    assert not spline.IsVaryingSignificantly()

    print("\tPASSED")

TestVarying()

def TestInfValues():
    print('Test infinite values')

    kf1 = Ts.KeyFrame(0.0, 0.0, Ts.KnotBezier)
    kf2 = Ts.KeyFrame(1.0, float('inf'), Ts.KnotBezier)
    kf3 = Ts.KeyFrame(2.0, 2.0, Ts.KnotBezier)
    kf4 = Ts.KeyFrame(3.0, float('-inf'), Ts.KnotBezier)

    # Only finite values are interpolatable.
    assert kf1.isInterpolatable
    assert not kf2.isInterpolatable
    assert kf3.isInterpolatable
    assert not kf4.isInterpolatable

    # Non-interpolatable values should be auto-converted to held knots.
    assert kf1.knotType == Ts.KnotBezier
    assert kf2.knotType == Ts.KnotHeld
    assert kf3.knotType == Ts.KnotBezier
    assert kf4.knotType == Ts.KnotHeld

    # Interpolatable values can be set to other knot types.
    assert kf1.CanSetKnotType( Ts.KnotLinear )
    assert kf3.CanSetKnotType( Ts.KnotLinear )

    # Non-interpolatable values cannot be set to anything but held.
    verifyNegativeCanResult(kf2.CanSetKnotType(Ts.KnotLinear))
    verifyNegativeCanResult(kf4.CanSetKnotType(Ts.KnotLinear))

    # Setting a knot to a non-finite value should convert it to held.
    kfTest = Ts.KeyFrame(0, 0.0, Ts.KnotBezier)
    assert kfTest.knotType != Ts.KnotHeld
    kfTest.value = float('inf')
    assert kfTest.knotType == Ts.KnotHeld

    # Check eval'ing a spline with non-interpolatable values.
    #
    # A non-interpolatable keyframe is treated as held,
    # and also forces the prior keyframe to be treated as held.
    #
    s = Ts.Spline([kf1, kf2, kf3, kf4])
    assert s.Eval(0.0) == 0
    assert s.Eval(0.5) == 0
    assert s.Eval(1.0) == float('inf')
    assert s.Eval(1.5) == float('inf')
    assert s.Eval(2.0) == 2
    assert s.Eval(2.5) == 2
    assert s.Eval(3.0) == float('-inf')
    assert s.Eval(3.5) == float('-inf')
    assert s.EvalDerivative(0.0) == 0
    assert s.EvalDerivative(0.5) == 0
    assert s.EvalDerivative(1.0) == 0
    assert s.EvalDerivative(1.5) == 0
    assert s.EvalDerivative(2.0) == 0
    assert s.EvalDerivative(2.5) == 0
    assert s.EvalDerivative(3.0) == 0
    assert s.EvalDerivative(3.5) == 0

    print("\tPASSED")

TestInfValues()

def TestEvalHeld():
    print("\nStart EvalHeld test")
    spline = Ts.Spline([
            Ts.KeyFrame(5, 2.0, Ts.KnotHeld),
            Ts.KeyFrame(10, -3.0, Ts.KnotBezier,
                        0.0, 0.0, 1 + 2/3.0, 3 + 1/3.0),
            Ts.KeyFrame(20, 16.0, Ts.KnotBezier, 2.0, 3.0, 4.0, 8.0),
            Ts.KeyFrame(40, 0.0, Ts.KnotLinear),
        ],
        Ts.ExtrapolationHeld,
        Ts.ExtrapolationLinear,
        )
    spline.extrapolation = (Ts.ExtrapolationHeld, Ts.ExtrapolationLinear)

    print("\tTest Eval and EvalHeld in varying regions")
    assert spline.Eval(10) == -3.0
    assert Gf.IsClose(spline.Eval(15), 4.098398216, EPSILON)
    assert spline.EvalHeld(10) == -3.0
    assert spline.EvalHeld(15) == -3.0

    print("\tTest that Eval and EvalHeld are the same for held keyframes")
    assert spline.Eval(0) == 2.0
    assert spline.Eval(5) == 2.0
    assert spline.Eval(7) == 2.0
    assert spline.EvalHeld(0) == 2.0
    assert spline.EvalHeld(5) == 2.0
    assert spline.EvalHeld(7) == 2.0

    print("\tTest that the left side of a held frame takes its value from " +
          "the previous hold")
    assert spline.Eval(20, Ts.Left) == 16.0
    assert spline.Eval(20) == 16.0
    assert spline.EvalHeld(20, Ts.Left) == -3.0
    assert spline.EvalHeld(20) == 16.0

    print("\tTest that holding ignores extrapolation")
    assert spline.Eval(40) == 0
    assert spline.Eval(50) == -8.0
    assert spline.EvalHeld(40) == 0.0
    assert spline.EvalHeld(50) == 0.0

    print("\tPASSED")

TestEvalHeld()

def TestHeldAtLeftSideOfFollowingKnot():
    """
    Verify that the left evaluated value of a knot that follows a held knot is
    always the value of the held knot.  This verifies the fix for PRES-89202.
    """
    s = Ts.Spline([
        # Held, then held.
        Ts.KeyFrame(
            time = 1.0, value = 2.0,
            knotType = Ts.KnotHeld),
        Ts.KeyFrame(
            time = 2.0, value = 3.0,
            knotType = Ts.KnotHeld),

        # Held, then linear.
        Ts.KeyFrame(
            time = 3.0, value = 4.0,
            knotType = Ts.KnotHeld),
        Ts.KeyFrame(
            time = 4.0, value = 5.0,
            knotType = Ts.KnotLinear),

        # Held, then dual Bezier.
        Ts.KeyFrame(
            time = 5.0, value = 6.0,
            knotType = Ts.KnotHeld),
        Ts.KeyFrame(
            time = 6.0, leftValue = 7.0, rightValue = 8.0,
            knotType = Ts.KnotBezier),
    ])

    print("\nStart held-then-left test")

    # Held, then held.  At the left side of the second knot, that knot's own
    # value should be ignored.  This worked correctly even before PRES-89202 was
    # fixed.
    assert s.Eval(time = 1.0, side = Ts.Right) == 2.0
    assert s.Eval(time = 2.0, side = Ts.Left) == 2.0
    assert s.Eval(time = 2.0, side = Ts.Right) == 3.0

    # Held, then linear.  At the left side of the second knot, that knot's own
    # value should be ignored.
    assert s.Eval(time = 3.0, side = Ts.Right) == 4.0
    assert s.Eval(time = 4.0, side = Ts.Left) == 4.0  # Torture case
    assert s.Eval(time = 4.0, side = Ts.Right) == 5.0

    # Held, then dual Bezier.  At the left side of the second knot, that knot's
    # own left value should be ignored.
    assert s.Eval(time = 5.0, side = Ts.Right) == 6.0
    assert s.Eval(time = 6.0, side = Ts.Left) == 6.0  # Torture case
    assert s.Eval(time = 6.0, side = Ts.Right) == 8.0

    print("\tPASSED")

TestHeldAtLeftSideOfFollowingKnot()

def TestDoSidesDiffer():
    """
    Exercise TsSpline::DoSidesDiffer.
    """
    s = Ts.Spline([
        Ts.KeyFrame(
            time = 1.0, value = 2.0,
            knotType = Ts.KnotHeld),
        Ts.KeyFrame(
            time = 2.0, leftValue = 3.0, rightValue = 4.0,
            knotType = Ts.KnotHeld),
        Ts.KeyFrame(
            time = 3.0, value = 5.0,
            knotType = Ts.KnotLinear),
        Ts.KeyFrame(
            time = 4.0, value = 6.0,
            knotType = Ts.KnotHeld),
        Ts.KeyFrame(
            time = 5.0, value = 7.0,
            knotType = Ts.KnotBezier,
            leftSlope = 0.0, rightSlope = 0.0, leftLen = 0.0, rightLen = 0.0),
        Ts.KeyFrame(
            time = 6.0, value = 8.0,
            knotType = Ts.KnotBezier,
            leftSlope = 0.0, rightSlope = 0.0, leftLen = 0.0, rightLen = 0.0),
        Ts.KeyFrame(
            time = 7.0, leftValue = 9.0, rightValue = 10.0,
            knotType = Ts.KnotBezier,
            leftSlope = 0.0, rightSlope = 0.0, leftLen = 0.0, rightLen = 0.0),
        Ts.KeyFrame(
            time = 8.0, leftValue = 11.0, rightValue = 11.0,
            knotType = Ts.KnotBezier,
            leftSlope = 0.0, rightSlope = 0.0, leftLen = 0.0, rightLen = 0.0),
        Ts.KeyFrame(
            time = 9.0, value = 12.0,
            knotType = Ts.KnotHeld),
        Ts.KeyFrame(
            time = 10.0, value = 12.0,
            knotType = Ts.KnotBezier,
            leftSlope = 0.0, rightSlope = 0.0, leftLen = 0.0, rightLen = 0.0),
    ])

    def _TestNonDiff(time):
        assert not s.DoSidesDiffer(time)
        assert s.Eval(time, Ts.Left) == s.Eval(time, Ts.Right)

    def _TestDiff(time):
        assert s.DoSidesDiffer(time)
        assert s.Eval(time, Ts.Left) != s.Eval(time, Ts.Right)

    print("\nStart DoSidesDiffer test\n")

    # At a non-dual held knot with no preceding knot: false.
    _TestNonDiff(time = 1.0)

    # Between knots: false.
    _TestNonDiff(time = 1.5)

    # At a dual-valued held knot: true.
    _TestDiff(time = 2.0)

    # At a linear knot following a held knot: true.
    _TestDiff(time = 3.0)

    # At a non-dual held knot: false.
    _TestNonDiff(time = 4.0)

    # At a Bezier knot following a held knot: true.
    _TestDiff(time = 5.0)

    # At an ordinary Bezier knot: false.
    _TestNonDiff(time = 6.0)

    # At a dual-valued Bezier knot: true.
    _TestDiff(time = 7.0)

    # At a dual-valued Bezier knot with both sides the same: false.
    _TestNonDiff(time = 8.0)

    # At a Bezier knot following a held knot with same value: false.
    _TestNonDiff(time = 10.0)

    print("\tPASSED")

TestDoSidesDiffer()


print('\nTest SUCCEEDED')
