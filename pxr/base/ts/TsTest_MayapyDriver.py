#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

#
################################################################################
# Runs inside mayapy.
# Started by, and communicates via pipe with, TsTest_MayapyEvaluator.
# Tested against Maya 2023 Update 3, Linux (Python 3.9.7).
################################################################################
#

import maya.standalone
from maya.api.OpenMayaAnim import MFnAnimCurve as Curve
from maya.api.OpenMaya import MTime
import sys


gDebugFilePath = None


class Ts(object):
    """
    POD classes coresponding to TsTest classes.  Rather than try to get libTs to
    import into mayapy, we just use these equivalents as containers, so that we
    can use eval and repr to pass objects in and out of this script.
    """
    class TsTest_SplineData(object):

        InterpHeld = 0
        InterpLinear = 1
        InterpCurve = 2

        ExtrapHeld = 0
        ExtrapLinear = 1
        ExtrapSloped = 2
        ExtrapLoop = 3

        LoopNone = 0
        LoopContinue = 1
        LoopRepeat = 2
        LoopReset = 3
        LoopOscillate = 4

        def __init__(self,
                     isHermite,
                     preExtrapolation, postExtrapolation,
                     knots = [],
                     innerLoopParams = None):
            self.isHermite = isHermite
            self.preExtrapolation = preExtrapolation
            self.postExtrapolation = postExtrapolation
            self.knots = knots
            self.innerLoopParams = innerLoopParams

        class Knot(object):
            def __init__(self,
                         time, nextSegInterpMethod, value,
                         preSlope, postSlope, preLen, postLen, preAuto, postAuto,
                         leftValue = None):
                self.time = time
                self.nextSegInterpMethod = nextSegInterpMethod
                self.value = value
                self.preSlope = preSlope
                self.postSlope = postSlope
                self.preLen = preLen
                self.postLen = postLen
                self.preAuto = preAuto
                self.postAuto = postAuto
                self.leftValue = leftValue

        class InnerLoopParams(object):
            def __init__(self,
                         enabled,
                         protoStart, protoLen,
                         numPreLoops, numPostLoops):
                self.enabled = enabled
                self.protoStart = protoStart
                self.protoLen = protoLen
                self.numPreLoops = numPreLoops
                self.numPostLoops = numPostLoops

        class Extrapolation(object):
            def __init__(self, method, slope = 0.0, loopMode = 0):
                self.method = method
                self.slope = slope
                self.loopMode = loopMode

    class TsTest_SampleTimes(object):

        def __init__(self, times):
            self.times = times

        class SampleTime(object):
            def __init__(self, time, left):
                self.time = time
                self.left = left

    class TsTest_Sample(object):

        def __init__(self, time, value):
            self.time = float(time)
            self.value = float(value)

        def __repr__(self):
            return "Ts.TsTest_Sample(" \
                "float.fromhex('%s'), float.fromhex('%s'))" \
                % (float.hex(self.time), float.hex(self.value))

# Convenience abbreviation
SData = Ts.TsTest_SplineData


_InterpTypeMap = {
    SData.InterpHeld: Curve.kTangentStep,
    SData.InterpLinear: Curve.kTangentLinear,
    SData.InterpCurve: Curve.kTangentGlobal
}

_AutoTypeMap = {
    "auto": Curve.kTangentAuto,
    "smooth": Curve.kTangentSmooth
}

_ExtrapTypeMap = {
    SData.ExtrapHeld: Curve.kConstant,
    SData.ExtrapLinear: Curve.kLinear,
    SData.ExtrapSloped: Curve.kLinear,
}

_ExtrapLoopMap = {
    SData.LoopRepeat: Curve.kCycleRelative,
    SData.LoopReset: Curve.kCycle,
    SData.LoopOscillate: Curve.kOscillate
}

def _GetTanType(tanType, isAuto, opts):

    if tanType != Curve.kTangentGlobal \
            or not isAuto:
        return tanType

    return _AutoTypeMap[opts["autoTanMethod"]]

