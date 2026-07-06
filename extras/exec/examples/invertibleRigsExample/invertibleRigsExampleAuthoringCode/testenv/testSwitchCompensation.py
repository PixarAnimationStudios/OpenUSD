#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import ExecIr, Gf, Sdf, Ts, Usd, InvertibleRigsExampleAuthoringCode
import unittest

def _SetSplineKnot(attr, time, value):
    """
    Sets a value on an attribute at a specified time by writing a knot to
    the attribute's spline. If no knot exists at that time, creates a new
    knot with curve interpolation for the segment that follows it.
    """
    valueType = attr.GetTypeName().type

    frame = time.GetValue()
    spline = attr.GetSpline()
    knot = spline.GetKnot(frame)
    if knot:
        knot.SetValue(value)
    else:
        knot = Ts.Knot(
            typeName=valueType.typeName, time=frame, value=value,
            nextInterp=Ts.InterpCurve)
    spline.SetKnot(knot)
    attr.SetSpline(spline)

def _AssertAvarValues(time, avars, expectedValues):
    """
    Asserts that the values of the given avars at the given time are close
    to the given expected values.
    """

    for avar, expectedValue in zip(avars, expectedValues):
        actualValue = avar.Get(time)
        if isinstance(actualValue, str):
            assert actualValue == expectedValue, \
                   f"For avar <{avar.GetPath()}> " \
                   f"'{actualValue}' != '{expectedValue}'"
        else:
            assert Gf.IsClose(actualValue, expectedValue, 1e-6), \
                   f"For avar <{avar.GetPath()}> " \
                   f"{actualValue} != {expectedValue}"