def SetUpCurve(curve, data, opts):

    # types: Curve.kTangent{Global,Linear,Step,Smooth,Auto}
    # XXX:
    # figure out what global means
    # verify fixed, flat, clamped, and plateau aren't needed (auth hints)
    # autodesk said 3 smooth types - what's the third?

    # Maya Bezier tangent specifications:
    # X coordinates are time, Y coordinates are value.
    # They specify tangent endpoints.
    # They are relative to keyframe time and value.
    # Both are negated for in-tangents (+X = longer, +Y = pointing down).
    # X and Y values must be multiplied by 3.

    tanType = Curve.kTangentGlobal
    prevValue = None

    times = []
    values = []
    tanInTypes = []
    tanInXs = []
    tanInYs = []
    tanOutTypes = []
    tanOutXs = []
    tanOutYs = []

    # Build keyframe data.
    for knot in data.knots:

        if knot.leftValue is not None:
            raise Exception("Maya splines can't use dual-valued knots")

        # XXX: if two consecutive knots have identical values, mayapy often
        # (always?)  evaluates the entire segment as flat.  A tiny difference
        # appears to be enough to prevent this.
        if knot.value != prevValue:
            value = knot.value
        else:
            value = knot.value + 1e-100
        prevValue = value

        # Time and value.
        times.append(knot.time)
        values.append(value)

        if data.isHermite:
            # Hermite spline.  Tan lengths may be zero and are ignored.  Any
            # nonzero length will allow us to establish a slope in X and Y, so
            # use length 1.
            preLen = 1.0
            postLen = 1.0
        else:
            # Non-Hermite spline.  Use tan lengths as specified.  Multiply by 3.
            preLen = knot.preLen * 3.0
            postLen = knot.postLen * 3.0

        # Use previous segment type as in-tan type.
        tanInTypes.append(_GetTanType(tanType, knot.preAuto, opts))
        tanInXs.append(preLen)
        tanInYs.append(knot.preSlope * preLen)

        # Determine new out-tan type and record that for the next in-tan.
        tanType = _InterpTypeMap[knot.nextSegInterpMethod]
        tanOutTypes.append(_GetTanType(tanType, knot.postAuto, opts))
        tanOutXs.append(postLen)
        tanOutYs.append(knot.postSlope * postLen)

    # Implement linear & sloped pre-extrap with explicit linear tangents.
    if data.preExtrapolation.method == SData.ExtrapLinear:
        tanInTypes[0] = Curve.kTangentLinear
        if len(times) == 1 or tanOutTypes[0] == Curve.kTangentStep:
            tanInXs[0] = 1.0
            tanInYs[0] = 0.0
        elif tanOutTypes[0] == Curve.kTangentLinear:
            tanInXs[0] = times[1] - times[0]
            tanInYs[0] = values[1] - values[0]
        else:
            tanInXs[0] = tanOutXs[0]
            tanInYs[0] = tanOutYs[0]
    elif data.preExtrapolation.method == SData.ExtrapSloped:
        tanInTypes[0] = Curve.kTangentLinear
        tanInXs[0] = 1.0
        tanInYs[0] = data.preExtrapolation.slope

    # Implement linear & sloped post-extrap with explicit linear tangents.
    if data.postExtrapolation.method == SData.ExtrapLinear:
        tanOutTypes[-1] = Curve.kTangentLinear
        if len(times) == 1 or tanInTypes[-1] == Curve.kTangentStep:
            tanOutXs[-1] = 1.0
            tanOutYs[-1] = 0.0
        elif tanInTypes[-1] == Curve.kTangentLinear:
            tanOutXs[-1] = times[-1] - times[-2]
            tanOutYs[-1] = values[-1] - values[-2]
        else:
            tanOutXs[-1] = tanInXs[-1]
            tanOutYs[-1] = tanInYs[-1]
    elif data.postExtrapolation.method == SData.ExtrapSloped:
        tanOutTypes[-1] = Curve.kTangentLinear
        tanOutXs[-1] = 1.0
        tanOutYs[-1] = data.postExtrapolation.slope

    # Debug dump.
    _Debug("times: %s" % times)
    _Debug("values: %s" % values)
    _Debug("tanInTypes: %s" % tanInTypes)
    _Debug("tanInXs: %s" % tanInXs)
    _Debug("tanInYs: %s" % tanInYs)
    _Debug("tanOutTypes: %s" % tanOutTypes)
    _Debug("tanOutXs: %s" % tanOutXs)
    _Debug("tanOutYs: %s" % tanOutYs)

    # Set overall spline flags.
    curve.setIsWeighted(not data.isHermite)

    # Set pre-infinity type.
    if data.preExtrapolation.method == SData.ExtrapLoop:
        curve.setPreInfinityType(
            _ExtrapLoopMap[data.preExtrapolation.loopMode])
    else:
        curve.setPreInfinityType(
            _ExtrapTypeMap[data.preExtrapolation.method])

    # Set post-infinity type.
    if data.postExtrapolation.method == SData.ExtrapLoop:
        curve.setPostInfinityType(
            _ExtrapLoopMap[data.postExtrapolation.loopMode])
    else:
        curve.setPostInfinityType(
            _ExtrapTypeMap[data.postExtrapolation.method])

    # XXX: I haven't been able to find a way to add diverse keyframes without
    # API problems.  So far, the most effective workaround appears to be (1)
    # call addKeysWithTangents; (2) call set{In,Out}TangentType for certain
    # types.

    # Create keyframes.
    curve.addKeysWithTangents(
        [MTime(t) for t in times], values,
        tangentInTypeArray = tanInTypes,
        tangentInXArray = tanInXs, tangentInYArray = tanInYs,
        tangentOutTypeArray = tanOutTypes,
        tangentOutXArray = tanOutXs, tangentOutYArray = tanOutYs)

    # Re-establish certain tangent types that seem to get confused by
    # addKeysWithTangents.
    debugInTypes = []
    debugOutTypes = []
    for i in range(len(data.knots)):
        if data.knots[i].preAuto:
            curve.setInTangentType(i, tanInTypes[i])
        if data.knots[i].nextSegInterpMethod == SData.InterpHeld \
                or data.knots[i].postAuto:
            curve.setOutTangentType(i, tanOutTypes[i])
        debugInTypes.append(curve.inTangentType(i))
        debugOutTypes.append(curve.outTangentType(i))

    # Debug after resetting types.
    _Debug("tanInTypes again: %s" % debugInTypes)
    _Debug("tanOutTypes again: %s" % debugOutTypes)