class TestSwitchCompensation(unittest.TestCase):

    def test_CompensateSwitch(self):
        """
        Test compensation on a switch controller.

        Switching control rigs with compensation is a primary workflow supported
        by invertible rigs. The value of the switch controller 'switch'
        attribute determines which control rig is used to compute the rig's pose
        at any given time. If we simply author a new value to the 'switch'
        attribute, then the resulting pose will, in general, change
        discontinuously. Compensation invokes inversion to compute the input
        avar values that preserve the pose and authors those values, along with
        the new 'switch' value, changing the active control rig such that the
        pose is continuous across the transition.
        """

        layer = Sdf.Layer.FindOrOpen("shot.usda")
        stage = Usd.Stage.Open(layer)

        switch = stage.GetPrimAtPath('/Root/Anim') \
                 .GetAttribute(ExecIr.Tokens.switch_)
        assert switch

        joint1 = stage.GetPrimAtPath('/Root/Anim/Joint1')
        joint2 = stage.GetPrimAtPath('/Root/Anim/Joint1/Joint2')
        assert joint1 and joint2

        ry1 = joint1.GetAttribute(ExecIr.Tokens.avarsRy)
        ry2 = joint2.GetAttribute(ExecIr.Tokens.avarsRy)
        assert ry1 and ry2

        inputAvarNames = (
            ExecIr.Tokens.avarsTx,
            ExecIr.Tokens.avarsTy,
            ExecIr.Tokens.avarsTz,
            ExecIr.Tokens.avarsRx,
            ExecIr.Tokens.avarsRy,
            ExecIr.Tokens.avarsRz,
            ExecIr.Tokens.avarsRspin)
        inputAvars = [prim.GetAttribute(name)
                      for prim in (joint1, joint2)
                      for name in inputAvarNames]

        authoring = InvertibleRigsExampleAuthoringCode.Authoring(stage)

        # Start at time 0 by breaking down default values into splines for all
        # input avars.
        currentTime = Usd.TimeCode(0.0)
        authoring.BreakdownInputAvars(switch, currentTime)

        _AssertAvarValues(
            currentTime, inputAvars,
            (0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0))

        # Author animation on rotation avars and a series of compensated changes
        # to the switch attribute that make the rig tumble by alternately
        # pivoting about one end or the other.

        currentTime = Usd.TimeCode(10.0)
        _SetSplineKnot(ry1, currentTime, 90.0)
        authoring.CompensateSwitch(switch, currentTime, ExecIr.Tokens.rig2)

        _AssertAvarValues(
            currentTime, inputAvars,
            (0, 0, 0, 0, 0, 0, 0,
             10, 0, -10, 0, 90, 0, 0))

        currentTime = Usd.TimeCode(30.0)
        _SetSplineKnot(ry2, currentTime, 270.0)
        authoring.CompensateSwitch(switch, currentTime, ExecIr.Tokens.rig1)

        _AssertAvarValues(
            currentTime, inputAvars,
            (20, 0, 0, 0, -90, 0, 0,
             0, 0, 0, 0, 0, 0, 0))

        currentTime = Usd.TimeCode(50.0)
        _SetSplineKnot(ry1, currentTime, 90.0)
        authoring.CompensateSwitch(switch, currentTime, ExecIr.Tokens.rig2)
    
        _AssertAvarValues(
            currentTime, inputAvars,
            (0, 0, 0, 0, 0, 0, 0,
             30, 0, -10, 0, 90, 0, 0))

        currentTime = Usd.TimeCode(60.0)
        _SetSplineKnot(ry2, currentTime, 180.0)

        layer.Export("result.usda")

    def test_BreakdownInputAvars(self):
        """
        Test the breakdown operation.

        Breakdown takes the value of an attribute at a given time and authors a
        spline knot of that value at that time on that attribute. See
        Ts.Spline.Breakdown.
        """

        def _SetSplineKnotNextInterp(attr, time, interp):
            """
            Find the knot at the given time on attr's spline and set the
            interpolation mode for the segment after that time to the given
            interpolation type.
            """
            spline = attr.GetSpline()
            knot = spline.GetKnot(time.GetValue())
            knot.SetNextInterpolation(interp)
            spline.SetKnot(knot)
            attr.SetSpline(spline)
        
        layer = Sdf.Layer.FindOrOpen("shot.usda")
        stage = Usd.Stage.Open(layer)

        switch = stage.GetPrimAtPath('/Root/Anim') \
                 .GetAttribute(ExecIr.Tokens.switch_)
        assert switch

        joint1 = stage.GetPrimAtPath('/Root/Anim/Joint1')
        joint2 = stage.GetPrimAtPath('/Root/Anim/Joint1/Joint2')
        assert joint1 and joint2

        inputAvarNames = (
            ExecIr.Tokens.avarsTx,
            ExecIr.Tokens.avarsTy,
            ExecIr.Tokens.avarsTz,
            ExecIr.Tokens.avarsRx,
            ExecIr.Tokens.avarsRy,
            ExecIr.Tokens.avarsRz,
            ExecIr.Tokens.avarsRspin)
        inputAvars = [prim.GetAttribute(name)
                      for prim in (joint1, joint2)
                      for name in inputAvarNames]
        inputAvars.append(switch)

        authoring = InvertibleRigsExampleAuthoringCode.Authoring(stage)

        # Author a default to one of the input avars.
        inputAvars[0].Set(10.0)

        # At time 0, break down default (and fallback)values into splines for
        # all input avars.
        currentTime = Usd.TimeCode(0.0)
        authoring.BreakdownInputAvars(switch, currentTime)

        # Verify avar default values have been preserved as spline knots at the
        # current time.
        _AssertAvarValues(
            currentTime, inputAvars,
            (10, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0,
             ExecIr.Tokens.rig1))

        # At time 10, set a knot on a different input avar and then break down.
        currentTime = Usd.TimeCode(10.0)
        _SetSplineKnot(inputAvars[1], currentTime, 20.0)
        authoring.BreakdownInputAvars(switch, currentTime)
        _AssertAvarValues(
            currentTime, inputAvars,
            (10, 20, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0,
             ExecIr.Tokens.rig1))

        # Change the interpolation mode for the spline segments following time
        # 10 to linear, to make it easy to reason about interpolated values.
        _SetSplineKnotNextInterp(inputAvars[0], currentTime, Ts.InterpLinear)
        _SetSplineKnotNextInterp(inputAvars[1], currentTime, Ts.InterpLinear)

        # At time 20, set knots on the two input avars we've been manipulating
        # and author a time sample on the switch attribute.
        currentTime = Usd.TimeCode(20.0)
        _SetSplineKnot(inputAvars[0], currentTime, 0.0)
        _SetSplineKnot(inputAvars[1], currentTime, 40.0)
        switch.Set(ExecIr.Tokens.rig2, currentTime)
        _AssertAvarValues(
            currentTime, inputAvars,
            (0, 40, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0,
             ExecIr.Tokens.rig2))

        # At time 15, verify we get the expected interpolated values, then
        # breakdown.
        currentTime = Usd.TimeCode(15.0)
        _AssertAvarValues(
            currentTime, inputAvars,
            (5, 30, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0,
             ExecIr.Tokens.rig1))
        authoring.BreakdownInputAvars(switch, currentTime)

        # At time 18, set knots on the two input avars we've been manipulating.
        currentTime = Usd.TimeCode(18.0)
        _SetSplineKnot(inputAvars[0], currentTime, 100.0)
        _SetSplineKnot(inputAvars[1], currentTime, 200.0)

        # Verify that the breakdown at time 15 preserved values.
        currentTime = Usd.TimeCode(15.0)
        _AssertAvarValues(
            currentTime, inputAvars,
            (5, 30, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0,
             ExecIr.Tokens.rig1))
        
if __name__ == "__main__":
    unittest.main()