def DoEval(data, times, opts):

    if data.innerLoopParams:
        raise Exception("Maya splines can't use inner loops")

    if (data.preExtrapolation.loopMode == SData.LoopContinue
            or data.postExtrapolation.loopMode == SData.LoopContinue):
        raise Exception("Maya splines can't use LoopContinue")

    result = []

    curve = Curve()
    curveObj = curve.create(Curve.kAnimCurveTU)

    if data.knots:
        SetUpCurve(curve, data, opts)

    for sampleTime in times.times:

        # Emulate left-side evaluation by subtracting an arbitrary tiny time
        # delta (but large enough to avoid Maya snapping it to a knot time).  We
        # will return a sample with a differing time, which will allow the
        # result to be understood as a small delta rather than an instantaneous
        # change.
        time = sampleTime.time
        if sampleTime.left:
            time -= 1e-5

        value = curve.evaluate(MTime(time))
        result.append(Ts.TsTest_Sample(time, value))

    return result


def _Debug(text, truncate = False):

    if not gDebugFilePath:
        return

    # Open, write, and close every time.
    # This is slow, but reliable.
    mode = "w" if truncate else "a"
    with open(gDebugFilePath, mode) as outfile:
        print(text, file = outfile)

if __name__ == "__main__":

    # Accept optional debug file path command-line arg.
    if len(sys.argv) > 1:
        gDebugFilePath = sys.argv[1]

    # Initialize, then signal readiness with an empty line on stdout.
    # The initialize() call can take a long time, like 30 seconds.
    _Debug("Initializing...", truncate = True)
    maya.standalone.initialize("Python")
    _Debug("Done initializing")
    print()
    sys.stdout.flush()

    # Loop until we're killed.
    while True:
        # Read until newline, then eval to deserialize.
        _Debug("Reading...")
        data, times, opts = eval(sys.stdin.readline())
        _Debug("Done reading")

        # Perform spline evaluation.
        result = DoEval(data, times, opts)

        # Serialize output with repr.  The print() function includes a
        # newline terminator, which will signal end of output.
        _Debug("Writing...")
        print(repr(result))
        sys.stdout.flush()
