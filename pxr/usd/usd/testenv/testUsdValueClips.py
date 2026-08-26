#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import contextlib
import math
import os
import shutil
import sys
import unittest
from pxr import Sdf, Tf, Ts, Usd, Vt, Gf

@contextlib.contextmanager
def InterpolationType(stage, interpolationType):
    oldInterpolationType = stage.GetInterpolationType()
    try:
        stage.SetInterpolationType(interpolationType)
        yield
    finally:
        stage.SetInterpolationType(oldInterpolationType)

@contextlib.contextmanager
def LayerChangeListener():
    class _Listener(object):
        def __init__(self):
            self.changedLayers = []
            self._listener = Tf.Notice.RegisterGlobally(
                Sdf.Notice.LayersDidChange, self._HandleNotice)
        def _HandleNotice(self, notice, sender):
            self.changedLayers += notice.GetLayers()

    l = _Listener()
    yield l

class TestUsdValueClips(unittest.TestCase):
    def CheckTimeSamples(self, attr):
        """Verifies attribute time samples are as expected via
        the time sample API"""
        allTimeSamples = attr.GetTimeSamples()
        self.assertEqual(attr.GetNumTimeSamples(), len(allTimeSamples))
        for i in range(0, len(allTimeSamples) - 1):
            (lowerSample, upperSample) = allTimeSamples[i], allTimeSamples[i+1]
        
            # The attribute's bracketing time samples at each time returned
            # by GetTimeSamples() should be equal to the time.
            self.assertEqual(attr.GetBracketingTimeSamples(lowerSample), 
                             (lowerSample, lowerSample))
            self.assertEqual(attr.GetBracketingTimeSamples(upperSample), 
                             (upperSample, upperSample))

            # The attribute's bracketing time samples should be the same
            # at every time in the interval (lowerSample, upperSample)
            for t in range(int(lowerSample) + 1, int(upperSample)):
                self.assertEqual(attr.GetBracketingTimeSamples(t), 
                                 (lowerSample, upperSample))

            # Check the midpoint between lower and upper samples as an
            # extra sanity check -- this catches issues for non-integer
            # time sample times.
            if lowerSample != upperSample:
                self.assertEqual(
                    attr.GetBracketingTimeSamples(
                        lowerSample + ((upperSample - lowerSample) / 2.0)),
                    (lowerSample, upperSample))

            # The attribute should return the same value at every time in the
            # interval [lowerSample, upperSample) if the stage's interpolation
            # type is held.
            with InterpolationType(attr.GetStage(), Usd.InterpolationTypeHeld):
                for t in range(int(lowerSample) + 1, int(upperSample)):
                    self.assertEqual(attr.Get(t), attr.Get(lowerSample))

        # Verify that the value before the first time sample and after the
        # last time sample are held.
        if len(allTimeSamples) > 0:
            firstTimeSample = min(allTimeSamples)
            self.assertEqual(attr.GetBracketingTimeSamples(firstTimeSample - 1),
                             (firstTimeSample, firstTimeSample))
            self.assertEqual(attr.Get(firstTimeSample - 1), 
                             attr.Get(firstTimeSample))

            lastTimeSample = max(allTimeSamples)
            self.assertEqual(attr.GetBracketingTimeSamples(lastTimeSample + 1),
                             (lastTimeSample, lastTimeSample))
            self.assertEqual(attr.Get(lastTimeSample + 1), 
                             attr.Get(lastTimeSample))

        # Verify that getting the complete time sample map for this
        # attribute is equivalent to asking for the value at each time
        # returned by GetTimeSamples()
        def _GetValue(attr, t):
            v = attr.Get(t)
            if v == None:
                return Sdf.ValueBlock()
            return v

        timeSampleMap = dict([(t, _GetValue(attr, t)) for t in allTimeSamples])

        self.assertEqual(timeSampleMap, attr.GetMetadata('timeSamples'))

        # Verify that getting ranges of time samples works
        if len(allTimeSamples) > 2:
            startClip = min(allTimeSamples) 
            endClip = startClip

            self.assertEqual(
                attr.GetTimeSamplesInInterval(
                    Gf.Interval(startClip - 1, endClip, True, False)),
                [])

            while endClip < max(allTimeSamples):
                self.assertEqual(
                    attr.GetTimeSamplesInInterval(
                        Gf.Interval(startClip, endClip)), 
                    [t for t in allTimeSamples if t <= endClip])
                endClip += 1

            self.assertEqual(
                attr.GetTimeSamplesInInterval(
                    Gf.Interval(endClip, endClip + 1, False, True)),
                [])

    def CheckSpline(self, attr, extrap=True):
        """
        Verifies attribute spline is as expected via the spline API. If extrap is
        True (the default) then it checks the pre- and post extrapolation
        regions.
        """
        # XXX: The extrap flag is a workaround for a bug where the first and last
        # knots on the spline have dual valued knots. The clip evaluation clamps
        # values outside the clip time range to the edges of the range, but the
        # dual valued knots give the spline different values in the extrapolation
        # region than the ones computed for normal evaluation.
        self.assertTrue(attr.HasSpline())
        spline = attr.GetSpline()

        stage = attr.GetStage()
        held = (stage.GetInterpolationType() == Usd.InterpolationTypeHeld)

        # All splines "may be time varying"
        self.assertTrue(attr.ValueMightBeTimeVarying())

        # time samples API should return empty objects
        self.assertEqual(len(attr.GetTimeSamples()), 0)
        self.assertEqual(attr.GetNumTimeSamples(), 0)
        self.assertEqual(attr.GetBracketingTimeSamples(0), ())
        self.assertEqual(len(attr.GetTimeSamples()), 0)

        # Check that evaluating the spline at time t is equivalent to
        # UsdAttribute::Get(t). Note that `spline` is expected to have
        # layer offsets baked in.
        span = spline.GetKnotsWithInnerLoopsBaked().GetTimeSpan()
        if extrap:
            begin = int(span.min - 1)
            end = int(span.max + 2)
        else:
            begin = int(math.floor(span.min + 1))
            end = int(math.ceil(span.max))
        for t in range(begin, end):
            evalResult = (spline.EvalHeld(t) if held else
                          spline.Eval(t))
            getResult = attr.Get(t)
            if evalResult is None or getResult is None:
                self.assertEqual(evalResult, getResult)
            else:
                self.assertAlmostEqual(float(evalResult), float(getResult))

            evalPreResult = (spline.EvalPreValueHeld(t) if held else
                             spline.EvalPreValue(t))
            getPreResult = attr.Get(Usd.TimeCode.PreTime(t))
            if evalPreResult is None or getPreResult is None:
                self.assertEqual(evalPreResult, getPreResult)
            else:
                self.assertAlmostEqual(float(evalPreResult),
                                       float(getPreResult))

    def CheckValue(self, attr, expected, time=None, query=True):
        if time is not None:
            self.assertEqual(attr.Get(time), expected)
            if query:
                self.assertEqual(Usd.AttributeQuery(attr).Get(time), expected)
        else:
            self.assertEqual(attr.Get(), expected)
            if query:
                self.assertEqual(Usd.AttributeQuery(attr).Get(), expected)

    def CheckValueClose(self, attr, expected, time=None, query=True):
        if time is not None:
            self.assertAlmostEqual(attr.Get(time), expected)
            if query:
                self.assertAlmostEqual(Usd.AttributeQuery(attr).Get(time),
                                       expected)
        else:
            self.assertAlmostEqual(attr.Get(), expected)
            if query:
                self.assertAlmostEqual(Usd.AttributeQuery(attr).Get(),
                                       expected)

    def test_ArrayEdits(self):
        """Checks that array edits in defaults, samples, and clips compose."""
        stage = Usd.Stage.Open('arrayEdits/root.usda')
        root = stage.GetPrimAtPath('/Root')
        attr = root.GetAttribute('attr')
        self.assertTrue(attr)

        self.assertTrue(attr.ValueMightBeTimeVarying())

        # At default time we see the weakest default plus the actions of the
        # append & prepend.
        self.CheckValue(attr, expected=[3, 3, 3, -999])

        # At sample times we expect to see the values as edited by the strongest
        # default, the next weaker samples, and the next weaker two clips.
        self.CheckValue(attr, time=1, expected=[8, -1, -1, 555, -999])
        self.CheckValue(attr, time=2, expected=[-1, 9, -1, 555, -999])
        self.CheckValue(attr, time=3, expected=[10, -7, 11, 555, -999])
        self.CheckValue(attr, time=4, expected=[10, -7, 11, 666, -999])

        self.CheckTimeSamples(attr)

    def test_BasicClipBehavior(self):
        """Exercises basic clip behavior."""
        stage = Usd.Stage.Open('basic/root.usda')

        model = stage.GetPrimAtPath('/Model_1')

        localAttr = model.GetAttribute('local')
        refAttr = model.GetAttribute('ref')
        clsAttr = model.GetAttribute('cls')
        payloadAttr = model.GetAttribute('payload')
        varAttr = model.GetAttribute('var')
        self.assertTrue(localAttr)
        self.assertTrue(clsAttr)
        self.assertTrue(refAttr)
        self.assertTrue(payloadAttr)
        self.assertTrue(varAttr)

        # No clip layers should be loaded yet
        self.assertEqual(stage.GetUsedLayers(includeClipLayers=True), 
                         stage.GetUsedLayers(includeClipLayers=False))
        self.assertFalse(Sdf.Layer.Find('basic/clip.usda'))
        self.assertFalse(Sdf.Layer.Find('basic/manifest.usda'))

        # Clips are never consulted for default values.  This implies also
        # that no clips should even get loaded as a result of the queries.
        # However, we must tell CheckValue() not to construct UsdAttributeQuery
        # objects, since that act *does* need to load clips if the attr is 
        # affected by clips
        self.CheckValue(localAttr, expected=1.0, query=False)
        self.CheckValue(refAttr, expected=1.0, query=False)
        self.CheckValue(clsAttr, expected=1.0, query=False)
        self.CheckValue(payloadAttr, expected=1.0, query=False)
        self.CheckValue(varAttr, expected=1.0, query=False)
        
        # Still shouldn't have loaded any clip layers! 
        self.assertEqual(stage.GetUsedLayers(includeClipLayers=True), 
                         stage.GetUsedLayers(includeClipLayers=False))
        self.assertFalse(Sdf.Layer.Find('basic/clip.usda'))
        self.assertFalse(Sdf.Layer.Find('basic/manifest.usda'))

        # These attributes all have multiple time samples either locally
        # or from the single clip, so they all might be time varying.
        self.assertTrue(localAttr.ValueMightBeTimeVarying())
        self.assertTrue(clsAttr.ValueMightBeTimeVarying())
        self.assertTrue(refAttr.ValueMightBeTimeVarying())
        self.assertTrue(payloadAttr.ValueMightBeTimeVarying())
        self.assertTrue(varAttr.ValueMightBeTimeVarying())

        # Model_1 has active clips authored starting at time 10. However, the
        # first active clip is "held active" to -inf, and for any given time t,
        # the prior active clip at time t is still considered active.  So even
        # when querying a timeSample prior to the first "active time", we expect
        # the first clip to be loaded and consulted, with a linear time-mapping
        # from stage time to time-within-clip-earlier-than-first-clipTimes-knot.
        # In our test case, this means all attrs except localAttr (which has
        # local timeSamples in the clip-anchoring layer) should get their values
        # from the first clip.
        self.CheckValue(localAttr, time=5, expected=5.0)
        self.CheckValue(refAttr, time=5, expected=-5.0)
        self.CheckValue(clsAttr, time=5, expected=-5.0)
        self.CheckValue(payloadAttr, time=5, expected=-5.0)
        self.CheckValue(varAttr, time=5, expected=-5.0)

        # We expect the manifest and the first clip to be opened at this point.
        self.assertTrue(Sdf.Layer.Find('basic/clip.usda'))
        self.assertTrue(Sdf.Layer.Find('basic/manifest.usda'))

        # Check we get the same values from the first clip for edge conditions.
        minDouble = -sys.float_info.max
        self.CheckValue(localAttr, time=minDouble, expected=5.0)
        self.CheckValue(refAttr, time=minDouble, expected=-5.0)
        self.CheckValue(clsAttr, time=minDouble, expected=-5.0)
        self.CheckValue(payloadAttr, time=minDouble, expected=-5.0)
        self.CheckValue(varAttr, time=minDouble, expected=-5.0)
        
        negInf = float('-inf')
        self.CheckValue(localAttr, time=negInf, expected=5.0)
        self.CheckValue(refAttr, time=negInf, expected=-5.0)
        self.CheckValue(clsAttr, time=negInf, expected=-5.0)
        self.CheckValue(payloadAttr, time=negInf, expected=-5.0)
        self.CheckValue(varAttr, time=negInf, expected=-5.0)

        # Starting at time 10, clips should be consulted for values.
        #
        # The strength order using during time sample resolution is
        # L1(ocal)C(lip)L2(ocal)I(nherit)V(ariant)R(eference)P(ayload), so local
        # opinions in layers stronger than the layer that anchors the clip
        # metadata (L1 above, which *includes* the anchoring subLayer) should
        # win over the clip, but the clip should win over all other opinions,
        # including those from loal subLayers weaker than the anchoring layer
        # (L2).
        self.CheckValue(localAttr, time=10, expected=10.0)
        self.CheckValue(refAttr, time=10, expected=-10.0)
        self.CheckValue(clsAttr, time=10, expected=-10.0)
        self.CheckValue(payloadAttr, time=10, expected=-10.0)
        self.CheckValue(varAttr, time=10, expected=-10.0)

        # The last active clip is considered active to +inf. Test edge
        # conditions.
        maxDouble = sys.float_info.max
        self.CheckValue(localAttr, time=maxDouble, expected=20.0)
        self.CheckValue(refAttr, time=maxDouble, expected=-20.0)
        self.CheckValue(clsAttr, time=maxDouble, expected=-20.0)
        self.CheckValue(payloadAttr, time=maxDouble, expected=-20.0)
        self.CheckValue(varAttr, time=maxDouble, expected=-20.0)

        posInf = float('inf')
        self.CheckValue(localAttr, time=posInf, expected=20.0)
        self.CheckValue(refAttr, time=posInf, expected=-20.0)
        self.CheckValue(clsAttr, time=posInf, expected=-20.0)
        self.CheckValue(payloadAttr, time=posInf, expected=-20.0)
        self.CheckValue(varAttr, time=posInf, expected=-20.0)

        # Attributes in prims that are descended from where the clip
        # metadata was authored should pick up opinions from the clip
        # too, just like above.
        child = stage.GetPrimAtPath('/Model_1/Child')
        childAttr = child.GetAttribute('attr')

        self.CheckValue(childAttr, expected=1.0)
        self.CheckValue(childAttr, time=5, expected=-5.0)
        self.CheckValue(childAttr, time=10, expected=-10.0)

        self.CheckTimeSamples(localAttr)
        self.CheckTimeSamples(refAttr)
        self.CheckTimeSamples(clsAttr)
        self.CheckTimeSamples(payloadAttr)
        self.CheckTimeSamples(varAttr)
        self.CheckTimeSamples(childAttr)

        # Before reload, stage should still be getting the old value
        clipAttr = stage.GetPrimAtPath('/Model_1/Child').GetAttribute('attr')
        self.CheckValue(clipAttr, expected=-5, time=5)

        # Ensure that UsdStage::Reload reloads clip layers
        # by editing one of the clip layers values.
        try:
            # Make a copy of the original layer and restore it
            # afterwards so we don't leave unwanted state behind
            # and cause subsequent test runs to fail.
            shutil.copy2('basic/clip.usda', 'basic/clip.usda.old')

            clip = Sdf.Layer.FindOrOpen('basic/clip.usda')
            clip.SetTimeSample(Sdf.Path('/Model/Child.attr'), 5, 1005)
            clip.Save()

            # After, it should get the newly set value in our clip layer
            stage.Reload()
            self.CheckValue(clipAttr, expected=1005, time=5)
        finally:
            shutil.move('basic/clip.usda.old', 'basic/clip.usda')

    def test_ClipTiming(self):
        """Exercises clip retiming via clipTimes metadata"""
        stage = Usd.Stage.Open('timing/root.usda')
        
        model = stage.GetPrimAtPath('/Model')
        attr = model.GetAttribute('size')

        # Default value should come through regardless of clip timing.
        self.CheckValue(attr, expected=1.0)

        # The 'clipTimes' metadata authored in the test asset offsets the 
        # time samples in the clip by 10 frames and scales it slower by 50%,
        # repeating at frame 21.
        with InterpolationType(stage, Usd.InterpolationTypeHeld):
            self.CheckValue(attr, time=0, expected=10.0)
            self.CheckValue(attr, time=5, expected=10.0)
            self.CheckValue(attr, time=10, expected=15.0)
            self.CheckValue(attr, time=15, expected=15.0)
            self.CheckValue(attr, time=20, expected=10.0)
            self.CheckValue(attr, time=25, expected=10.0)
            self.CheckValue(attr, time=30, expected=15.0)
            self.CheckValue(attr, time=35, expected=15.0)
            self.CheckValue(attr, time=40, expected=20.0)

            # Requests for samples before and after the mapping specified in
            # 'clipTimes' just pick up the first or last time sample.
            self.CheckValue(attr, time=-1, expected=10.0)
            self.CheckValue(attr, time=41, expected=20.0)

        # Repeat the test with linear interpolation
        with InterpolationType(stage, Usd.InterpolationTypeLinear):
            self.CheckValue(attr, time=0, expected=10.0)
            self.CheckValue(attr, time=5, expected=12.5)
            self.CheckValue(attr, time=10, expected=15.0)
            self.CheckValue(attr, time=15, expected=17.5)
            self.CheckValue(attr, time=20, expected=10.0)
            self.CheckValue(attr, time=25, expected=12.5)
            self.CheckValue(attr, time=30, expected=15.0)
            self.CheckValue(attr, time=35, expected=17.5)
            self.CheckValue(attr, time=40, expected=20.0)

            self.CheckValue(attr, time=-1, expected=10.0)
            self.CheckValue(attr, time=41, expected=20.0)

        # The clip has time samples authored every 5 frames, but
        # since we've scaled everything by 50%, we should have samples
        # every 10 frames.
        self.assertEqual(
            attr.GetTimeSamples(), 
            [-10, 0, 10, 20 - Usd.TimeCode.SafeStep(), 20, 30, 40])
        self.assertEqual(
            attr.GetTimeSamplesInInterval(Gf.Interval(0, 30)),
            [0, 10, 20 - Usd.TimeCode.SafeStep(), 20, 30])

        # Test trickier cases where time samples in the clip fall outside
        # of the time domain specified by the 'clipTimes' metadata.
        model2 = stage.GetPrimAtPath('/Model2')
        attr2 = model2.GetAttribute('size')

        self.CheckValue(attr2, time=20, expected=20.0)
        self.CheckValue(attr2, time=30, expected=25.0)

        # Repeat the test with held interpolation
        with InterpolationType(stage, Usd.InterpolationTypeHeld):
            self.CheckValue(attr2, time=20, expected=15.0)
            self.CheckValue(attr2, time=30, expected=25.0)

        self.assertEqual(attr2.GetTimeSamples(),
            [0.0, 10.0, 20.0, 25.0, 30.0])
        self.assertEqual(attr2.GetTimeSamplesInInterval(Gf.Interval(0, 25)), 
            [0.0, 10.0, 20.0, 25.0])

        self.CheckTimeSamples(attr)
        self.CheckTimeSamples(attr2)

    def test_ClipTimeSamples(self):
        """Test that each stage time in a clips time mapping is treated as
        a time sample."""
        stage = Usd.Stage.Open('timeSamples/root.usda')

        model = stage.GetPrimAtPath('/Model')
        attr = model.GetAttribute('size')

        self.assertEqual(
            attr.GetTimeSamples(),
            [0.0, 2.0, 4.0, 5.0 - Usd.TimeCode.SafeStep(), 5.0, 6.0, 7.0, 8.0, 
             9.0])
        self.CheckTimeSamples(attr)

    def test_ClipTimingOutsideRange(self):
        """Tests clip retiming behavior when the mapped clip times are outside
        the range of time samples in the clip"""
        stage = Usd.Stage.Open('timingOutsideClip/root.usda')

        model = stage.GetPrimAtPath('/Model')
        attr = model.GetAttribute('size')

        # Asking for frames outside the mapped times should also clamp to
        # the nearest time sample.
        for t in range(-10, 0):
            self.CheckValue(attr, time=t, expected=25.0)
            self.assertEqual(attr.GetBracketingTimeSamples(t), (0.0, 0.0))

        for t in range(11, 20):
            self.CheckValue(attr, time=t, expected=25.0)
            self.assertEqual(attr.GetBracketingTimeSamples(t), (10.0, 10.0))

        self.assertEqual(attr.GetTimeSamples(), 
            [0.0, 10.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(-1.0, 1.0)), 
            [0.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0.0, 0.0)), 
            [0.0])
        self.CheckTimeSamples(attr)

    def test_ClipTimeCodeTiming(self):
        """Exercises clip retiming via clipTimes metadata for timecode and
        duration value attributes, authored as time samples, splines, and
        arrays.

        Time-based values read from a clip are converted from the clip's
        internal time to external (stage) time using the bracketing clipTimes
        segment. A GfTimeCode is a point on the time axis, so the segment's
        full linear map (scale and offset) is applied, exactly as an internal
        time would be translated to external time. A GfDuration is a length on
        the time axis, so only the segment's scale is applied; a constant offset
        would cancel out of a duration and must not be added.

        With the clipTimes authored here the first segment maps external
        [0, 20] to internal [10, 20] (external advances twice as fast as
        internal, so scale = 2), and after a jump discontinuity the second
        segment maps external [20, 40] to internal [0, 20] (scale = 1). This is
        why, e.g., the same clip value resolves to different external values in
        the two segments, and why 'dur' (scaled only) diverges from 'time'
        (scaled and offset)."""
        stage = Usd.Stage.Open('timeCodeTiming/root.usda')

        model = stage.GetPrimAtPath('/Model')
        attr = model.GetAttribute('time')
        attr2 = model.GetAttribute('timeArray')
        timeSpline = model.GetAttribute('timeSpline')
        dur = model.GetAttribute('dur')
        durSpline = model.GetAttribute('durSpline')
        durArray = model.GetAttribute('durArray')

        # Default value should come through regardless of clip timing.
        self.CheckValue(attr, expected=1.0)
        self.CheckValue(attr2, expected=Vt.TimeCodeArray([1.0,2.0]))
        self.CheckValue(timeSpline, expected=1.0)
        self.CheckValue(dur, expected=Gf.Duration(1.0))
        self.CheckValue(durSpline, expected=Gf.Duration(1.0))
        self.CheckValue(durArray, expected=Vt.DurationArray([1.0, 2.0]))

        self.assertTrue(timeSpline.HasSpline())
        self.assertTrue(durSpline.HasSpline())

        # 'timeSpline' is a timecode spline whose value equals the internal
        # time; it retimes identically to the 'time' time samples below (scale
        # and offset). Outside the clip's mapped range the query external time
        # is clamped to the range boundary, so the value is held (10.0 / 25.0)
        # rather than extrapolated along the spline.
        #
        # The value after the jump discontinuity below is tricky to follow. The
        # time values come through a value clip with two time regions. The first
        # is [(0, 10), (20, 20)]. Which maps stage times of 0 and 20 to clip
        # times of 10 and 20. This is equivalent to a layer offset of scale = 2
        # and offset = -20. (Remember, layer offsets map from clip times toward
        # stage times while the "times" array in clips defines the inverse
        # operation.) A scale of 2 and offset of -20 maps clipTime 10 to
        # stageTime 0 and clipTime 20 to stageTime 20. But the second time
        # region maps [(20, 0), (40, 20)] which is linear, but with an offset.
        # As a layer offset, it would be scale = 1 and offset = +20
        #
        # So:
        #   stageTime 0 -> clipTime 10 -> clipValue 15 -> stageValue 10
        #   stageTime 5 -> clipTime 12.5 -> clipValue 12.5 -> stageValue 5
        #   stageTime pre 20 -> clipTime pre 20 -> clipValue 5 -> stageValue -10
        # <jump discontinuity>, switch to the second time region and layer offset
        #   stageTime 20 -> clipTime 0 -> clipValue 25 -> stageValue 45
        #   ...
        #
        # XXX: See the comment at the top of CheckSpline for an explanation
        # of the extrap argument and why we're setting it to false.
        self.CheckSpline(timeSpline, extrap=False)

        # 'durSpline' is a duration spline with the same authored values, but
        # durations are only scaled (never offset), so it diverges from
        # 'timeSpline': the left segment scales by 2, the right segment by 1.
        self.CheckSpline(durSpline, extrap=False)

        stage.SetInterpolationType(Usd.InterpolationTypeLinear)

        # The 'clipTimes' metadata authored in the test asset maps external
        # [0, 20] to internal [10, 20] (external advances twice as fast, so
        # timecode values scale by 2), then after a jump discontinuity maps
        # external [20, 40] to internal [0, 20] (scale 1).
        self.CheckValue(attr, time=0, expected=10.0)
        self.CheckValue(attr, time=5, expected=5.0)
        self.CheckValue(attr, time=10, expected=0.0)
        self.CheckValue(attr, time=15, expected=-5.0)
        # @20 we should get the jump discontinuity time sample as the pre-time
        # (left segment, scale 2).
        self.CheckValue(attr, time=Usd.TimeCode.PreTime(20), expected=-10.0)
        self.CheckValue(attr, time=20, expected=45.0)
        self.CheckValue(attr, time=25, expected=40.0)
        self.CheckValue(attr, time=30, expected=35.0)
        self.CheckValue(attr, time=35, expected=30.0)
        self.CheckValue(attr, time=40, expected=25.0)

        # Requests for samples before and after the mapping specified in
        # 'clipTimes' hold the first or last time sample (scaled by the nearest
        # real segment).
        self.CheckValue(attr, time=-1, expected=10.0)
        self.CheckValue(attr, time=41, expected=25.0)

        # Repeat getting values at the same times for the Vt.TimeCodeArray
        # valued attribute.
        self.CheckValue(attr2, time=0, expected=Vt.TimeCodeArray([0.0, 10.0]))
        self.CheckValue(attr2, time=5, expected=Vt.TimeCodeArray([5.0, 5.0]))
        self.CheckValue(attr2, time=10, expected=Vt.TimeCodeArray([10.0, 0.0]))
        self.CheckValue(attr2, time=15, expected=Vt.TimeCodeArray([15.0, -5.0]))
        # @20 we should get the jump discontinuity time sample as the pre-time
        # (left segment, scale 2).
        self.CheckValue(attr2, time=Usd.TimeCode.PreTime(20),
                        expected=Vt.TimeCodeArray([20.0, -10.0]))
        self.CheckValue(attr2, time=20, expected=Vt.TimeCodeArray([20.0, 45.0]))
        self.CheckValue(attr2, time=25, expected=Vt.TimeCodeArray([25.0, 40.0]))
        self.CheckValue(attr2, time=30, expected=Vt.TimeCodeArray([30.0, 35.0]))
        self.CheckValue(attr2, time=35, expected=Vt.TimeCodeArray([35.0, 30.0]))
        self.CheckValue(attr2, time=40, expected=Vt.TimeCodeArray([40.0, 25.0]))

        self.CheckValue(attr2, time=-1, expected=Vt.TimeCodeArray([0.0, 10.0]))
        self.CheckValue(attr2, time=41, expected=Vt.TimeCodeArray([40.0, 25.0]))

        # 'dur' is a duration authored with the same clip time samples as
        # 'time', but durations are only scaled (never offset): each value is
        # multiplied by the bracketing segment's scale (2 in the first segment,
        # 1 in the second).
        self.CheckValue(dur, time=0, expected=Gf.Duration(30.0))
        self.CheckValue(dur, time=5, expected=Gf.Duration(25.0))
        self.CheckValue(dur, time=10, expected=Gf.Duration(20.0))
        self.CheckValue(dur, time=15, expected=Gf.Duration(15.0))
        self.CheckValue(dur, time=Usd.TimeCode.PreTime(20),
                        expected=Gf.Duration(10.0))
        self.CheckValue(dur, time=20, expected=Gf.Duration(25.0))
        self.CheckValue(dur, time=25, expected=Gf.Duration(20.0))
        self.CheckValue(dur, time=30, expected=Gf.Duration(15.0))
        self.CheckValue(dur, time=35, expected=Gf.Duration(10.0))
        self.CheckValue(dur, time=40, expected=Gf.Duration(5.0))
        self.CheckValue(dur, time=-1, expected=Gf.Duration(30.0))
        self.CheckValue(dur, time=41, expected=Gf.Duration(5.0))

        # 'durArray' mirrors 'timeArray' but scaled only (no offset).
        self.CheckValue(durArray, time=0,
                        expected=Vt.DurationArray([20.0, 30.0]))
        self.CheckValue(durArray, time=5,
                        expected=Vt.DurationArray([25.0, 25.0]))
        self.CheckValue(durArray, time=10,
                        expected=Vt.DurationArray([30.0, 20.0]))
        self.CheckValue(durArray, time=15,
                        expected=Vt.DurationArray([35.0, 15.0]))
        self.CheckValue(durArray, time=Usd.TimeCode.PreTime(20),
                        expected=Vt.DurationArray([40.0, 10.0]))
        self.CheckValue(durArray, time=20,
                        expected=Vt.DurationArray([0.0, 25.0]))
        self.CheckValue(durArray, time=25,
                        expected=Vt.DurationArray([5.0, 20.0]))
        self.CheckValue(durArray, time=30,
                        expected=Vt.DurationArray([10.0, 15.0]))
        self.CheckValue(durArray, time=35,
                        expected=Vt.DurationArray([15.0, 10.0]))
        self.CheckValue(durArray, time=40,
                        expected=Vt.DurationArray([20.0, 5.0]))
        self.CheckValue(durArray, time=-1,
                        expected=Vt.DurationArray([20.0, 30.0]))
        self.CheckValue(durArray, time=41,
                        expected=Vt.DurationArray([20.0, 5.0]))

        # Repeat the test over again with held interpolation.
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        # The same retiming as above, now with held interpolation of the
        # clip's time samples.
        self.CheckValue(attr, time=0, expected=10.0)
        self.CheckValue(attr, time=5, expected=10.0)
        self.CheckValue(attr, time=10, expected=0.0)
        self.CheckValue(attr, time=15, expected=0.0)
        # @20 we should get the jump discontinuity time sample as the pre-time
        # (left segment, scale 2).
        self.CheckValue(attr, time=Usd.TimeCode.PreTime(20),
                        expected=-10.0)
        self.CheckValue(attr, time=20, expected=45.0)
        self.CheckValue(attr, time=25, expected=40.0)
        self.CheckValue(attr, time=30, expected=35.0)
        self.CheckValue(attr, time=35, expected=30.0)
        self.CheckValue(attr, time=40, expected=25.0)

        # Requests for samples before and after the mapping specified in
        # 'clipTimes' hold the first or last time sample (scaled by the nearest
        # real segment).
        self.CheckValue(attr, time=-1, expected=10.0)
        self.CheckValue(attr, time=41, expected=25.0)

        # Repeat getting values at the same times for the Vt.TimeCodeArray
        # valued attribute.
        self.CheckValue(attr2, time=0, expected=Vt.TimeCodeArray([0.0, 10.0]))
        self.CheckValue(attr2, time=5, expected=Vt.TimeCodeArray([0.0, 10.0]))
        self.CheckValue(attr2, time=10, expected=Vt.TimeCodeArray([10.0, 0.0]))
        self.CheckValue(attr2, time=15, expected=Vt.TimeCodeArray([10.0, 0.0]))
        # @20 we should get the jump discontinuity time sample as the pre-time
        # (left segment, scale 2).
        self.CheckValue(attr2, time=Usd.TimeCode.PreTime(20),
                        expected=Vt.TimeCodeArray([20.0, -10.0]))
        self.CheckValue(attr2, time=20,
                        expected=Vt.TimeCodeArray([20.0, 45.0]))
        self.CheckValue(attr2, time=25, expected=Vt.TimeCodeArray([25.0, 40.0]))
        self.CheckValue(attr2, time=30, expected=Vt.TimeCodeArray([30.0, 35.0]))
        self.CheckValue(attr2, time=35, expected=Vt.TimeCodeArray([35.0, 30.0]))
        self.CheckValue(attr2, time=40, expected=Vt.TimeCodeArray([40.0, 25.0]))

        self.CheckValue(attr2, time=-1, expected=Vt.TimeCodeArray([0.0, 10.0]))
        self.CheckValue(attr2, time=41, expected=Vt.TimeCodeArray([40.0, 25.0]))

        # 'dur' held (scaled only, no offset).
        self.CheckValue(dur, time=0, expected=Gf.Duration(30.0))
        self.CheckValue(dur, time=5, expected=Gf.Duration(30.0))
        self.CheckValue(dur, time=10, expected=Gf.Duration(20.0))
        self.CheckValue(dur, time=15, expected=Gf.Duration(20.0))
        self.CheckValue(dur, time=Usd.TimeCode.PreTime(20),
                        expected=Gf.Duration(10.0))
        self.CheckValue(dur, time=20, expected=Gf.Duration(25.0))
        self.CheckValue(dur, time=25, expected=Gf.Duration(20.0))
        self.CheckValue(dur, time=30, expected=Gf.Duration(15.0))
        self.CheckValue(dur, time=35, expected=Gf.Duration(10.0))
        self.CheckValue(dur, time=40, expected=Gf.Duration(5.0))
        self.CheckValue(dur, time=-1, expected=Gf.Duration(30.0))
        self.CheckValue(dur, time=41, expected=Gf.Duration(5.0))

        # 'durArray' held (scaled only, no offset).
        self.CheckValue(durArray, time=0,
                        expected=Vt.DurationArray([20.0, 30.0]))
        self.CheckValue(durArray, time=5,
                        expected=Vt.DurationArray([20.0, 30.0]))
        self.CheckValue(durArray, time=10,
                        expected=Vt.DurationArray([30.0, 20.0]))
        self.CheckValue(durArray, time=15,
                        expected=Vt.DurationArray([30.0, 20.0]))
        self.CheckValue(durArray, time=Usd.TimeCode.PreTime(20),
                        expected=Vt.DurationArray([40.0, 10.0]))
        self.CheckValue(durArray, time=20,
                        expected=Vt.DurationArray([0.0, 25.0]))
        self.CheckValue(durArray, time=25,
                        expected=Vt.DurationArray([5.0, 20.0]))
        self.CheckValue(durArray, time=30,
                        expected=Vt.DurationArray([10.0, 15.0]))
        self.CheckValue(durArray, time=35,
                        expected=Vt.DurationArray([15.0, 10.0]))
        self.CheckValue(durArray, time=40,
                        expected=Vt.DurationArray([20.0, 5.0]))
        self.CheckValue(durArray, time=-1,
                        expected=Vt.DurationArray([20.0, 30.0]))
        self.CheckValue(durArray, time=41,
                        expected=Vt.DurationArray([20.0, 5.0]))

        # The clip has time samples authored every 5 frames, but
        # since we've scaled everything by 50%, we should have samples
        # every 10 frames.
        self.assertEqual(
            attr.GetTimeSamples(),
            [0, 10, 20 - Usd.TimeCode.SafeStep(), 20, 25, 30, 35, 40])
        self.assertEqual(
            attr.GetTimeSamplesInInterval(Gf.Interval(0, 30)),
            [0, 10, 20 - Usd.TimeCode.SafeStep(), 20, 25, 30])

        # 'dur' shares the same retimed sample times as 'time'.
        self.assertEqual(
            dur.GetTimeSamples(),
            [0, 10, 20 - Usd.TimeCode.SafeStep(), 20, 25, 30, 35, 40])
        self.assertEqual(
            dur.GetTimeSamplesInInterval(Gf.Interval(0, 30)),
            [0, 10, 20 - Usd.TimeCode.SafeStep(), 20, 25, 30])

        self.CheckTimeSamples(attr)
        self.CheckTimeSamples(attr2)
        self.CheckTimeSamples(dur)
        self.CheckTimeSamples(durArray)

    def test_ClipsWithLayerOffsets(self):
        """Tests behavior of clips when layer offsets are involved"""
        stage = Usd.Stage.Open('layerOffsets/root.usda')

        model1 = stage.GetPrimAtPath('/Model_1')
        attr1 = model1.GetAttribute('size')
        model2 = stage.GetPrimAtPath('/Model_2')
        attr2 = model2.GetAttribute('size')
        model3 = stage.GetPrimAtPath('/Model_3')
        attr3 = model3.GetAttribute('size')
        model4 = stage.GetPrimAtPath('/Model_4')
        attr4 = model4.GetAttribute('size')
        model5 = stage.GetPrimAtPath('/Model_5')
        attr5 = model5.GetAttribute('size')
        model6 = stage.GetPrimAtPath('/Model_6')
        attr6 = model6.GetAttribute('size')
        model7 = stage.GetPrimAtPath('/Model_7')
        attr7 = model7.GetAttribute('size')
        model8 = stage.GetPrimAtPath('/Model_8')
        attr8 = model8.GetAttribute('size')

        # Default value should be unaffected by layer offsets.
        self.CheckValue(attr1, expected=1.0)

        # The clip should be active starting from frame +10.0 due to the
        # offset; before that, we get the held value of the clip's first 
        # time sample,
        self.CheckValue(attr1, time=9, expected=-5.0)

        # Sublayer offset of 10 frames is present, so attribute value at
        # frame 20 should be from the clip at frame 10, etc.
        self.CheckValue(attr1, time=20, expected=-10.0)
        self.CheckValue(attr1, time=15, expected=-5.0)
        self.CheckValue(attr1, time=10, expected=-5.0)
        self.assertEqual(attr1.GetTimeSamples(), 
           [10.0, 15.0, 20.0, 25.0, 30.0])
        self.assertEqual(attr1.GetTimeSamplesInInterval(
            Gf.Interval(-10, 10)), [10.0])
 
        # Test that layer offsets on layers where
        # clipTimes/clipActive are authored are taken into
        # account. The test case is similar to above, except
        # clipTimes/clipActive have been authored in a sublayer that
        # is offset by 20 frames instead of 10.
        self.CheckValue(attr2, expected=1.0)
        self.CheckValue(attr2, time=19, expected=-5.0)
        self.CheckValue(attr2, time=40, expected=-20.0)
        self.CheckValue(attr2, time=35, expected=-15.0)
        self.CheckValue(attr2, time=30, expected=-10.0)
        self.assertEqual(attr2.GetTimeSamples(), 
            [20.0, 25.0, 30.0, 35.0, 40.0])
        self.assertEqual(attr2.GetTimeSamplesInInterval(
            Gf.Interval(-17, 21)), 
            [20.0])

        # Test that reference offsets are taken into account. An offset
        # of 10 frames is authored on the reference; this should be combined
        # with the offset of 10 frames on the sublayer.
        self.CheckValue(attr3, expected=1.0)
        self.CheckValue(attr3, time=19, expected=-5.0)
        self.CheckValue(attr3, time=40, expected=-20.0)
        self.CheckValue(attr3, time=35, expected=-15.0)
        self.CheckValue(attr3, time=30, expected=-10.0)
        self.assertEqual(attr3.GetTimeSamples(), 
            [20.0, 25.0, 30.0, 35.0, 40.0])
        self.assertEqual(attr3.GetTimeSamplesInInterval(
            Gf.Interval(-5, 5)),
            [])

        # Test that a reference offset with a non-unit scale is taken into
        # account. Model_4 references the same clip data as Model_3 but with a
        # scale of 2 on the reference (offset = 10; scale = 2), which composes
        # with the sublayer offset of 10 to a net offset of 20 and a scale of 2.
        # The clip's internal time t therefore maps to stage time 2*t + 20. The
        # 'size' value is a plain float, so only the sample lookup is retimed;
        # the returned values are not themselves transformed.
        self.CheckValue(attr4, expected=1.0)
        self.CheckValue(attr4, time=19, expected=-5.0)
        self.CheckValue(attr4, time=30, expected=-5.0)
        self.CheckValue(attr4, time=40, expected=-10.0)
        self.CheckValue(attr4, time=50, expected=-15.0)
        self.CheckValue(attr4, time=60, expected=-20.0)
        self.assertEqual(attr4.GetTimeSamples(),
            [20.0, 30.0, 40.0, 50.0, 60.0])
        self.assertEqual(attr4.GetTimeSamplesInInterval(
            Gf.Interval(-5, 25)),
            [20.0])

        self.CheckTimeSamples(attr1)
        self.CheckTimeSamples(attr2)
        self.CheckTimeSamples(attr3)
        self.CheckTimeSamples(attr4)

        # Verify GetPropertyStackWithLayerOffsets run on an attribute with
        # clips returns the clip spec's layer offset matching the source spec's
        # layer offset.
        self.assertEqual(attr3.GetPropertyStackWithLayerOffsets(40),
            [(Sdf.Find('layerOffsets/clip.usda', '/Model.size'), 
                Sdf.LayerOffset(20)), 
             (Sdf.Find('layerOffsets/ref.usda', '/Model.size'), 
                Sdf.LayerOffset(20))])

        # Test that layer offsets are taken into account when clip times is
        # absent. This should result in the same time samples as attr1
        self.CheckValue(attr5, time=0, expected=-5)
        self.CheckValue(attr5, time=9, expected=-5)
        self.CheckValue(attr5, time=10, expected=-5)
        self.CheckValue(attr5, time=15, expected=-5)
        self.CheckValue(attr5, time=20, expected=-10)
        self.CheckValue(attr5, time=25, expected=-15)
        self.CheckValue(attr5, time=30, expected=-20)
        self.CheckValue(attr5, time=50, expected=-20)
        self.assertEqual(attr1.GetTimeSamples(), attr5.GetTimeSamples())
        self.CheckTimeSamples(attr5)

        # Test that layer offsets combined with stretch/shrink regions in clip
        # times are taken into account. This example is sped up 2x by clip
        # times with a layer offset of 10.
        self.CheckValue(attr6, time=0, expected=-5)
        self.CheckValue(attr6, time=2.5, expected=-5)
        self.CheckValue(attr6, time=5, expected=-5)
        self.CheckValue(attr6, time=10, expected=-10)
        self.CheckValue(attr6, time=11, expected=-10)
        self.CheckTimeSamples(attr6)

        # Test that the layer offset for a clipSet is the offset from the
        # stage to the layer where the strongest authored 'active' metadata
        # is defined.
        self.CheckValue(attr7, time=0, expected=-5)
        self.CheckValue(attr7, time=20, expected=-5)
        self.CheckValue(attr7, time=25, expected=-5)
        self.CheckValue(attr7, time=30, expected=-10)
        self.CheckValue(attr7, time=35, expected=-15)
        self.CheckValue(attr7, time=40, expected=-20)
        self.CheckValue(attr7, time=45, expected=-20)
        self.CheckTimeSamples(attr7)

        # Test that clip times are offset independently of the layer offset
        # for a clipSet. The base offset is 20 and the clip times offset is
        # 10, so evaluation proceeds as if clip times is scaled by
        # 10 - 20 = -10. Note that evaluation itself proceeds by first
        # applying the base offset (20) to the queried time.
        self.CheckValue(attr8, time=0, expected=-5)
        self.CheckValue(attr8, time=10, expected=-5)
        self.CheckValue(attr8, time=15, expected=-5)
        self.CheckValue(attr8, time=20, expected=-10)
        self.CheckValue(attr8, time=25, expected=-15)
        self.CheckValue(attr8, time=30, expected=-20)
        self.CheckValue(attr8, time=50, expected=-20)
        self.CheckTimeSamples(attr8)

    def test_ClipsWithSplineWithLayerOffsets(self):
        """Tests behavior of splines in clips with layer offsets involvement"""
        stage = Usd.Stage.Open('layerOffsets/root.usda')
        model1 = stage.GetPrimAtPath('/Model_1')
        attr1 = model1.GetAttribute('attrSpline')
        self.CheckSpline(attr1)
        self.CheckValue(attr1, time=14, expected=3)
        self.CheckValue(attr1, time=15, expected=5)
        self.CheckValue(attr1, time=26, expected=17)
        self.CheckValue(attr1, time=30, expected=9)
        self.CheckValue(attr1, time=50, expected=9)

        model3 = stage.GetPrimAtPath('/Model_3')
        attr3 = model3.GetAttribute('attrSpline')
        self.CheckSpline(attr3)
        # Times are +10 of attr1 checks
        self.CheckValue(attr3, time=24, expected=3)
        self.CheckValue(attr3, time=25, expected=5)
        self.CheckValue(attr3, time=36, expected=17)
        self.CheckValue(attr3, time=40, expected=9)
        self.CheckValue(attr3, time=60, expected=9)

        # Model_4's net layer offset is (offset = 20, scale = 2), so the clip's
        # internal time t maps to stage time 2*t + 20. attr1's checks land at
        # internal times 4/5/16/20/40; for Model_4 those occur at 2*t + 20,
        # i.e. stage 28/30/52/60/100. The spline values are plain floats and so
        # are not transformed; only the spline's time axis is retimed.
        model4 = stage.GetPrimAtPath('/Model_4')
        attr4 = model4.GetAttribute('attrSpline')
        self.CheckSpline(attr4)
        self.CheckValue(attr4, time=28, expected=3)
        self.CheckValue(attr4, time=30, expected=5)
        self.CheckValue(attr4, time=52, expected=17)
        self.CheckValue(attr4, time=60, expected=9)
        self.CheckValue(attr4, time=100, expected=9)

    def test_TimeCodeClipsWithLayerOffsets(self):
        """Tests behavior of clips when layer offsets are involved and the
        attributes are GfTimeCode values. This test is almost identical to 
        test_ClipsWithLayerOffsets except that values returned themselves are
        also offset by the layer offsets."""
        stage = Usd.Stage.Open('layerOffsets/root.usda')

        model1 = stage.GetPrimAtPath('/Model_1')
        attr1 = model1.GetAttribute('time')
        model2 = stage.GetPrimAtPath('/Model_2')
        attr2 = model2.GetAttribute('time')
        model3 = stage.GetPrimAtPath('/Model_3')
        attr3 = model3.GetAttribute('time')
        model4 = stage.GetPrimAtPath('/Model_4')
        attr4 = model4.GetAttribute('time')

        # Default time code value will be affected by layer offsets.
        self.CheckValue(attr1, expected=11.0)

        # The first time sample from the clip should be active starting from 
        # frame +10.0 due to the offset; before that, we get the held value
        # of the clip's first time sample, which is itself then adjusted by
        # the offset.
        self.CheckValue(attr1, time=9, expected=5.0)

        # Sublayer offset of 10 frames is present, so attribute value at
        # frame 20 should be from the clip at frame 10, etc. plus the value of
        # the offset.
        self.CheckValue(attr1, time=20, expected=0.0)
        self.CheckValue(attr1, time=15, expected=5.0)
        self.CheckValue(attr1, time=10, expected=5.0)
        self.assertEqual(attr1.GetTimeSamples(), 
           [10.0, 15.0, 20.0, 25.0, 30.0])
        self.assertEqual(attr1.GetTimeSamplesInInterval(
            Gf.Interval(-10, 10)), [10.0])

        # Test that layer offsets on layers where
        # clipTimes/clipActive are authored are taken into
        # account. The test case is similar to above, except
        # clipTimes/clipActive have been authored in a sublayer that
        # is offset by 20 frames instead of 10.
        self.CheckValue(attr2, expected=11.0)
        self.CheckValue(attr2, time=19, expected=15.0)
        self.CheckValue(attr2, time=40, expected=0.0)
        self.CheckValue(attr2, time=35, expected=5.0)
        self.CheckValue(attr2, time=30, expected=10.0)
        self.assertEqual(attr2.GetTimeSamples(), 
            [20.0, 25.0, 30.0, 35.0, 40.0])
        self.assertEqual(attr2.GetTimeSamplesInInterval(
            Gf.Interval(-17, 21)), 
            [20.0])

        # Test that reference offsets are taken into account. An offset
        # of 10 frames is authored on the reference; this should be combined
        # with the offset of 10 frames on the sublayer.
        self.CheckValue(attr3, expected=21.0)
        self.CheckValue(attr3, time=19, expected=15.0)
        self.CheckValue(attr3, time=40, expected=0.0)
        self.CheckValue(attr3, time=35, expected=5.0)
        self.CheckValue(attr3, time=30, expected=10.0)
        self.assertEqual(attr3.GetTimeSamples(), 
            [20.0, 25.0, 30.0, 35.0, 40.0])
        self.assertEqual(attr3.GetTimeSamplesInInterval(
            Gf.Interval(-5, 5)),
            [])

        # This gets a little complicated so some explanation is in order:
        # Model_4 comes from the root_sublayer.usda with an offset of 10.  But
        # Model_4 in root_sublayer.usda comes from /Model in ref2.usda with
        # another offset of 10 and a scale of 2. /Model in ref2.usda comes from
        # /Model in ref.usda (with no scale or offset). The default value for
        # the time attribute is in ref.usda, but ref2.usda used a value clip
        # referencing clip.usda for the timeSamples. So the default is 1.0 but
        # the time samples are 5:-5, 10:-10, 15:-15, and 20:-20.
        #
        # Mapping this all back, the total time offset from clip.usda or
        # ref.usda to the stage is a scale=2 and offset=20.
        #
        # When querying the default value, 1.0 is found in ref.usda:
        #    stageValue = 1.0 * 2 + 20 = 22.0
        #
        # When querying time samples, they are found in clip.usda. Stage times
        # are mapped back through the offsets as:
        #    clipTime = (stageTime - 20) / 2
        # So:
        #    stageTime 30 -> clipTime 5 -> clipValue -5 -> stageValue 10
        #    stageTime 40 -> clipTime 10 -> clipValue -10 -> stageValue 0
        #    ...
        self.CheckValue(attr4, expected=22.0)
        self.CheckValue(attr4, time=19, expected=10.0)
        self.CheckValue(attr4, time=30, expected=10.0)
        self.CheckValue(attr4, time=40, expected=0.0)
        self.CheckValue(attr4, time=50, expected=-10.0)
        self.CheckValue(attr4, time=60, expected=-20.0)
        self.assertEqual(attr4.GetTimeSamples(),
            [20.0, 30.0, 40.0, 50.0, 60.0])
        self.assertEqual(attr4.GetTimeSamplesInInterval(
            Gf.Interval(-5, 25)),
            [20.0])

        self.CheckTimeSamples(attr1)
        self.CheckTimeSamples(attr2)
        self.CheckTimeSamples(attr3)
        self.CheckTimeSamples(attr4)

    def test_DurationClipsWithLayerOffsets(self):
        """Tests behavior of clips when layer offsets are involved and the
        attributes are GfDuration values. This is analogous to
        test_TimeCodeClipsWithLayerOffsets, except that a GfDuration is a length
        on the time axis rather than a point: only the scale of a layer offset
        is applied to a duration value, never the offset (a constant translation
        cancels out of a length). With the translation-only offsets on
        Model_1/2/3 (scale = 1), the returned duration values therefore equal
        the raw clip values, matching test_ClipsWithLayerOffsets; only Model_4,
        whose net offset has scale = 2, shows the values scaled."""
        stage = Usd.Stage.Open('layerOffsets/root.usda')

        model1 = stage.GetPrimAtPath('/Model_1')
        attr1 = model1.GetAttribute('dur')
        model2 = stage.GetPrimAtPath('/Model_2')
        attr2 = model2.GetAttribute('dur')
        model3 = stage.GetPrimAtPath('/Model_3')
        attr3 = model3.GetAttribute('dur')
        model4 = stage.GetPrimAtPath('/Model_4')
        attr4 = model4.GetAttribute('dur')

        # Default duration value is unaffected by the translation-only offset
        # (scale = 1, offset dropped).
        self.CheckValue(attr1, expected=Gf.Duration(1.0))

        # The clip is active starting from frame +10.0 due to the offset; before
        # that, we get the held value of the clip's first time sample. Unlike a
        # timecode, the duration value is not shifted by the offset.
        self.CheckValue(attr1, time=9, expected=Gf.Duration(-5.0))

        # Sublayer offset of 10 frames is present, so the attribute value at
        # frame 20 comes from the clip at frame 10, etc. The value itself is not
        # offset.
        self.CheckValue(attr1, time=20, expected=Gf.Duration(-10.0))
        self.CheckValue(attr1, time=15, expected=Gf.Duration(-5.0))
        self.CheckValue(attr1, time=10, expected=Gf.Duration(-5.0))
        self.assertEqual(attr1.GetTimeSamples(),
           [10.0, 15.0, 20.0, 25.0, 30.0])
        self.assertEqual(attr1.GetTimeSamplesInInterval(
            Gf.Interval(-10, 10)), [10.0])

        # As in test_ClipsWithLayerOffsets, Model_2 authors clipTimes/clipActive
        # in a sublayer offset by 20 frames instead of 10.
        self.CheckValue(attr2, expected=Gf.Duration(1.0))
        self.CheckValue(attr2, time=19, expected=Gf.Duration(-5.0))
        self.CheckValue(attr2, time=40, expected=Gf.Duration(-20.0))
        self.CheckValue(attr2, time=35, expected=Gf.Duration(-15.0))
        self.CheckValue(attr2, time=30, expected=Gf.Duration(-10.0))
        self.assertEqual(attr2.GetTimeSamples(),
            [20.0, 25.0, 30.0, 35.0, 40.0])
        self.assertEqual(attr2.GetTimeSamplesInInterval(
            Gf.Interval(-17, 21)),
            [20.0])

        # Model_3 combines a reference offset of 10 with the sublayer offset of
        # 10 for a net offset of 20 (scale = 1). The duration values are still
        # not shifted.
        self.CheckValue(attr3, expected=Gf.Duration(1.0))
        self.CheckValue(attr3, time=19, expected=Gf.Duration(-5.0))
        self.CheckValue(attr3, time=40, expected=Gf.Duration(-20.0))
        self.CheckValue(attr3, time=35, expected=Gf.Duration(-15.0))
        self.CheckValue(attr3, time=30, expected=Gf.Duration(-10.0))
        self.assertEqual(attr3.GetTimeSamples(),
            [20.0, 25.0, 30.0, 35.0, 40.0])
        self.assertEqual(attr3.GetTimeSamplesInInterval(
            Gf.Interval(-5, 5)),
            [])

        # Model_4's net layer offset is (offset = 20, scale = 2), so the clip's
        # internal time t maps to stage time 2*t + 20. A duration is scaled but
        # not offset, so the returned values are 2*clipValue (the +20 is
        # dropped), which is what distinguishes this from the timecode case.
        self.CheckValue(attr4, expected=Gf.Duration(2.0))
        self.CheckValue(attr4, time=19, expected=Gf.Duration(-10.0))
        self.CheckValue(attr4, time=30, expected=Gf.Duration(-10.0))
        self.CheckValue(attr4, time=40, expected=Gf.Duration(-20.0))
        self.CheckValue(attr4, time=50, expected=Gf.Duration(-30.0))
        self.CheckValue(attr4, time=60, expected=Gf.Duration(-40.0))
        self.assertEqual(attr4.GetTimeSamples(),
            [20.0, 30.0, 40.0, 50.0, 60.0])
        self.assertEqual(attr4.GetTimeSamplesInInterval(
            Gf.Interval(-5, 25)),
            [20.0])

        self.CheckTimeSamples(attr1)
        self.CheckTimeSamples(attr2)
        self.CheckTimeSamples(attr3)
        self.CheckTimeSamples(attr4)

    def test_ClipTimingDiscontinuities(self):
        """Tests behavior of clip timing with discontinuities to control
        looping"""
        stage = Usd.Stage.Open('timingDiscontinuity/root.usda')
        attr = stage.GetAttributeAtPath('/World.value')

        # Test that values interpolate up to the discontinuity at
        # time 10, then loop back to the start of the clip at time 10.
        self.CheckValue(attr, time=6, expected=6)
        self.CheckValue(attr, time=7, expected=7)
        self.CheckValue(attr, time=8, expected=8)
        self.CheckValue(attr, time=9, expected=9)
        self.CheckValue(attr, time=9.5, expected=9.5)
        self.CheckValue(attr, 
                        time=10 - Usd.TimeCode.SafeStep(), 
                        expected=10 - Usd.TimeCode.SafeStep())
        self.CheckValue(attr, time=10, expected=0)
        self.CheckValue(attr, time=11, expected=1)
        self.CheckValue(attr, time=12, expected=2)
        self.CheckValue(attr, time=13, expected=3)

        # The list of time samples includes an entry at each discontinuity.
        # If there's a discontinuity at time t, there will be time samples
        # at t and t - Usd.TimeCode.SafeStep(). This allows us to represent
        # the discontinuity consistently when flattening the attribute.
        self.assertEqual(
            attr.GetTimeSamples(), 
            [-10, 0, 3, 6, 10 - Usd.TimeCode.SafeStep(), 10, 
             13, 16, 20 - Usd.TimeCode.SafeStep(), 20])

        self.CheckTimeSamples(attr)

    def test_ClipReverseTiming(self):
        '''Tests behavior when reversing time samples in clips'''
        stage = Usd.Stage.Open('reversing/root.usda')
        attr = stage.GetAttributeAtPath('/Model.size')

        # From time [0, 4] we retrieve values from the clip at times [0, 4]
        self.CheckValue(attr, time=0, expected=0)
        self.CheckValue(attr, time=1, expected=2)
        self.CheckValue(attr, time=2, expected=4)
        self.CheckValue(attr, time=3, expected=6)
        self.CheckValue(attr, time=4, expected=8)

        # From time (4, 8] the times metadata reverse the clip times, so at
        # time = 5 we get the value in the clip at time 3, at time = 6 we get
        # the value in the clip at time 2, etc.
        self.CheckValue(attr, time=5, expected=6)
        self.CheckValue(attr, time=6, expected=4)
        self.CheckValue(attr, time=7, expected=2)
        self.CheckValue(attr, time=8, expected=0)

        self.assertEqual(
            attr.GetTimeSamples(),
            [0, 2, 4, 6, 8])

        self.CheckTimeSamples(attr)

        # Verify reversed timing for time-based values. The 'times' metadata
        # maps external [0, 4] to internal [0, 4] (scale 1, offset 0), then
        # reverses on external (4, 8], mapping to internal [4, 0] (scale -1,
        # offset 8). A GfTimeCode is a point on the time axis, so it is retimed
        # with the segment's full linear map (scale and offset). A GfDuration is
        # a length, so only the scale is applied (a constant offset cancels);
        # when the segment reverses the duration is negated and can go negative.
        timeAttr = stage.GetAttributeAtPath('/Model.time')
        durAttr = stage.GetAttributeAtPath('/Model.dur')

        # Default values pass through unaffected by clip retiming.
        self.CheckValue(timeAttr, expected=1.0)
        self.CheckValue(durAttr, expected=Gf.Duration(1.0))

        # Forward segment (scale 1, offset 0): timecode and duration agree.
        self.CheckValue(timeAttr, time=0, expected=10.0)
        self.CheckValue(timeAttr, time=1, expected=12.0)
        self.CheckValue(timeAttr, time=2, expected=14.0)
        self.CheckValue(timeAttr, time=3, expected=16.0)
        self.CheckValue(timeAttr, time=4, expected=18.0)

        self.CheckValue(durAttr, time=0, expected=Gf.Duration(10.0))
        self.CheckValue(durAttr, time=1, expected=Gf.Duration(12.0))
        self.CheckValue(durAttr, time=2, expected=Gf.Duration(14.0))
        self.CheckValue(durAttr, time=3, expected=Gf.Duration(16.0))
        self.CheckValue(durAttr, time=4, expected=Gf.Duration(18.0))

        # Reversed segment (scale -1, offset 8): the clip authors samples at
        # internal times 0, 2, 4, which retime to external times 4, 6, 8, so
        # 6 and 8 are exact samples. A timecode keeps the segment's offset
        # (8 - v): 14 -> -6, 10 -> -2. A duration is scale-only (-v): 14 -> -14,
        # 10 -> -10, i.e. negative and diverging from the timecode.
        self.CheckValue(timeAttr, time=6, expected=-6.0)
        self.CheckValue(timeAttr, time=8, expected=-2.0)

        self.CheckValue(durAttr, time=6, expected=Gf.Duration(-14.0))
        self.CheckValue(durAttr, time=8, expected=Gf.Duration(-10.0))

        # Values between the retimed samples are linearly interpolated by value
        # resolution from the *converted* bracketing samples, not by evaluating
        # a single segment's linear map at the query time. External time 5 falls
        # between the samples at external 4 (forward: 18) and external 6
        # (reversed: -6 for timecode, -14 for duration), so it blends across the
        # timing peak: timecode -> (18 + -6)/2 = 6, duration -> (18 + -14)/2 = 2.
        # External time 7 lies wholly within the reversed segment, between the
        # samples at 6 and 8.
        self.CheckValue(timeAttr, time=5, expected=6.0)
        self.CheckValue(timeAttr, time=7, expected=-4.0)

        self.CheckValue(durAttr, time=5, expected=Gf.Duration(2.0))
        self.CheckValue(durAttr, time=7, expected=Gf.Duration(-12.0))

        self.assertEqual(timeAttr.GetTimeSamples(), [0, 2, 4, 6, 8])
        self.assertEqual(durAttr.GetTimeSamples(), [0, 2, 4, 6, 8])

        self.CheckTimeSamples(timeAttr)
        self.CheckTimeSamples(durAttr)

    def test_ClipStrengthOrderingInherits(self):
        '''Tests strength of clips when inherits provides clips with nested
        prims during resolution'''
        rootLayerFile = 'inherits/root.usda'
        stage = Usd.Stage.Open(rootLayerFile)

        primInherited = stage.GetPrimAtPath("/InheritedClips/Inner")
        attrInherited = primInherited.GetAttribute("attr")
        self.CheckValue(attrInherited, time=0, expected=10)
        self.CheckValue(attrInherited, time=1, expected=20)
        self.CheckValue(attrInherited, time=2, expected=30)

        # The "_firstInherits" class should have the strongest opinion,
        # clobbering any clips from "_hasClips"
        primClobberedBase = stage.GetPrimAtPath("/InheritedClipsClobbered")
        attrClobberedBase = primClobberedBase.GetAttribute("attr")
        self.CheckValue(attrClobberedBase, time=0, expected=-1)
        self.CheckValue(attrClobberedBase, time=1, expected=-2)
        self.CheckValue(attrClobberedBase, time=2, expected=-3)

        primClobbered = stage.GetPrimAtPath("/InheritedClipsClobbered/Inner")
        attrClobbered = primClobbered.GetAttribute("attr")
        self.CheckValue(attrClobbered, time=0, expected=-10)
        self.CheckValue(attrClobbered, time=1, expected=-20)

    def test_ClipStrengthOrdering(self):
        '''Tests strength of clips during resolution'''

        rootLayerFile = 'ordering/root.usda'
        clipFile = 'ordering/clip.usda'
        subLayerClipIntroFile = \
            'ordering/sublayer_with_clip_intro.usda'
        subLayerWithOpinionFile = \
            'ordering/sublayer_with_opinion.usda'

        clipLayer = Sdf.Layer.FindOrOpen(clipFile)
        subLayerClipIntroLayer = Sdf.Layer.FindOrOpen(subLayerClipIntroFile)
        subLayerWithOpinionLayer = Sdf.Layer.FindOrOpen(subLayerWithOpinionFile)

        primPath = Sdf.Path('/Model')
        
        stage = Usd.Stage.Open(rootLayerFile)

        model = stage.GetPrimAtPath(primPath)

        # Ensure that a stronger layer wins over clips
        propName = 'baz'
        attr = model.GetAttribute(propName)
        self.assertEqual(attr.GetPropertyStack(10.0),
                    [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                     [subLayerClipIntroLayer, clipLayer, subLayerWithOpinionLayer]])
        # With a default time code, clips won't show up
        self.assertEqual(attr.GetPropertyStack(Usd.TimeCode.Default()),
                    [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                     [subLayerClipIntroLayer, subLayerWithOpinionLayer]])
        self.CheckValue(attr, time=10, expected=5.0)

        # Ensure that a clip opinion wins out over a weaker sublayer
        propName = 'foo'
        attr = model.GetAttribute(propName)
        self.assertEqual(attr.GetPropertyStack(5.0),
                    [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                     [clipLayer, subLayerWithOpinionLayer]])
        # With a default time code, clips won't show up
        self.assertEqual(attr.GetPropertyStack(Usd.TimeCode.Default()),
                    [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                     [subLayerWithOpinionLayer]])
        self.CheckValue(attr, time=5, expected=50.0) 

        # Ensure fallback to weaker layers works as intended 
        propName = 'bar'
        attr = model.GetAttribute(propName)
        self.assertEqual(attr.GetPropertyStack(15.0),
                    [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                     [subLayerWithOpinionLayer]])
        # With a default time code, clips won't show up
        self.assertEqual(attr.GetPropertyStack(Usd.TimeCode.Default()),
                    [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                     [subLayerWithOpinionLayer]])
        self.CheckValue(attr, time=15, expected=500.0)

    def test_SingleClip(self):
        """Verifies behavior with a single clip being applied to a prim"""
        stage = Usd.Stage.Open('singleclip/root.usda')

        model = stage.GetPrimAtPath('/SingleClip')

        # This prim has a single clip that contributes just one time sample
        # for this attribute. That value will be used over all time.
        attr_1 = model.GetAttribute('attr_1')

        self.assertFalse(attr_1.ValueMightBeTimeVarying())
        self.CheckValue(attr_1, time=0, expected=10.0)
        self.assertEqual(attr_1.GetBracketingTimeSamples(0.0), (0.0, 0.0))
        self.assertEqual(attr_1.GetTimeSamples(), [0.0])
        self.assertEqual(attr_1.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [0.0])

        self.CheckTimeSamples(attr_1)

        # This attribute has no time samples in the clip or elsewhere. Value 
        # resolution will fall back to the default value, which will be used over 
        # all time.
        attr_2 = model.GetAttribute('attr_2')

        self.assertFalse(attr_2.ValueMightBeTimeVarying())
        self.CheckValue(attr_2, time=0, expected=2.0)
        self.assertEqual(attr_2.GetBracketingTimeSamples(0.0), ())
        self.assertEqual(attr_2.GetTimeSamples(), [])
        self.assertEqual(attr_2.GetTimeSamplesInInterval( 
            Gf.Interval.GetFullInterval()), [])

        self.CheckTimeSamples(attr_2)

        # This attribute has a spline, all of whose knots and extrapolations
        # will be used because there is only one clip.
        attr_3 = model.GetAttribute('attr_3')
        self.CheckSpline(attr_3)
        floatType = "float"
        spline = Ts.Spline(floatType)
        k1 = Ts.Knot(floatType, time=-1, value=1,
                     nextInterp=Ts.InterpLinear)
        k2 = Ts.Knot(floatType, time=2, value=2,
                     nextInterp=Ts.InterpLinear)
        spline.SetKnot(k1)
        spline.SetKnot(k2)
        spline.SetPreExtrapolation(Ts.Extrapolation(Ts.ExtrapLoopReset))
        postExtrap = Ts.Extrapolation(Ts.ExtrapSloped)
        postExtrap.slope = 1.0
        spline.SetPostExtrapolation(postExtrap)
        self.assertEqual(attr_3.GetSpline(), spline)

    def test_MultipleClips(self):
        """Verifies behavior with multiple clips being applied to a single prim"""
        stage = Usd.Stage.Open('multiclip/root.usda')

        model = stage.GetPrimAtPath('/Model_1')
        attr = model.GetAttribute('size')

        # This prim has multiple clips that contribute values to this attribute,
        # so it should be detected as potentially time varying.
        self.assertTrue(attr.ValueMightBeTimeVarying())
        
        # clip1 is active in the range [..., 16)
        # clip2 is active in the range [16, ...)
        # Check that we get time samples from the right clip when querying
        # in those ranges.
        self.CheckValue(attr, time=5, expected=-5)
        self.CheckValue(attr, time=10, expected=-10)
        self.CheckValue(attr, time=15, expected=-15)
        # we are at clip boundary at time=16, for pre-time(16) we will be in 
        # clip1 and for ordinary value we will be extrapolating from first
        # sample of clip2, ie. our ordinary value will be -23 but our pre-time
        # will come from clip1 which is -15
        self.CheckValue(attr, time=Usd.TimeCode.PreTime(16), expected=-15)
        self.CheckValue(attr, time=16, expected=-23)
        self.CheckValue(attr, time=19, expected=-23)
        self.CheckValue(attr, time=22, expected=-26)
        self.CheckValue(attr, time=25, expected=-29)

        # Value clips introduce time samples at their boundaries, even if there
        # isn't an actual time sample in the clip at that time. This is to
        # isolate them from surrounding clips. So, the value from frame 16 comes
        # from clip 2.
        self.CheckValue(attr, time=16, expected=-23)
        self.assertEqual(attr.GetBracketingTimeSamples(16), (16, 16))

        # Verify that GetTimeSamples() returns time samples from both clips.
        self.assertEqual(
            attr.GetTimeSamples(), 
            [0.0, 5.0, 10.0, 15.0, 16.0 - Usd.TimeCode.SafeStep(), 16.0, 19.0, 
             22.0, 25.0, 32.0])
        self.assertEqual(
            attr.GetTimeSamplesInInterval(Gf.Interval(0, 30)), 
            [0.0, 5.0, 10.0, 15.0, 16.0 - Usd.TimeCode.SafeStep(), 16.0, 19.0,
             22.0, 25.0])
        self.CheckTimeSamples(attr)

    def test_MultipleClipsWithNoTimeSamples(self):
        """Tests behavior when multiple clips are specified on a prim and none
        have time samples for an attributed owned by that prim."""
        stage = Usd.Stage.Open('multiclip/root.usda')

        model = stage.GetPrimAtPath('/ModelWithNoClipSamples')
        attr = model.GetAttribute('size')
        
        # Since none of the clips provide samples for this attribute, we should
        # fall back to the default value and report that this attribute's values
        # are constant over time.
        self.assertFalse(attr.ValueMightBeTimeVarying())
        self.assertEqual(attr.GetResolveInfo(0).GetSource(),
            Usd.ResolveInfoSourceDefault)

        # This prim has multiple clips specified from frames [0.0, 31.0] but
        # none provide samples for the size attribute. The value in this
        # time range should be equal to the default value from the reference.
        # The value outside this time range should also be the default
        # value, since no clips are active in those times.
        for t in range(-10, 40):
            self.CheckValue(attr, time=t, expected=1.0)

        # Since none of the clips provide samples, there should be no
        # time samples or bracketing time samples at any of these times.
        for t in range(-10, 40):
            self.assertEqual(attr.GetBracketingTimeSamples(t), ())

        self.assertEqual(attr.GetTimeSamples(), [])
        self.assertEqual(attr.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])

        self.CheckTimeSamples(attr)

    def test_MultipleClipsWithSomeTimeSamples(self):
        """Tests behavior when multiple clips are specified on a prim and
        some of them have samples for an attribute owned by that prim, while
        others do not."""
        stage = Usd.Stage.Open('multiclip/root.usda')
        
        model = stage.GetPrimAtPath('/ModelWithSomeClipSamples')
        attr = model.GetAttribute('size')
        
        # The clip in the range [..., 16) has no samples for the attribute,
        # so the value should be the default value from the manifest. Since
        # no default value is specified, we get a value of None.
        with InterpolationType(stage, Usd.InterpolationTypeLinear):
            for t in range(-10, 16):
                self.CheckValue(attr, time=t, expected=None)

        with InterpolationType(stage, Usd.InterpolationTypeHeld):
            for t in range(-10, 16):
                self.CheckValue(attr, time=t, expected=None)

        # This attribute should be detected as potentially time-varying
        # since multiple clips are involved and at least one of them has
        # samples.
        self.assertTrue(attr.ValueMightBeTimeVarying())

        # The clip in the range [16, ...) has samples on frames 3, 6, 9 so
        # we expect time samples for this attribute at frames 19, 22, and 25.
        with InterpolationType(stage, Usd.InterpolationTypeHeld):
            # We are at clip boundary at time=16, for pre-time(16) we will be in
            # nosample_clip, and hence we will get None, which is same as a
            # value block.
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(16), expected=None)
            self.CheckValue(attr, time=16, expected=-23.0)
            self.CheckValue(attr, time=17, expected=-23.0)
            self.CheckValue(attr, time=18, expected=-23.0)
            self.CheckValue(attr, time=19, expected=-23.0)
            self.CheckValue(attr, time=20, expected=-23.0)
            self.CheckValue(attr, time=21, expected=-23.0)
            # We are at a sample boundary at time=22, with held interpolation,
            # for pre-time(22), we will hold the value from previous sample,
            # that is -23.0
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(22), expected=-23.0)
            self.CheckValue(attr, time=22, expected=-26.0)
            self.CheckValue(attr, time=23, expected=-26.0)
            self.CheckValue(attr, time=24, expected=-26.0)
            # We are at a sample boundary at time=25, with held interpolation,
            # for pre-time(25), we will hold the value from previous sample,
            # that is -26.0
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(25), expected=-26.0)
            self.CheckValue(attr, time=25, expected=-29.0)
            self.CheckValue(attr, time=26, expected=-29.0)
            self.CheckValue(attr, time=27, expected=-29.0)
            self.CheckValue(attr, time=28, expected=-29.0)
            self.CheckValue(attr, time=29, expected=-29.0)
            self.CheckValue(attr, time=30, expected=-29.0)
            self.CheckValue(attr, time=31, expected=-29.0)

        # Repeat test with linear interpolation
        with InterpolationType(stage, Usd.InterpolationTypeLinear):
            # We are at clip boundary at time=16, for pre-time(16) we will be in
            # nosample_clip, and hence we will get None, which is same as a
            # value block.
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(16), expected=None)
            self.CheckValue(attr, time=16, expected=-23.0)
            self.CheckValue(attr, time=17, expected=-23.0)
            self.CheckValue(attr, time=18, expected=-23.0)
            self.CheckValue(attr, time=19, expected=-23.0)
            self.CheckValue(attr, time=20, expected=-24.0)
            self.CheckValue(attr, time=21, expected=-25.0)
            self.CheckValue(attr, time=22, expected=-26.0)
            self.CheckValue(attr, time=23, expected=-27.0)
            self.CheckValue(attr, time=24, expected=-28.0)
            self.CheckValue(attr, time=25, expected=-29.0)
            self.CheckValue(attr, time=26, expected=-29.0)
            self.CheckValue(attr, time=27, expected=-29.0)
            self.CheckValue(attr, time=28, expected=-29.0)
            self.CheckValue(attr, time=29, expected=-29.0)
            self.CheckValue(attr, time=30, expected=-29.0)
            self.CheckValue(attr, time=31, expected=-29.0)

        self.assertEqual(
            attr.GetTimeSamples(), 
            [0.0, 16.0 - Usd.TimeCode.SafeStep(), 16.0, 19.0, 22.0, 25.0, 32.0])
        self.assertEqual(
            attr.GetTimeSamplesInInterval(Gf.Interval(-5, 50)), 
            [0.0, 16.0 - Usd.TimeCode.SafeStep(), 16.0, 19.0, 22.0, 25.0, 32.0])

        self.CheckTimeSamples(attr)

    def test_MultipleClipsWithSomeTimeSamples2(self):
        """Another test case similar to TestMultipleClipsWithSomeTimeSamples2."""
        stage = Usd.Stage.Open('multiclip/root.usda')

        model = stage.GetPrimAtPath('/ModelWithSomeClipSamples2')
        attr = model.GetAttribute('size')

        # This attribute should be detected as potentially time-varying
        # since multiple clips are involved and at least one of them has
        # samples.
        self.assertTrue(attr.ValueMightBeTimeVarying())

        # Clips are active in the range [..., 4.0), [4.0, 8.0), and [8.0, ...).
        # The first and last clips have time samples for the size attribute,
        # while the middle clip does not.
        with InterpolationType(stage, Usd.InterpolationTypeHeld):
            # First clip.
            self.CheckValue(attr, time=-1, expected=-23.0)
            self.CheckValue(attr, time=0, expected=-23.0)
            self.CheckValue(attr, time=1, expected=-23.0)
            self.CheckValue(attr, time=2, expected=-23.0)
            # We are at a sample boundary at time=3, with held interpolation,
            # for pre-time(3), we will hold the value from previous sample,
            # that is -23.0
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(3), expected=-23.0)
            self.CheckValue(attr, time=3, expected=-26.0)

            # We are at clip boundary at time=4, for pre-time(4) we will be in 
            # clip2, and its value will be held from the previous sample, that
            # is -26.0
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(4), expected=-26.0)
            self.CheckValue(attr, time=4, expected=None)
            # Middle clip with no samples. Since the middle clip has no 
            # time samples and there is no default value specified in the
            # manifest, we get a value of None.
            self.CheckValue(attr, time=5, expected=None)
            self.CheckValue(attr, time=6, expected=None)
            self.CheckValue(attr, time=7, expected=None)

            # We are at clip boundary at time=8, for pre-time(8) we will be in
            # nosample_clip, and hence we will get None, which is same as a
            # value block.
            # Last clip.
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(8), expected=None)
            self.CheckValue(attr, time=8, expected=-26.0)
            self.CheckValue(attr, time=9, expected=-26.0)
            self.CheckValue(attr, time=10, expected=-26.0)
            # We are at a sample boundary at time=11, with held interpolation,
            # for pre-time(11), we will hold the value from previous sample,
            # that is -26.0
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(11), expected=-26.0)
            self.CheckValue(attr, time=11, expected=-29.0)
            self.CheckValue(attr, time=12, expected=-29.0)

        # Repeat test with linear interpolation
        with InterpolationType(stage, Usd.InterpolationTypeLinear):
            # First clip.
            self.CheckValue(attr, time=-1, expected=-23.0)
            self.CheckValue(attr, time=0, expected=-23.0)
            self.CheckValue(attr, time=1, expected=-24.0)
            self.CheckValue(attr, time=2, expected=-25.0)
            self.CheckValue(attr, time=3, expected=-26.0)

            # We are at clip boundary at time=4, for pre-time(4) we will be in
            # clip2, and since clip times have 4 mapped to 7, we will get the
            # value at 7, which is interpolated to -27.0
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(4), expected=-27.0)
            # Middle clip with no samples. Since the middle clip has no 
            # time samples and there is no default value specified in the
            # manifest, we get a value of None.
            self.CheckValue(attr, time=4, expected=None)
            self.CheckValue(attr, time=5, expected=None)
            self.CheckValue(attr, time=6, expected=None)
            self.CheckValue(attr, time=7, expected=None)

            # Last clip.
            # We are at clip boundary at time=8, for pre-time(8) we will be in
            # nosample_clip, and hence we will get None, which is same as a
            # value block.
            self.CheckValue(attr, time=Usd.TimeCode.PreTime(8), expected=None)
            self.CheckValue(attr, time=8, expected=-26.0)
            self.CheckValue(attr, time=9, expected=-27.0)
            self.CheckValue(attr, time=10, expected=-28.0)
            self.CheckValue(attr, time=11, expected=-29.0)
            self.CheckValue(attr, time=12, expected=-29.0)

        self.assertEqual(
            attr.GetTimeSamples(), 
            [0.0, 3.0, 4.0 - Usd.TimeCode.SafeStep(), 4.0, 7.0, 8.0, 11.0])
        self.assertEqual(
            attr.GetTimeSamplesInInterval(Gf.Interval(0, 10)), 
            [0.0, 3.0, 4.0 - Usd.TimeCode.SafeStep(), 4.0, 7.0, 8.0])

        self.CheckTimeSamples(attr)

    def test_MultipleClipsWithSomeTimeSamples3(self):
        """Tests multi-clip case where first clip has no time samples"""
        stage = Usd.Stage.Open('multiclip/root.usda')

        attr = stage.GetAttributeAtPath('/ModelWithSomeClipSamples3.size')

        # The first active clip has no time samples, so at the first clip's
        # start time we should get None since no default value is declared
        # in the manifest.
        self.CheckValue(attr, time=0, expected=None)

        # At time 1 we should interpolate between the value at the first
        # clip's start time and the value at t=2, which is the next clip's
        # start time.
        self.CheckValue(attr, time=1, expected=None)
        
        # We are at clip boundary at time=2, for pre-time(2) we will be in
        # nosample_clip, and hence we will get None, which is same as a
        # value block.
        # Verify the time samples from the second clip.
        self.CheckValue(attr, time=Usd.TimeCode.PreTime(2), expected=None)
        self.CheckValue(attr, time=2, expected=-23)
        self.CheckValue(attr, time=3, expected=-23)
        self.CheckValue(attr, time=6, expected=-26)
        self.CheckValue(attr, time=9, expected=-29)

        # There must be a time sample for each clip at their start and end times.
        self.assertEqual(attr.GetTimeSamples(), [0.0, 2.0, 3.0, 6.0, 9.0])

        self.CheckTimeSamples(attr)

    def test_MultipleClipsWithTimesSpanningClips(self):
        """Tests that clip time mappings that span multiple clips work as
        expected"""
        stage = Usd.Stage.Open('multiclip/root.usda')

        model = stage.GetPrimAtPath('/ModelWithTimesSpanningClips')
        attr = model.GetAttribute('size')

        # The clip time mappings specified for this prim span a time range
        # where two different clips are active. For a given stage time, the
        # corresponding clip time should be determined from the mapping first,
        # independent of what clip is active. The active clip should then be
        # consulted at that clip time to retrieve the final value.
        self.CheckValue(attr, time=1, expected=100.0)
        self.CheckValue(attr, time=1.5, expected=150.0)
        self.CheckValue(attr, time=2, expected=200.0)
        self.CheckValue(attr, time=2.5, expected=250.0)
        self.CheckValue(attr, time=3, expected=300.0)
        self.CheckValue(attr, time=3.5, expected=350.0)
        self.CheckValue(attr, time=4, expected=400.0)

        self.assertEqual(attr.GetTimeSamples(), [1.0, 2.0, 3.0, 4.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0, 3)), 
                         [1.0, 2.0, 3.0])

    def test_MultipleClipsWithTimesSpanningClipsWithDifferentTypes(self):
        """Tests that clip time mappings that span multiple clips with different
           attribute types specified in various clips work as expected"""
        stage = Usd.Stage.Open('multiclip/root.usda')

        model = stage.GetPrimAtPath(
            '/ModelWithTimesSpanningClipsWithDifferentTypes')
        attr = model.GetAttribute('size')

        # The clip time mappings specified for this prim span a time range
        # where two different clips are active. For a given stage time, the
        # corresponding clip time should be determined from the mapping first,
        # independent of what clip is active. The active clip should then be
        # consulted at that clip time to retrieve the final value. The type of
        # the attribute from the active clip should be respected as well.
        self.CheckValue(attr, time=1, expected=100.0)
        self.CheckValue(attr, time=1.5, expected=150.5)
        self.CheckValue(attr, time=2, expected=201.0)
        self.CheckValue(attr, time=2.5, expected=201.0)
        self.CheckValue(attr, time=3, expected="three")
        self.CheckValue(attr, time=3.5, expected="three")
        self.CheckValue(attr, time=4, expected="four")

        self.assertEqual(attr.GetTimeSamples(), [1.0, 2.0, 3.0, 4.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0, 3)), 
                         [1.0, 2.0, 3.0])

    def test_MultipleClipsWithTimesSpanningClips2(self):
        """Another test similar to test_MultipleClipsWithTimesSpanningClips"""
        stage = Usd.Stage.Open('multiclip/root.usda')

        model = stage.GetPrimAtPath('/ModelWithTimesSpanningClips_2')
        attr = model.GetAttribute('size')

        self.CheckValue(attr, time=100.5, expected=100.5)
        self.CheckValue(attr, time=100.75, expected=100.75)
        self.CheckValue(attr, time=101.0, expected=101.0)
        self.CheckValue(attr, time=101.25, expected=151.25)
        self.CheckValue(attr, time=101.5, expected=201.5)
        self.CheckValue(attr, time=101.75, expected=201.75)
        self.CheckValue(attr, time=102, expected=202)

        self.assertEqual(attr.GetTimeSamples(), 
                         [100.5, 100.75, 101.0, 101.5, 101.75, 102.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(101, 102)), 
                         [101.0, 101.5, 101.75, 102.0])

        self.CheckTimeSamples(attr)

    def test_MultipleClipsWithTimesSpanningClips3(self):
        """Check behavior of multiple clips with a jump discontinuity
        not on an active time boundary
        """
        stage = Usd.Stage.Open('multiclip/root.usda')

        model = stage.GetPrimAtPath('/ModelWithTimesSpanningClips_3')
        attr = model.GetAttribute('size')

        self.CheckValue(attr, time=0, expected=105)
        self.CheckValue(attr, time=5, expected=105)
        self.CheckValue(attr, time=Usd.TimeCode.PreTime(10), expected=910)
        self.CheckValue(attr, time=10, expected=910)
        self.CheckValue(attr, time=Usd.TimeCode.PreTime(20), expected=920)
        self.CheckValue(attr, time=20, expected=900)
        self.CheckValue(attr, time=30, expected=910)

        self.CheckTimeSamples(attr)

    def test_MultipleClipsWithNoTimes(self):
        """Test sequencing multiple clips together with no times metadata
        to remap times."""
        stage = Usd.Stage.Open('multiclip/root.usda')

        attr = stage.GetAttributeAtPath('/ModelWithNoTimes.size')

        # In this test case there is no times metadata specified,
        # so the stage times map to the clip times directly. 
        self.CheckValue(attr, time=0.0, expected=-5)
        self.CheckValue(attr, time=5.0, expected=-5)
        self.CheckValue(attr, time=7.0, expected=-27)
        self.CheckValue(attr, time=9.0, expected=-29)

        # At t=6 we are between the last time sample in the active range
        # of the first clip and the first time sample in the second clip.
        # We should interpolate between these two sample values.
        self.CheckValue(attr, time=6.0, expected=-16)

        # The clips should only contribute time samples from their active
        # range, so we should only get the samples at 5.0 from the first
        # clip, 9.0 from the second clip, and 0.0 and 7.0 from the active
        # metadata.
        self.assertEqual(attr.GetTimeSamples(), [0.0, 5.0, 7.0, 9.0])
        self.CheckTimeSamples(attr)

    def test_InterpolateMissingClipValues(self):
        """Tests interpolation of values for clips that do not have time
        samples for attributes that have been declared in the manifest."""
        def _Test(prim):
            # The first clip active in the range [0, 2) has no samples for
            # this attribute. We should hold the value in this time range
            # from the first sample from the second clip.
            attrNotInFirstClip = prim.GetAttribute('attrNotInFirstClip')

            self.CheckValue(attrNotInFirstClip, time=-1, expected=300)
            self.CheckValue(attrNotInFirstClip, time=-0.5, expected=300)
            self.CheckValue(attrNotInFirstClip, time=0, expected=300)
            self.CheckValue(attrNotInFirstClip, time=0.5, expected=300)
            self.CheckValue(attrNotInFirstClip, time=1, expected=300)
            self.CheckValue(attrNotInFirstClip, time=1.5, expected=300)
            self.CheckValue(attrNotInFirstClip, time=2, expected=300)
            self.CheckValue(attrNotInFirstClip, time=2.5, expected=350)
            self.CheckValue(attrNotInFirstClip, time=3, expected=400)
            self.CheckValue(attrNotInFirstClip, time=3.5, expected=450)
            self.CheckValue(attrNotInFirstClip, time=4, expected=500)
            self.CheckValue(attrNotInFirstClip, time=4.5, expected=550)
            self.CheckValue(attrNotInFirstClip, time=5, expected=600)
            self.CheckValue(attrNotInFirstClip, time=5.5, expected=650)
            self.CheckValue(attrNotInFirstClip, time=6, expected=700)
            self.CheckValue(attrNotInFirstClip, time=6.5, expected=750)
            self.CheckValue(attrNotInFirstClip, time=7, expected=800)
            self.CheckValue(attrNotInFirstClip, time=7.5, expected=800)
            self.CheckValue(attrNotInFirstClip, time=8, expected=800)

            self.assertEqual(attrNotInFirstClip.GetTimeSamples(),
                             [2.0, 3.0, 4.0, 5.0, 6.0, 7.0])
                
            self.CheckTimeSamples(attrNotInFirstClip)

            # The middle clips that are active in the range [2, 6) have no
            # samples for this attribute. We should interpolate the value in
            # this time range from the first clip and the last clip.
            attrNotInMiddleClips = prim.GetAttribute('attrNotInMiddleClips')

            self.CheckValue(attrNotInMiddleClips, time=-1, expected=100)
            self.CheckValue(attrNotInMiddleClips, time=-0.5, expected=100)
            self.CheckValue(attrNotInMiddleClips, time=0, expected=100)
            self.CheckValue(attrNotInMiddleClips, time=0.5, expected=150)
            self.CheckValue(attrNotInMiddleClips, time=1, expected=200)
            self.CheckValue(attrNotInMiddleClips, time=1.5, expected=250)
            self.CheckValue(attrNotInMiddleClips, time=2, expected=300)
            self.CheckValue(attrNotInMiddleClips, time=2.5, expected=350)
            self.CheckValue(attrNotInMiddleClips, time=3, expected=400)
            self.CheckValue(attrNotInMiddleClips, time=3.5, expected=450)
            self.CheckValue(attrNotInMiddleClips, time=4, expected=500)
            self.CheckValue(attrNotInMiddleClips, time=4.5, expected=550)
            self.CheckValue(attrNotInMiddleClips, time=5, expected=600)
            self.CheckValue(attrNotInMiddleClips, time=5.5, expected=650)
            self.CheckValue(attrNotInMiddleClips, time=6, expected=700)
            self.CheckValue(attrNotInMiddleClips, time=6.5, expected=750)
            self.CheckValue(attrNotInMiddleClips, time=7, expected=800)
            self.CheckValue(attrNotInMiddleClips, time=7.5, expected=800)
            self.CheckValue(attrNotInMiddleClips, time=8, expected=800)

            self.assertEqual(attrNotInMiddleClips.GetTimeSamples(),
                             [0.0, 1.0, 6.0, 7.0])
            self.CheckTimeSamples(attrNotInMiddleClips)

            # The last clip active in the range [6, ...) has no samples for
            # this attribute. We should hold the value in this time range
            # from the last sample in the second-to-last clip.
            attrNotInLastClip = prim.GetAttribute('attrNotInLastClip')

            self.CheckValue(attrNotInLastClip, time=-1, expected=100)
            self.CheckValue(attrNotInLastClip, time=-0.5, expected=100)
            self.CheckValue(attrNotInLastClip, time=0, expected=100)
            self.CheckValue(attrNotInLastClip, time=0.5, expected=150)
            self.CheckValue(attrNotInLastClip, time=1, expected=200)
            self.CheckValue(attrNotInLastClip, time=1.5, expected=250)
            self.CheckValue(attrNotInLastClip, time=2, expected=300)
            self.CheckValue(attrNotInLastClip, time=2.5, expected=350)
            self.CheckValue(attrNotInLastClip, time=3, expected=400)
            self.CheckValue(attrNotInLastClip, time=3.5, expected=450)
            self.CheckValue(attrNotInLastClip, time=4, expected=500)
            self.CheckValue(attrNotInLastClip, time=4.5, expected=550)
            self.CheckValue(attrNotInLastClip, time=5, expected=600)
            self.CheckValue(attrNotInLastClip, time=5.5, expected=600)
            self.CheckValue(attrNotInLastClip, time=6, expected=600)
            self.CheckValue(attrNotInLastClip, time=6.5, expected=600)
            self.CheckValue(attrNotInLastClip, time=7, expected=600)
            self.CheckValue(attrNotInLastClip, time=7.5, expected=600)
            self.CheckValue(attrNotInLastClip, time=8, expected=600)

            self.assertEqual(attrNotInLastClip.GetTimeSamples(),
                             [0.0, 1.0, 2.0, 3.0, 4.0, 5.0])
            self.CheckTimeSamples(attrNotInLastClip)

            # This attribute is in the manifest but not in any clip. We
            # expect to get 1 time sample with a value of None since no 
            # default value is declared in the manifest.
            attrNotInAnyClip = prim.GetAttribute('attrNotInAnyClip')

            self.CheckValue(attrNotInAnyClip, time=-1, expected=None)
            self.CheckValue(attrNotInAnyClip, time=0, expected=None)
            self.CheckValue(attrNotInAnyClip, time=1, expected=None)

            self.assertEqual(attrNotInAnyClip.GetTimeSamples(), [0.0])
            self.CheckTimeSamples(attrNotInAnyClip)

        stage = Usd.Stage.Open('missingValueInterpolation/root.usda')
        _Test(stage.GetPrimAtPath('/Model'))
        _Test(stage.GetPrimAtPath('/ModelWithManifestBlocks'))
        
    def test_InterpolateMissingClipValuesWithBlocksInManifest(self):
        """Tests that interpolation of values for empty clips avoids opening
        layers for clips that are declared to have no values in the manifest."""
        def _OpenTestStage():
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))
            return Usd.Stage.Open('missingValueInterpolation/root.usda')

        # These attributes have been marked as not having time samples in various
        # clips, so when we query for values none of those clips should ever be
        # opened.
        stage = _OpenTestStage()
        attrNotInFirstClip = stage.GetAttributeAtPath(
            '/ModelWithManifestBlocks.attrNotInFirstClip')
        for i in range(0, 8):
            attrNotInFirstClip.Get(i)
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))

        del stage
        stage = _OpenTestStage()
        attrNotInMiddleClips = stage.GetAttributeAtPath(
            '/ModelWithManifestBlocks.attrNotInMiddleClips')
        for i in range(0, 8):
            attrNotInMiddleClips.Get(i)
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))

        del stage
        stage = _OpenTestStage()
        attrNotInLastClip = stage.GetAttributeAtPath(
            '/ModelWithManifestBlocks.attrNotInLastClip')
        for i in range(0, 8):
            attrNotInLastClip.Get(i)
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))

        del stage
        stage = _OpenTestStage()
        attrNotInAnyClip = stage.GetAttributeAtPath(
            '/ModelWithManifestBlocks.attrNotInAnyClip')
        for i in range(0, 8):
            attrNotInAnyClip.Get(i)
            # Note that even though this clip is indicated as not having
            # samples in any clips, we do still wind up opening clip1.usda
            # when querying time samples. Avoiding this would incur an
            # additional check on the manifest in the more common cases,
            # so we choose not to do that.
            self.assertTrue(
                Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))

    def test_InterpolateMissingClipValuesWithFallbacksInManifest(self):
        """Tests that interpolation for missing clip values will be
        skipped for attributes with a fallback value declared in the
        manifest."""
        stage = Usd.Stage.Open('missingValueInterpolation/root.usda')

        attrNotInFirstClip = stage.GetAttributeAtPath(
            '/ModelWithManifestFallbacks.attrNotInFirstClip')

        self.CheckValue(attrNotInFirstClip, time=-1, expected=42)
        self.CheckValue(attrNotInFirstClip, time=-0.5, expected=42)
        self.CheckValue(attrNotInFirstClip, time=0, expected=42)
        self.CheckValue(attrNotInFirstClip, time=0.5, expected=106.5)
        self.CheckValue(attrNotInFirstClip, time=1, expected=171)
        self.CheckValue(attrNotInFirstClip, time=1.5, expected=235.5)
        self.CheckValue(attrNotInFirstClip, time=2, expected=300)

        self.assertEqual(attrNotInFirstClip.GetTimeSamples(),
                         [0.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0])
        self.CheckTimeSamples(attrNotInFirstClip)

        attrNotInMiddleClips = stage.GetAttributeAtPath(
            '/ModelWithManifestFallbacks.attrNotInMiddleClips')

        self.CheckValue(attrNotInMiddleClips, time=1, expected=200)
        self.CheckValue(attrNotInMiddleClips, time=1.5, expected=121)
        self.CheckValue(attrNotInMiddleClips, time=2, expected=42)
        self.CheckValue(attrNotInMiddleClips, time=2.5, expected=42)
        self.CheckValue(attrNotInMiddleClips, time=3, expected=42)
        self.CheckValue(attrNotInMiddleClips, time=3.5, expected=42)
        self.CheckValue(attrNotInMiddleClips, time=4, expected=42)
        self.CheckValue(attrNotInMiddleClips, time=4.5, expected=206.5)
        self.CheckValue(attrNotInMiddleClips, time=5, expected=371)
        self.CheckValue(attrNotInMiddleClips, time=5.5, expected=535.5)
        self.CheckValue(attrNotInMiddleClips, time=6, expected=700)

        self.assertEqual(attrNotInMiddleClips.GetTimeSamples(),
                         [0.0, 1.0, 2.0, 4.0, 6.0, 7.0])
        self.CheckTimeSamples(attrNotInMiddleClips)

        attrNotInLastClip = stage.GetAttributeAtPath(
            '/ModelWithManifestFallbacks.attrNotInLastClip')

        self.CheckValue(attrNotInLastClip, time=5, expected=600.0)
        self.CheckValue(attrNotInLastClip, time=5.5, expected=321.0)
        self.CheckValue(attrNotInLastClip, time=6, expected=42.0)
        self.CheckValue(attrNotInLastClip, time=7, expected=42.0)
        self.CheckValue(attrNotInLastClip, time=8, expected=42.0)

        self.assertEqual(attrNotInLastClip.GetTimeSamples(),
                         [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0])
        self.CheckTimeSamples(attrNotInLastClip)

        attrNotInAnyClip = stage.GetAttributeAtPath(
            '/ModelWithManifestFallbacks.attrNotInAnyClip')

        self.CheckValue(attrNotInAnyClip, time=-1, expected=42.0)
        self.CheckValue(attrNotInAnyClip, time=0, expected=42.0)
        self.CheckValue(attrNotInAnyClip, time=1, expected=42.0)

        self.assertEqual(attrNotInAnyClip.GetTimeSamples(), 
                         [0.0, 2.0, 4.0, 6.0, 7.0])
        self.CheckTimeSamples(attrNotInAnyClip)

    def test_GetTimeSamplesInIntervalWithoutInterpolation(self):
        """Tests behavior of GetTimeSamplesInInterval with clip sets
        that are missing time samples with interpolation between
        missing clip values turned off."""
        def _OpenTestStage():
            # Use the test case from missingValueInterpolation but turn off
            # the interpolation behavior for this test case.
            stage = Usd.Stage.Open('missingValueInterpolation/root.usda')
            Sdf.CreatePrimInLayer(stage.GetSessionLayer(), '/Model').SetInfo(
                'clips', {'default': {'interpolateMissingClipValues' : False}})

            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))

            return stage

        # When interpolation is turned off, querying time samples in an
        # interval should only need to open the clips that are active
        # during that interval. In this case, only clip 1 is active in
        # the time interval [0, 1] with time samples at 0.0 and 1.0.
        stage = _OpenTestStage()
        attrNotInFirstClip = stage.GetAttributeAtPath(
            '/Model.attrNotInLastClip')
        self.assertEqual(
            attrNotInFirstClip.GetTimeSamplesInInterval(
                Gf.Interval(0.0, 1.0)),
            [0.0, 1.0])

        self.assertTrue(
            Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))        

        # If there are no values in any clips, we should still only need
        # to open the clip that is active during that interval, which is
        # clip 1. This is because each clip introduces a time sample at
        # its start time when interpolation is turned off, even if it
        # has no authored samples.
        del stage
        stage = _OpenTestStage()
        attrNotInAnyClip = stage.GetAttributeAtPath('/Model.attrNotInAnyClip')
        self.assertEqual(
            attrNotInAnyClip.GetTimeSamplesInInterval(
                Gf.Interval(0.0, 1.0)),
            [0.0])

        self.assertTrue(
            Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))        

    def test_GetTimeSamplesInIntervalWithInterpolation(self):
        """Tests behavior of GetTimeSamplesInInterval with clip sets
        that are missing time samples with interpolation between
        missing clip values turned on."""
        def _OpenTestStage():
            stage = Usd.Stage.Open('missingValueInterpolation/root.usda')
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))
            self.assertFalse(
                Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))
            return stage

        # When interpolation is turned on, querying time samples in an
        # interval should only need to open the clips that are active
        # during that interval if any of them contain time samples. In
        # this case, only clip 1 is active in the time interval [0, 1]
        # with time samples at 0.0 and 1.0.
        stage = _OpenTestStage()
        attrNotInFirstClip = stage.GetAttributeAtPath(
            '/Model.attrNotInLastClip')
        self.assertEqual(
            attrNotInFirstClip.GetTimeSamplesInInterval(
                Gf.Interval(0.0, 1.0)),
            [0.0, 1.0])

        self.assertTrue(
            Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))        

        # However, if the active clip does not contain time samples,
        # we currently need to scan to see if any other clips provide
        # time samples. In the worst case, when no clips provide samples,
        # this will cause all clips to be opened.
        del stage
        stage = _OpenTestStage()
        attrNotInAnyClip = stage.GetAttributeAtPath('/Model.attrNotInAnyClip')
        self.assertEqual(
            attrNotInAnyClip.GetTimeSamplesInInterval(
                Gf.Interval(0.0, 1.0)),
            [0.0])

        self.assertTrue(
            Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))
        self.assertTrue(
            Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
        self.assertTrue(
            Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))
        self.assertTrue(
            Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))        

        # This can be mitigated by authoring value blocks in the manifest to
        # indicate that certain clips do not provide samples. In this case,
        # we've authored blocks for all clips so none of them should be opened.
        del stage
        stage = _OpenTestStage()
        attrNotInAnyClip = stage.GetAttributeAtPath(
            '/ModelWithManifestBlocks.attrNotInAnyClip')
        self.assertEqual(
            attrNotInAnyClip.GetTimeSamplesInInterval(
                Gf.Interval(0.0, 1.0)),
            [0.0])
        # XXX: The clips code always reports that there exists a time sample at
        # the first time, and since we need to fetch the value type of the
        # samples in the interval the first clip does get opened, but the other
        # clips do not.
        self.assertTrue(
            Sdf.Layer.Find('missingValueInterpolation/clip1.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip2.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip3.usda'))
        self.assertFalse(
            Sdf.Layer.Find('missingValueInterpolation/clip4.usda'))

    def test_AncestralClips(self):
        """Tests that clips specified on a descendant model will override
        clips specified on an ancestral model"""
        stage = Usd.Stage.Open('ancestral/root.usda')

        ancestor = stage.GetPrimAtPath('/ModelGroup')
        ancestorAttr = ancestor.GetAttribute('attr')
        
        self.assertEqual(ancestorAttr.GetTimeSamples(), [0, 5, 10, 15])
        self.assertEqual(ancestorAttr.GetTimeSamplesInInterval(Gf.Interval(0, 15)), 
                         [0, 5, 10, 15])
        self.CheckValue(ancestorAttr, time=5, expected=-5)
        self.CheckValue(ancestorAttr, time=10, expected=-10)
        self.CheckValue(ancestorAttr, time=15, expected=-15)

        # Tests that attributes on prims will receive values from clips specified
        # on ancestors.
        descendant = stage.GetPrimAtPath('/ModelGroup/Subgroup')
        descendantAttr = descendant.GetAttribute('attr')

        self.assertEqual(descendantAttr.GetTimeSamples(), [0, 5, 10, 15])
        self.assertEqual(descendantAttr.GetTimeSamplesInInterval(Gf.Interval(0, 15)), 
                         [0, 5, 10, 15])
        self.CheckValue(descendantAttr, time=5, expected=-5)
        self.CheckValue(descendantAttr, time=10, expected=-10)
        self.CheckValue(descendantAttr, time=15, expected=-15)

        # Tests that clips specified on a descendant model will override
        # clips specified on an ancestral model
        descendant = stage.GetPrimAtPath('/ModelGroup/Subgroup/Model')
        descendantAttr = descendant.GetAttribute('attr')

        self.assertEqual(descendantAttr.GetTimeSamples(), [0, 1, 2, 3])
        self.assertEqual(descendantAttr.GetTimeSamplesInInterval(Gf.Interval(0, 2.95)), 
                         [0, 1, 2])
        self.CheckValue(descendantAttr, time=1, expected=-1)
        self.CheckValue(descendantAttr, time=2, expected=-2)
        self.CheckValue(descendantAttr, time=3, expected=-3)

        self.CheckTimeSamples(ancestorAttr)
        self.CheckTimeSamples(descendantAttr)

    def test_ClipFlatten(self):
        """Ensure that UsdStages with clips are flattened as expected.
        In particular, the time samples in the flattened stage should incorporate
        data from clips, and no clip metadata should be present"""

        stage = Usd.Stage.Open('flatten/root.usda')
        expectedFlatStage = Sdf.Layer.FindOrOpen(
            'flatten/flat.usda')

        self.assertEqual(stage.ExportToString(addSourceFileComment=False),
                    expectedFlatStage.ExportToString())

    def test_ClipValidation(self):
        """Tests validation of clip metadata"""

        # class Listener(object):
        #     def __init__(self):
        #         self.warnings = []
        #         self._listener = Tf.Notice.RegisterGlobally(
        #             'TfDiagnosticNotice::IssuedWarning', 
        #             self._OnNotice)

        #     def _OnNotice(self, notice, sender):
        #         self.warnings.append(notice.warning)

        # l = Listener()

        stage = Usd.Stage.Open('validation/root.usda')

        # XXX: The notice listening portion of this test is disabled for now, since
        # parallel UsdStage population causes these warnings to be emitted from
        # separate threads.  The diagnostic system does not issue notices for
        # warnings and errors not issued from "the main thread".

        # self.assertEqual(len(l.warnings), numExpectedWarnings)

        # # Each 'Error' prim should have caused a warning to be posted.
        # for i in range(1, numExpectedWarnings):
        #     errorPrimName = 'Error%d' % i
        #     numErrorsForPrim = sum(1 if errorPrimName in str(e) else 0 
        #                            for e in l.warnings)
        #     self.assertEqual(numErrorsForPrim, 1)

        # # The 'NoError' prims should not have caused any errors to be posted.
        # self.assertFalse(any(['NoError' in str(e) for e in l.warnings]))

    def test_ClipsOnNonModel(self):
        """Verifies that clips authored on non-models work"""
        stage = Usd.Stage.Open('nonmodel/root.usda')

        nonModel = stage.GetPrimAtPath('/NonModel')
        self.assertFalse(nonModel.IsModel())
        attr = nonModel.GetAttribute('a')
        self.CheckValue(attr, time=1.0, expected=-100.0)

    def test_ClipsCannotIntroduceNewTopology(self):
        """Verifies that clips cannot introduce new scenegraph topology"""
        stage = Usd.Stage.Open('topology/root.usda')

        prim = stage.GetPrimAtPath('/Model')
        self.assertTrue(prim.IsModel())

        # Clips cannot introduce new topology. Prims and properties defined only
        # in the clip should not be visible on the stage.
        self.assertFalse(prim.GetAttribute('clipOnly'))
        self.assertEqual(prim.GetChildren(), [])

    def test_ClipAuthoring(self):
        """Tests clip authoring API on Usd.ClipsAPI"""
        allFormats = ['usd' + x for x in 'ac']
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestClipAuthoring.'+fmt)

            prim = stage.DefinePrim('/Model')
            model = Usd.ClipsAPI(prim)

            prim2 = stage.DefinePrim('/Model2')
            model2 = Usd.ClipsAPI(prim2)

            # Clip authoring API supports the use of lists as well as Vt arrays.
            clipAssetPaths = [Sdf.AssetPath('clip1.usda'), 
                              Sdf.AssetPath('clip2.usda')]
            model.SetClipAssetPaths(clipAssetPaths)
            self.assertEqual(model.GetClipAssetPaths(), clipAssetPaths)

            model2.SetClipAssetPaths(
                Sdf.AssetPathArray([Sdf.AssetPath('clip1.usda'),
                                    Sdf.AssetPath('clip2.usda')]))
            self.assertEqual(model2.GetClipAssetPaths(), clipAssetPaths)

            clipPrimPath = "/Clip"
            model.SetClipPrimPath(clipPrimPath)
            self.assertEqual(model.GetClipPrimPath(), clipPrimPath)

            clipTimes = Vt.Vec2dArray([(0.0, 0.0),(10.0, 10.0),(20.0, 20.0)])
            model.SetClipTimes(clipTimes)
            self.assertEqual(model.GetClipTimes(), clipTimes)

            model2.SetClipTimes(
                Vt.Vec2dArray([Gf.Vec2d(0.0, 0.0),
                               Gf.Vec2d(10.0, 10.0),
                               Gf.Vec2d(20.0, 20.0)]))
            self.assertEqual(model2.GetClipTimes(), clipTimes)

            clipActive = [(0.0, 0.0),(10.0, 1.0),(20.0, 0.0)]
            model.SetClipActive(clipActive)
            self.assertEqual(model.GetClipActive(), Vt.Vec2dArray(clipActive))

            model2.SetClipActive(
                Vt.Vec2dArray([Gf.Vec2d(0.0, 0.0),
                               Gf.Vec2d(10.0, 1.0),
                               Gf.Vec2d(20.0, 0.0)]))
            self.assertEqual(model2.GetClipActive(), Vt.Vec2dArray(clipActive))

            clipManifestAssetPath = Sdf.AssetPath('clip_manifest.usda')
            model.SetClipManifestAssetPath(clipManifestAssetPath)
            self.assertEqual(model.GetClipManifestAssetPath(), clipManifestAssetPath)

            model.SetInterpolateMissingClipValues(True)
            self.assertEqual(model.GetInterpolateMissingClipValues(), True)

            # Test authoring of template clip metadata
            model.SetClipTemplateAssetPath('clip.###.usda')
            self.assertEqual(model.GetClipTemplateAssetPath(), 'clip.###.usda')

            model.SetClipTemplateStride(4.5)
            self.assertEqual(model.GetClipTemplateStride(), 4.5)

            model.SetClipTemplateStartTime(1)
            self.assertEqual(model.GetClipTemplateStartTime(), 1)

            model.SetClipTemplateEndTime(5)
            self.assertEqual(model.GetClipTemplateEndTime(), 5)
        
            # Ensure we can't set the clipTemplateStride to 0
            with self.assertRaises(Tf.ErrorException) as e:
                model.SetClipTemplateStride(0)

            # Ensure we can't set the clipTemplateStride to <0
            with self.assertRaises(Tf.ErrorException) as e:
                model.SetClipTemplateStride(-1)

            model.SetClipTemplateActiveOffset(2)
            self.assertEqual(model.GetClipTemplateActiveOffset(), 2)

            model.SetClipTemplateActiveOffset(-5)
            self.assertEqual(model.GetClipTemplateActiveOffset(), -5)

    def test_ClipSetAuthoring(self):
        """Tests clip authoring API with clip sets on Usd.ClipsAPI"""
        allFormats = ['usd' + x for x in 'ac']
        for fmt in allFormats:
            stage = Usd.Stage.CreateInMemory('TestClipSetAuthoring.'+fmt)

            prim = stage.DefinePrim('/Model')
            model = Usd.ClipsAPI(prim)

            prim2 = stage.DefinePrim('/Model2')
            model2 = Usd.ClipsAPI(prim2)

            clipSetName = "my_clip_set"

            # Clip authoring API supports the use of lists as well as Vt arrays.
            clipAssetPaths = [Sdf.AssetPath('clip1.usda'), 
                              Sdf.AssetPath('clip2.usda')]
            model.SetClipAssetPaths(clipAssetPaths, clipSetName)
            self.assertEqual(model.GetClipAssetPaths(clipSetName), 
                             clipAssetPaths)

            model2.SetClipAssetPaths(
                Sdf.AssetPathArray([Sdf.AssetPath('clip1.usda'),
                                    Sdf.AssetPath('clip2.usda')]),
                clipSetName)
            self.assertEqual(model2.GetClipAssetPaths(clipSetName), 
                             clipAssetPaths)

            clipPrimPath = "/Clip"
            model.SetClipPrimPath(clipPrimPath, clipSetName)
            self.assertEqual(model.GetClipPrimPath(clipSetName), clipPrimPath)

            clipTimes = Vt.Vec2dArray([(0.0, 0.0),(10.0, 10.0),(20.0, 20.0)])
            model.SetClipTimes(clipTimes, clipSetName)
            self.assertEqual(model.GetClipTimes(clipSetName), clipTimes)

            model2.SetClipTimes(
                Vt.Vec2dArray([Gf.Vec2d(0.0, 0.0),
                               Gf.Vec2d(10.0, 10.0),
                               Gf.Vec2d(20.0, 20.0)]),
                clipSetName)
            self.assertEqual(model2.GetClipTimes(clipSetName), clipTimes)

            clipActive = [(0.0, 0.0),(10.0, 1.0),(20.0, 0.0)]
            model.SetClipActive(clipActive, clipSetName)
            self.assertEqual(model.GetClipActive(clipSetName), 
                             Vt.Vec2dArray(clipActive))

            model2.SetClipActive(
                Vt.Vec2dArray([Gf.Vec2d(0.0, 0.0),
                               Gf.Vec2d(10.0, 1.0),
                               Gf.Vec2d(20.0, 0.0)]),
                clipSetName)
            self.assertEqual(model2.GetClipActive(clipSetName), 
                             Vt.Vec2dArray(clipActive))

            clipManifestAssetPath = Sdf.AssetPath('clip_manifest.usda')
            model.SetClipManifestAssetPath(clipManifestAssetPath, clipSetName)
            self.assertEqual(model.GetClipManifestAssetPath(clipSetName), 
                             clipManifestAssetPath)

            # Test authoring of template clip metadata
            model.SetClipTemplateAssetPath('clip.###.usda', clipSetName)
            self.assertEqual(model.GetClipTemplateAssetPath(clipSetName), 
                             'clip.###.usda')

            model.SetClipTemplateStride(4.5, clipSetName)
            self.assertEqual(model.GetClipTemplateStride(clipSetName), 4.5)

            model.SetClipTemplateStartTime(1, clipSetName)
            self.assertEqual(model.GetClipTemplateStartTime(clipSetName), 1)

            model.SetClipTemplateEndTime(5, clipSetName)
            self.assertEqual(model.GetClipTemplateEndTime(clipSetName), 5)
        
            # Ensure we can't set the clipTemplateStride to 0
            with self.assertRaises(Tf.ErrorException) as e:
                model.SetClipTemplateStride(0, clipSetName)

    def test_ClipTimesBracketingTimeSamplePrecision(self):
        stage = Usd.Stage.Open('precision/root.usda')
        prim = stage.GetPrimAtPath('/World/fx/Particles_Splash/points')
        attr = prim.GetAttribute('points')

        self.assertEqual(attr.GetTimeSamples(), [101.0, 101.99, 102.0, 103.0])
        self.assertEqual(attr.GetBracketingTimeSamples(101), (101.00, 101.00))
        self.assertEqual(attr.GetBracketingTimeSamples(101.99), (101.99, 101.99))
        self.assertEqual(attr.GetBracketingTimeSamples(101.90), (101.00, 101.99))
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(101.0,102.0)), 
                    [101.00, 101.99, 102.00])	   

    def test_ClipManifest(self):
        """Verifies behavior with value clips when a clip manifest is 
        specified."""
        stage = Usd.Stage.Open('manifest/root.usda')
        prim = stage.GetPrimAtPath('/WithManifestClip')

        # No clip layers should be loaded yet. We have an manifest explicitly
        # specified so we don't need to open any of the clip layers to
        # generate one.
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))

        # This attribute doesn't exist in the manifest, so we should
        # not have looked in any clips for samples, and its value should
        # fall back to its default value.
        notInManifestAndInClip = prim.GetAttribute('notInManifestAndInClip')
        self.assertFalse(notInManifestAndInClip.ValueMightBeTimeVarying())
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))
        self.CheckValue(notInManifestAndInClip, time=0, expected=3.0)
        self.assertEqual(notInManifestAndInClip.GetTimeSamples(), [])
        self.assertEqual(notInManifestAndInClip.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])
        self.CheckTimeSamples(notInManifestAndInClip)

        # This attribute also doesn't exist in the manifest and also
        # does not have any samples in the clips. It should behave exactly
        # as above; we should not have to open any of the clips.
        notInManifestNotInClip = prim.GetAttribute('notInManifestNotInClip')
        self.assertFalse(notInManifestNotInClip.ValueMightBeTimeVarying())
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))
        self.CheckValue(notInManifestNotInClip, time=0, expected=4.0)
        self.assertEqual(notInManifestNotInClip.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])
        self.CheckTimeSamples(notInManifestNotInClip)
        
        # This attribute is in the manifest but is declared uniform,
        # so we should also not look in any clips for samples.
        uniformInManifestAndInClip = prim.GetAttribute('uniformInManifestAndInClip')
        self.assertFalse(uniformInManifestAndInClip.ValueMightBeTimeVarying())
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))
        self.CheckValue(uniformInManifestAndInClip, time=0, expected=5.0)
        self.assertEqual(uniformInManifestAndInClip.GetTimeSamples(), [])
        self.assertEqual(uniformInManifestAndInClip.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])
        self.CheckTimeSamples(uniformInManifestAndInClip)

        # This attribute is in the manifest and has samples in the
        # first clip, but not the other. We should get the clip's samples
        # in the first time range, and the default value in the second
        # range.
        inManifestAndInClip = prim.GetAttribute('inManifestAndInClip')
        self.assertTrue(inManifestAndInClip.ValueMightBeTimeVarying())
        # Since there's more than one clip we don't need to open any 
        # layers to determine if the attribute might be varying.
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))
        self.CheckValue(inManifestAndInClip, time=0, expected=0.0)
        self.CheckValue(inManifestAndInClip, time=1, expected=-1.0)

        # Note that the clip at t=2 does not have a value for this attribute,
        # and the manifest has no default value specified, so we get None.

        # time=2 is at clip boundary, at pre-time(2.0), we will be in clip1,
        # which will evaluate to -1.0
        self.CheckValue(inManifestAndInClip, time=Usd.TimeCode.PreTime(2), 
                        expected=-1.0)
        self.CheckValue(inManifestAndInClip, time=2, expected=None)

        self.assertEqual(inManifestAndInClip.GetTimeSamples(), 
                         [0.0, 1.0, 2.0, 3.0])
        self.assertEqual(inManifestAndInClip.GetTimeSamplesInInterval(
            Gf.Interval(0, 2.1)), [0.0, 1.0, 2.0])
        self.CheckTimeSamples(inManifestAndInClip)

        # Close and reopen the stage to ensure the clip layers are closed
        # before we do the test below.
        del stage
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))

        # Lastly, this attribute is in the manifest but has no
        # samples in the clip and no default in the manifest, so we should
        # get None.
        stage = Usd.Stage.Open('manifest/root.usda')
        prim = stage.GetPrimAtPath('/WithManifestClip')

        inManifestNotInClip = prim.GetAttribute('inManifestNotInClip')
        self.assertTrue(inManifestNotInClip.ValueMightBeTimeVarying())
        # Since there's more than one clip we don't need to open any 
        # layers to determine if the attribute might be varying.
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))
        self.CheckValue(inManifestNotInClip, time=0, expected=None)
        self.assertEqual(inManifestNotInClip.GetTimeSamples(), 
                         [0.0, 1.0, 2.0, 3.0])
        self.assertEqual(inManifestNotInClip.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [0.0, 1.0, 2.0, 3.0])
        self.CheckTimeSamples(inManifestNotInClip)

    def test_ClipManifestFallback(self):
        """Verifies fallback values from manifest when a clip does not
        have values for an attribute that is in the manifest."""
        stage = Usd.Stage.Open('manifestFallback/root.usda')

        # In the following test cases, the clip that is active at t=2.0
        # contains no attributes.

        # If the attribute is declared with a default value in the
        # manifest, we fall back to that value.
        fallbackInManifest = \
            stage.GetAttributeAtPath('/Model.fallbackInManifest')
        self.CheckValue(fallbackInManifest, time=0.0, expected=10.0)
        # we are at clip boundary at time=2.0, for pre-time(2.0), we will be in
        # clip1 with a jump discontinuity as well, so first time mapping @2 will
        # be used.
        self.CheckValue(fallbackInManifest, time=Usd.TimeCode.PreTime(2.0), 
                        expected=14.0)
        self.CheckValue(fallbackInManifest, time=2.0, expected=50.0)
        # we are at clip boundary at time=4.0, for pre-time(4.0), we will be in
        # nosample clip, which doesn't provide any samples, so instead of using 
        # the first time mapping @4, i.e. (4.0, 2.0) because of jump
        # discontinuity, we we will look for fallback value in manifest, 
        # which is 50.
        self.CheckValue(fallbackInManifest, time=Usd.TimeCode.PreTime(4.0), 
                        expected=50.0)
        self.CheckValue(fallbackInManifest, time=4.0, expected=20.0)
        self.assertEqual(fallbackInManifest.GetTimeSamples(),
                         [0.0, 1.0, 2.0 - Usd.TimeCode.SafeStep(), 2.0,
                          4.0 - Usd.TimeCode.SafeStep(), 4.0])
        self.CheckTimeSamples(fallbackInManifest)
        
        fallbackBlockInManifest = \
            stage.GetAttributeAtPath('/Model.fallbackBlockInManifest')
        self.CheckValue(fallbackBlockInManifest, time=0.0, expected=10.0)
        # we are at clip boundary at time=2.0, for pre-time(2.0), we will be in
        # clip1 with a jump discontinuity as well, so first time mapping @2 will
        # be used.
        self.CheckValue(fallbackBlockInManifest, time=Usd.TimeCode.PreTime(2.0), 
                        expected=14.0)
        self.CheckValue(fallbackBlockInManifest, time=2.0, expected=None)
        # we are at clip boundary at time=4.0, for pre-time(4.0), we will be in
        # nosample clip, so we will look for fallback value in manifest, which
        # has a block.
        self.CheckValue(fallbackBlockInManifest, time=Usd.TimeCode.PreTime(4.0), 
                        expected=None)
        self.CheckValue(fallbackBlockInManifest, time=4.0, expected=20.0)
        self.assertEqual(fallbackBlockInManifest.GetTimeSamples(),
                         [0.0, 1.0, 2.0 - Usd.TimeCode.SafeStep(), 2.0,
                          4.0 - Usd.TimeCode.SafeStep(), 4.0])
        self.CheckTimeSamples(fallbackBlockInManifest)

        # If the attribute is declared without a default value in the
        # manifest, we get a value of None.
        noFallbackInManifest =  \
            stage.GetAttributeAtPath('/Model.noFallbackInManifest')
        self.CheckValue(noFallbackInManifest, time=0.0, expected=10.0)
        # we are at clip boundary at time=2.0, for pre-time(2.0), we will be in
        # clip1, which has a jump discontinuity as well, so first time mapping 
        # @2 will be used.
        self.CheckValue(noFallbackInManifest, time=Usd.TimeCode.PreTime(2.0), 
                        expected=14.0)
        self.CheckValue(noFallbackInManifest, time=2.0, expected=None)
        # we are at clip boundary at time=4.0, for pre-time(4.0), we will be in
        # nosample clip, so we will look for fallback value in manifest, which
        # doesn't have a fallback therefore None, which is same as a block.
        self.CheckValue(noFallbackInManifest, time=Usd.TimeCode.PreTime(4.0), 
                        expected=None)
        self.CheckValue(noFallbackInManifest, time=4.0, expected=20.0)
        self.assertEqual(noFallbackInManifest.GetTimeSamples(),
                         [0.0, 1.0, 2.0 - Usd.TimeCode.SafeStep(), 2.0,
                          4.0 - Usd.TimeCode.SafeStep(), 4.0])
        self.CheckTimeSamples(noFallbackInManifest)

    def test_ClipManifestFallback2(self):
        """Verifies fallback values from manifest when using a single clip
        that does not have values for an attribute that is in the manifest."""
        stage = Usd.Stage.Open('manifestFallback/root.usda')

        # In this test case the prim has a single clip with no samples
        # that is active at time=0.

        # If the attribute is declared with a default value in the
        # manifest, we fall back to that value.
        fallbackInManifest = \
            stage.GetAttributeAtPath('/Model_2.fallbackInManifest')
        self.CheckValue(fallbackInManifest, time=-1.0, expected=50.0)
        self.CheckValue(fallbackInManifest, time=0.0, expected=50.0)
        self.CheckValue(fallbackInManifest, time=1.0, expected=50.0)
        self.assertEqual(fallbackInManifest.GetTimeSamples(), [0.0])
        self.CheckTimeSamples(fallbackInManifest)
        
        fallbackBlockInManifest = \
            stage.GetAttributeAtPath('/Model_2.fallbackBlockInManifest')
        self.CheckValue(fallbackBlockInManifest, time=-1.0, expected=None)
        self.CheckValue(fallbackBlockInManifest, time=0.0, expected=None)
        self.CheckValue(fallbackBlockInManifest, time=1.0, expected=None)
        self.assertEqual(fallbackBlockInManifest.GetTimeSamples(), [0.0])
        self.CheckTimeSamples(fallbackBlockInManifest)

        # If the attribute is declared without a default value in the
        # manifest, we get a value of None.
        noFallbackInManifest =  \
            stage.GetAttributeAtPath('/Model_2.noFallbackInManifest')
        self.CheckValue(noFallbackInManifest, time=-1.0, expected=None)
        self.CheckValue(noFallbackInManifest, time=0.0, expected=None)
        self.CheckValue(noFallbackInManifest, time=1.0, expected=None)
        self.assertEqual(noFallbackInManifest.GetTimeSamples(), [0.0])
        self.CheckTimeSamples(noFallbackInManifest)

    def test_ClipManifestGeneration(self):
        """Tests generating a manifest using UsdClipsAPI"""
        def _ValidateManifest(manifest):
            def _CheckAttribute(path):
                attr = manifest.GetAttributeAtPath(path)
                self.assertTrue(attr)
                self.assertIsNone(attr.default)

            _CheckAttribute('/Clip/A.a')
            _CheckAttribute('/Clip/A.b')
            _CheckAttribute('/Clip/A{v=a}.c')
            _CheckAttribute('/Clip/A.z')

            # We should not have an entry for d in the manifest since it has
            # no time samples in any of the clips.
            self.assertFalse(manifest.GetAttributeAtPath('/Clip/A.d'))

        # Test UsdClipsAPI.GenerateManifestFromLayers
        clip1 = Sdf.Layer.FindOrOpen('manifestGeneration/clip_1.usda')
        self.assertTrue(clip1)
        clip2 = Sdf.Layer.FindOrOpen('manifestGeneration/clip_2.usda')
        self.assertTrue(clip2)

        manifest = Usd.ClipsAPI.GenerateClipManifestFromLayers(
            [clip1, clip2], '/Clip/A')
        _ValidateManifest(manifest)

        # Test errors when passing in a non-prim path
        with self.assertRaises(Tf.ErrorException):
            manifest = Usd.ClipsAPI.GenerateClipManifestFromLayers(
                [clip1, clip2], '/')

        with self.assertRaises(Tf.ErrorException):
            manifest = Usd.ClipsAPI.GenerateClipManifestFromLayers(
                [clip1, clip2], '/Foo.bar')

        # Test errors when passing in an invalid clip layer
        with self.assertRaises(Tf.ErrorException):
            manifest = Usd.ClipsAPI.GenerateClipManifestFromLayers(
                [clip1, None], '/Model')

        # Test UsdClipsAPI.GenerateManifest on authored clip sets.
        stage = Usd.Stage.Open('manifestGeneration/root.usda')
        prim = stage.GetPrimAtPath('/Model')

        clipsAPI = Usd.ClipsAPI(prim)
        clipsAPI.SetClipAssetPaths([Sdf.AssetPath('./clip_1.usda'), 
                                    Sdf.AssetPath('./clip_2.usda')])
        clipsAPI.SetClipActive([(0, 0), (2, 1)])

        _ValidateManifest(clipsAPI.GenerateClipManifest())
        _ValidateManifest(clipsAPI.GenerateClipManifest('default'))

    def test_ClipManifestGenerationWithMissingValues(self):
        """Tests generating a manifest using UsdClipsAPI and
        writeBlocksForClipsWithMissingValues=True"""
        def _ValidateManifest(manifest):
            def _CheckAttribute(path, expectedTimeSamples):
                self.assertEqual(
                    manifest.ListTimeSamplesForPath(path), expectedTimeSamples)

                for time in expectedTimeSamples:
                    self.assertEqual(
                        manifest.QueryTimeSample(path, time), Sdf.ValueBlock())

            # These attributes don't exist in clip 2, which is active at time
            # 2.0 so we expect to see value blocks authored at that time.
            _CheckAttribute('/Clip/A.a', [2.0])
            _CheckAttribute('/Clip/A.b', [2.0])
            _CheckAttribute('/Clip/A{v=a}.c', [2.0])

            # This attribute doesn't exist in clip 1, which is active at times
            # 0.0 and 4.0, so we expect to see value blocks authored at those
            # times.
            _CheckAttribute('/Clip/A.z', [0.0, 4.0])

        stage = Usd.Stage.Open('manifestGeneration/missingValues/root.usda')
        prim = stage.GetPrimAtPath('/Model')

        clipsAPI = Usd.ClipsAPI(prim)

        _ValidateManifest(clipsAPI.GenerateClipManifest(
            writeBlocksForClipsWithMissingValues=True))
        _ValidateManifest(clipsAPI.GenerateClipManifest(
            'default', writeBlocksForClipsWithMissingValues=True))

    def test_ClipManifestAutoGeneration(self):
        """Verifies behavior with automatic generation of clip manifest
        when no manifest is explicitly specified."""
        # Temporarily modify the first clip to test Reload functionality
        clipLayer = Sdf.Layer.FindOrOpen('manifestGeneration/clip_1.usda')
        del clipLayer.GetPrimAtPath('/Clip/A').properties['b']

        stage = Usd.Stage.Open('manifestGeneration/root.usda')
        prim = stage.GetPrimAtPath('/Model')

        def _GetManifestLayer(stage):
            def _IsManifestLayer(l):
                return l.anonymous and 'generated_manifest' in l.identifier
            layers = [l for l in stage.GetUsedLayers() if _IsManifestLayer(l)]
            self.assertEqual(len(layers), 1)
            return layers[0]

        # The prim at /Model has one active clip specified and no manifest.
        # A manifest should have automatically been generated for this
        # clip containing declarations for the attributes in the clip.
        #
        # Note that the manifest does not contain /Clip/A.d. Although it
        # is declared in clip_1.usda, it has no time samples so it's ignored.
        #
        # Note that the manifest does not contain /Clip/B. Since the clip set's
        # primPath is set to /Clip/A, no values under /Clip/B will ever be used
        # so it's ignored.
        manifestLayer = _GetManifestLayer(stage)
        self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A.a'))
        self.assertFalse(manifestLayer.GetAttributeAtPath('/Clip/A.b'))
        self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A{v=a}.c'))
        self.assertFalse(manifestLayer.GetAttributeAtPath('/Clip/A.d'))
        self.assertFalse(manifestLayer.GetPrimAtPath('/Clip/B'))

        # Reloading the stage should cause the manifest to be regenerated.
        with LayerChangeListener() as l:
            stage.Reload()

            # Note that unlike test cases below, the manifestLayer will remain
            # valid and will just be modified in place.
            self.assertTrue(clipLayer in l.changedLayers)
            self.assertTrue(manifestLayer in l.changedLayers)
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A.a'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A.b'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A{v=a}.c'))
            self.assertFalse(manifestLayer.GetPrimAtPath('/Clip/B'))

        # Now add on a new active clip. A new manifest should be created
        # containing attributes from clip_1.usda and clip_2.usda.
        with LayerChangeListener() as l:
            clipsAPI = Usd.ClipsAPI(prim)
            clipsAPI.SetClipAssetPaths([Sdf.AssetPath('./clip_1.usda'), 
                                        Sdf.AssetPath('./clip_2.usda')])
            clipsAPI.SetClipActive([(0, 0), (2, 1)])

            self.assertFalse(manifestLayer)
            manifestLayer = _GetManifestLayer(stage)

            self.assertTrue(manifestLayer in l.changedLayers)
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A.a'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A.b'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A.z'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A{v=a}.c'))
            self.assertFalse(manifestLayer.GetPrimAtPath('/Clip/B'))

        # Authoring more clip metadata causes the prim to resync, but
        # should reuse the existing manifest instead of regenerating.
        with LayerChangeListener() as l:
            clipsAPI.SetClipTimes([(0, 0), (1, 1), (2, 0), (2, 1)])

            self.assertTrue(manifestLayer)

            self.assertFalse(manifestLayer in l.changedLayers)
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A.a'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A.b'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A.z'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/A{v=a}.c'))

        # Change the clip prim path. A new manifest should be created
        # containing only attributes under the new path.
        with LayerChangeListener() as l:
            clipsAPI.SetClipPrimPath('/Clip/B')

            self.assertFalse(manifestLayer)
            manifestLayer = _GetManifestLayer(stage)

            self.assertTrue(manifestLayer in l.changedLayers)
            self.assertFalse(manifestLayer.GetPrimAtPath('/Clip/A'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/B.g'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/B.h'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/Clip/B.y'))

    def test_ClipTemplateManifestGeneration(self):
        """Tests generating a manifest using UsdClipsAPI with template-based
        value clips"""
        def _ValidateManifest(manifest):
            def _CheckAttribute(path):
                attr = manifest.GetAttributeAtPath(path)
                self.assertTrue(attr)
                self.assertIsNone(attr.default)

            _CheckAttribute('/points.extent')
            _CheckAttribute('/points.a')
            
            # This attribute exists in a clip that should not be picked up
            # by the template specification.
            self.assertFalse(manifest.GetAttributeAtPath('/points.b'))

        stage = Usd.Stage.Open('template/manifestGeneration/root.usda')
        prim = stage.GetPrimAtPath('/World/points')

        clipsAPI = Usd.ClipsAPI(prim)
        _ValidateManifest(clipsAPI.GenerateClipManifest())
        _ValidateManifest(clipsAPI.GenerateClipManifest('default'))

    def test_ClipTemplateManifestGenerationWithMissingValues(self):
        """Tests generating a manifest using UsdClipsAPI with template-based
        value clips and writeBlocksForClipsWithMissingValues=True"""
        def _ValidateManifest(manifest):
            def _CheckAttribute(path, expectedTimeSamples):
                self.assertEqual(
                    manifest.ListTimeSamplesForPath(path), expectedTimeSamples)

                for time in expectedTimeSamples:
                    self.assertEqual(
                        manifest.QueryTimeSample(path, time), Sdf.ValueBlock())

            # This attribute exists in all clips, so we expect no blocks to be
            # authored in the manifest.
            _CheckAttribute('/points.extent', [])

            # This attribute does not exist in clips 1 and 3 which are active
            # at times 1 and 3, so we expect blocks at those times.
            _CheckAttribute('/points.a', [1.0, 3.0])

            # This attribute does not exist in clips 1 and 2 which are active
            # at times 1 and 2, so we expect blocks at those times.
            _CheckAttribute('/points.b', [1.0, 2.0])

        stage = Usd.Stage.Open(
            'template/manifestGeneration/missingValues/root.usda')
        prim = stage.GetPrimAtPath('/World/points')

        clipsAPI = Usd.ClipsAPI(prim)
        _ValidateManifest(clipsAPI.GenerateClipManifest(
            writeBlocksForClipsWithMissingValues=True))
        _ValidateManifest(clipsAPI.GenerateClipManifest(
            'default', writeBlocksForClipsWithMissingValues=True))

    def test_ClipTemplateManifestAutoGeneration(self):
        """Verifies automatic generation of clip manifest for template-based
        value clip specification"""
        stage = Usd.Stage.Open('template/manifestGeneration/root.usda')
        prim = stage.GetPrimAtPath('/World/points')

        def _GetManifestLayer(stage):
            def _IsManifestLayer(l):
                return l.anonymous and 'generated_manifest' in l.identifier
            layers = [l for l in stage.GetUsedLayers() if _IsManifestLayer(l)]
            self.assertEqual(len(layers), 1)
            return layers[0]
        
        # The prim at /World/points has template-based clips specified
        # that should pick up p.001.usda and p.002.usda.
        # A manifest should have automatically been generated for this
        # clip containing declarations for the attributes in these clips.
        manifestLayer = _GetManifestLayer(stage)
        self.assertTrue(manifestLayer.GetAttributeAtPath('/points.extent'))
        self.assertTrue(manifestLayer.GetAttributeAtPath('/points.a'))
        self.assertFalse(manifestLayer.GetAttributeAtPath('/points.b'))

        # Extend the template end time to pick up an additional clip.
        # This should cause a resync and a new manifest to be generated.
        with LayerChangeListener() as l:
            clipsAPI = Usd.ClipsAPI(prim)
            clipsAPI.SetClipTemplateEndTime(3)

            self.assertFalse(manifestLayer)
            manifestLayer = _GetManifestLayer(stage)
            self.assertTrue(manifestLayer in l.changedLayers)

            self.assertTrue(manifestLayer.GetAttributeAtPath('/points.extent'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/points.a'))
            self.assertTrue(manifestLayer.GetAttributeAtPath('/points.b'))

    def test_ClipTemplateBehavior(self):
        primPath = Sdf.Path('/World/fx/Particles_Splash/points')
        attrName = 'extent'

        stage = Usd.Stage.Open('template/int1/result_int_1.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)
        self.CheckValue(attr, time=1, expected=Vt.Vec3fArray(2, (1,1,1)))
        self.CheckValue(attr, time=1.5, 
                        expected=Vt.Vec3fArray(2, (1.5,1.5,1.5)))
        self.CheckValue(attr, time=2, expected=Vt.Vec3fArray(2, (2,2,2)))
        self.CheckValue(attr, time=2.5, 
                        expected=Vt.Vec3fArray(2, (2.5,2.5,2.5)))
        self.CheckValue(attr, time=3, expected=Vt.Vec3fArray(2, (3,3,3)))
        self.CheckValue(attr, time=3.5, 
                        expected=Vt.Vec3fArray(2, (3.5,3.5,3.5)))
        self.CheckValue(attr, time=4, expected=Vt.Vec3fArray(2, (4,4,4)))

        stage = Usd.Stage.Open('template/int2/result_int_2.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)
        self.CheckValue(attr, time=1, expected=Vt.Vec3fArray(2, (1,1,1)))
        self.CheckValue(attr, time=10, expected=Vt.Vec3fArray(2, (10,10,10)))
        self.CheckValue(attr, time=17, expected=Vt.Vec3fArray(2, (17,17,17)))
        self.CheckValue(attr, time=26, expected=Vt.Vec3fArray(2, (26,26,26)))
        self.CheckValue(attr, time=33, expected=Vt.Vec3fArray(2, (33,33,33)))
        self.CheckValue(attr, time=43, expected=Vt.Vec3fArray(2, (43,43,43)))
        self.CheckValue(attr, time=49, expected=Vt.Vec3fArray(2, (49,49,49)))

        # Test with template offsets applied
        stage = Usd.Stage.Open('template/int3/result_int_3.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)
        self.CheckValue(attr, time=2.5, expected=Vt.Vec3fArray(2, (1,1,1)))
        self.CheckValue(attr, time=3.0, expected=Vt.Vec3fArray(2, (1,1,1)))
        self.CheckValue(attr, time=3.25, expected=Vt.Vec3fArray(2, (2,2,2)))
        self.CheckValue(attr, time=3.5, expected=Vt.Vec3fArray(2, (3,3,3)))
        self.CheckValue(attr, time=4.0, expected=Vt.Vec3fArray(2, (3,3,3)))
        self.CheckValue(attr, time=4.5, expected=Vt.Vec3fArray(2, (3,3,3)))

        # XXX: bug/155441 precludes us from adding the following test case
        # stage = Usd.Stage.Open('template/int4/result_int_4.usda')
        # prim = stage.GetPrimAtPath(primPath)
        # attr = prim.GetAttribute(attrName)
        # self.CheckValue(attr, time=0, expected=Vt.Vec3fArray(2, (0,0,0)))
        # self.CheckValue(attr, time=1, expected=Vt.Vec3fArray(2, (1,1,1)))
        # self.CheckValue(attr, time=3.5, expected=Vt.Vec3fArray(2, (3,3,3)))
        # self.CheckValue(attr, time=4.0, expected=Vt.Vec3fArray(2, (4,4,4)))

        stage = Usd.Stage.Open('template/subint1/result_subint_1.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)
        self.CheckValue(attr, time=101, expected=Vt.Vec3fArray(2, (101,101,101)))
        self.CheckValue(attr, time=101.5, 
                        expected=Vt.Vec3fArray(2, (101.5,101.5,101.5)))
        self.CheckValue(attr, time=102, expected=Vt.Vec3fArray(2, (102,102,102)))
        self.CheckValue(attr, time=102.5, 
                        expected=Vt.Vec3fArray(2, (102.5,102.5,102.5)))
        self.CheckValue(attr, time=103, expected=Vt.Vec3fArray(2, (103,103,103)))
        self.CheckValue(attr, time=103.5, 
                        expected=Vt.Vec3fArray(2, (103.5,103.5,103.5)))
        self.CheckValue(attr, time=104, expected=Vt.Vec3fArray(2, (104,104,104)))

        stage = Usd.Stage.Open('template/subint2/result_subint_2.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)
        self.CheckValue(attr, time=10.00, 
                        expected=Vt.Vec3fArray(2, (10.00, 10.00, 10.00)))
        self.CheckValue(attr, time=10.025, 
                        expected=Vt.Vec3fArray(2, (10.025, 10.025, 10.025)))
        self.CheckValue(attr, time=10.05, 
                        expected=Vt.Vec3fArray(2, (10.05, 10.05, 10.05)))
        self.CheckValue(attr, time=10.08, 
                       expected=Vt.Vec3fArray(2, (10.08, 10.08, 10.08)))
        self.CheckValue(attr, time=10.10, 
                        expected=Vt.Vec3fArray(2, (10.10, 10.10, 10.10)))
        self.CheckValue(attr, time=10.125, 
                        expected=Vt.Vec3fArray(2, (10.125, 10.125, 10.125)))
        self.CheckValue(attr, time=10.15, 
                        expected=Vt.Vec3fArray(2, (10.15, 10.15, 10.15)))

        # Test with template offsets applied
        stage = Usd.Stage.Open('template/subint3/result_subint_3.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)

        self.CheckValue(attr, time=9.95,  expected=Vt.Vec3fArray(2, (10, 10, 10)))
        self.CheckValue(attr, time=10.00, expected=Vt.Vec3fArray(2, (10, 10, 10)))
        self.CheckValue(attr, time=10.025, 
                        expected=Vt.Vec3fArray(2, (10.05, 10.05, 10.05)))
        self.CheckValue(attr, time=10.05, expected=Vt.Vec3fArray(2, (10.1, 10.1, 10.1)))
        self.CheckValue(attr, time=10.10, expected=Vt.Vec3fArray(2, (10.1, 10.1, 10.1)))
        self.CheckValue(attr, time=10.15, expected=Vt.Vec3fArray(2, (10.1, 10.1, 10.1)))
        self.CheckValue(attr, time=10.20, expected=Vt.Vec3fArray(2, (10.1, 10.1, 10.1)))
        self.CheckValue(attr, time=10.25, expected=Vt.Vec3fArray(2, (10.1, 10.1, 10.1)))

    def test_ClipTemplateWithOffsets(self):
        stage = Usd.Stage.Open('template/layerOffsets/root.usda')
        prim = stage.GetPrimAtPath('/Model')
        attr = prim.GetAttribute('a')

        # Times are offset by 2 via reference and layer offsets,
        # so we expect the value at time 4 to read from clip 2, etc.
        self.CheckValue(attr, time=3.0, expected=1.0)
        self.CheckValue(attr, time=4.0, expected=2.0)
        self.CheckValue(attr, time=5.0, expected=3.0)

        # Because of the time offset, this should try to read clip 4,
        # but since we only have 3 clips we hold the value from the
        # last one.
        self.CheckValue(attr, time=6.0, expected=3.0)

    def test_ClipsWithSparseOverrides(self):
        # This layer overrides the clipActive metadata to flip
        # the active clips
        stage = Usd.Stage.Open('sparseOverrides/over_root.usda')
        prim = stage.GetPrimAtPath('/main')
        attr = prim.GetAttribute('foo')

        self.CheckValue(attr,  time=101.0, expected=3.0)
        self.CheckValue(attr,  time=103.0, expected=1.0)

        # This is the original layer with the clip metadata authored.
        stage = Usd.Stage.Open('sparseOverrides/root.usda')
        prim = stage.GetPrimAtPath('/main')
        attr = prim.GetAttribute('foo')

        self.CheckValue(attr,  time=101.0, expected=1.0)
        self.CheckValue(attr,  time=103.0, expected=3.0)

        # This layer overrides the startTime from the template metadata
        # to be equal to the endTime, effectively giving us only one clip
        stage = Usd.Stage.Open('sparseOverrides/template_over_root.usda')
        prim = stage.GetPrimAtPath('/main')
        attr = prim.GetAttribute('foo')

        self.CheckValue(attr,  time=101.0, expected=3.0)
        self.CheckValue(attr,  time=103.0, expected=3.0)

        # This is the original layer with the template metadata authored. 
        stage = Usd.Stage.Open('sparseOverrides/template_root.usda')
        prim = stage.GetPrimAtPath('/main')
        attr = prim.GetAttribute('foo')

        self.CheckValue(attr,  time=101.0, expected=1.0)
        self.CheckValue(attr,  time=103.0, expected=3.0)

    def test_MultipleClipSets(self):
        """Verifies behavior with multiple clip sets defined on
        the same prim that affect different prims"""
        stage = Usd.Stage.Open('clipsets/root.usda')

        prim = stage.GetPrimAtPath('/Set/Child_1')
        attr = prim.GetAttribute('attr')
        self.CheckValue(attr, time=0, expected=-5.0)
        self.CheckValue(attr, time=1, expected=-10.0)
        self.CheckValue(attr, time=2, expected=-15.0)
        self.CheckTimeSamples(attr)

        prim = stage.GetPrimAtPath('/Set/Child_2')
        attr = prim.GetAttribute('attr')
        self.CheckValue(attr, time=0, expected=-50.0)
        self.CheckValue(attr, time=1, expected=-100.0)
        self.CheckValue(attr, time=2, expected=-200.0)
        self.CheckTimeSamples(attr)

    def test_ListEditClipSets(self):
        """Verifies reordering and deleting clip sets via list editing
        operations"""
        stage = Usd.Stage.Open('clipsetListEdits/root.usda')

        prim = stage.GetPrimAtPath('/DefaultOrderTest')
        attr = prim.GetAttribute('attr')
        self.CheckValue(attr, time=0, expected=10.0)
        self.CheckValue(attr, time=1, expected=20.0)
        self.CheckValue(attr, time=2, expected=30.0)
        self.CheckTimeSamples(attr)

        prim = stage.GetPrimAtPath('/ReorderTest')
        attr = prim.GetAttribute('attr')
        self.CheckValue(attr, time=0, expected=100.0)
        self.CheckValue(attr, time=1, expected=200.0)
        self.CheckValue(attr, time=2, expected=300.0)
        self.CheckTimeSamples(attr)

        prim = stage.GetPrimAtPath('/DeleteTest')
        attr = prim.GetAttribute('attr')
        self.CheckValue(attr, time=0, expected=100.0)
        self.CheckValue(attr, time=1, expected=200.0)
        self.CheckValue(attr, time=2, expected=300.0)
        self.CheckTimeSamples(attr)

    def test_InterpolateSamplesInClip(self):
        """Tests that time samples in clips are interpolated
        when a clip time is specified and no sample exists in
        the clip at that time."""
        stage = Usd.Stage.Open('interpolation/root.usda')

        prim = stage.GetPrimAtPath('/InterpolationTest')
        attr = prim.GetAttribute('attr')
        self.CheckValue(attr, time=0, expected=0.0)
        self.CheckValue(attr, time=1, expected=5.0)
        self.CheckValue(attr, time=2, expected=10.0)
        self.CheckValue(attr, time=3, expected=15.0)
        self.CheckValue(attr, time=4, expected=20.0)
        self.CheckTimeSamples(attr)

    def test_InterpolateSamplesToNextClip(self):
        """Tests that time samples in clips are interpolated using the
        value from the next clip if necessary."""
        stage = Usd.Stage.Open('interpolation/root.usda')

        attr = stage.GetAttributeAtPath('/ClipInterpolationTest.attr')
        self.CheckValue(attr, time=0, expected=10.0)
        self.CheckValue(attr, time=1, expected=20.0)

        # At t=0.5, we're beyond the last (and only) time sample in the first
        # clip that is inside the clip's active time. We should ignore the
        # time sample authored at t=1.0 in the first clip (since that's outside
        # the active time for that clip) and interpolate using the time sample
        # in the next clip at t=1.0
        self.CheckValue(attr, time=0.5, expected=15.0)

        self.assertEqual(attr.GetTimeSamples(), [0.0, 1.0, 3.0])
        self.CheckTimeSamples(attr)

    def test_InterpolateSamplesToNextClip2(self):
        """Tests that additional entries in the times metadata can be used
        to 'block' interpolating values using the next clip."""
        stage = Usd.Stage.Open('interpolation/root.usda')

        attr = stage.GetAttributeAtPath('/ClipInterpolationTest2.attr')
        self.CheckValue(attr, time=0, expected=10.0)
        # We are at clip boundary at time=1, so we look at clip1 for pre-time(1)
        # with a jump discontinuity @1, so first time mapping will be used @1.
        self.CheckValue(attr, time=Usd.TimeCode.PreTime(1), 
                        expected=100.0)
        self.CheckValue(attr, time=1, expected=20.0)

        # At t=0.5, we're beyond the last (and only) time sample in the first
        # clip that is inside the clip's active time. In this case we do *not*
        # ignore the time sample authored at t=1.0 in the first clip because
        # we have an entry in the times metadata that explicitly says to use
        # the value from the first clip at t=1.0 when interpolating values
        # up to the end of the clip.
        self.CheckValue(attr, time=0.5, expected=55.0)

        self.assertEqual(attr.GetTimeSamples(), 
                         [0.0, 1.0 - Usd.TimeCode.SafeStep(), 1.0, 3.0])
        self.CheckTimeSamples(attr)

    def test_AssetPathValuesInClips(self):
        """Tests that asset path values in clips are resolved
        properly."""
        stage = Usd.Stage.Open('assetPathValues/root.usda')

        def _CheckPaths(p1, p2):
            self.assertEqual(os.path.normcase(p1), os.path.normcase(p2))

        def _CheckAssetPathValue(attr, time, expected):
            _CheckPaths(attr.Get(time).resolvedPath, expected)
            _CheckPaths(Usd.AttributeQuery(attr).Get(time).resolvedPath,
                        expected)

        def _CheckAssetPathArrayValue(attr, time, expected):
            array = attr.Get(time)
            self.assertEqual(len(array), len(expected))
            for (p1, p2) in zip(array, expected):
                _CheckPaths(p1.resolvedPath, p2)

        # Test that relative asset paths from clips are anchored to the
        # clip layer. Note that at time 1 we have a clip with no samples
        # so we should get the default value defined in the manifest;
        # the resolved path there should be anchored to the manifest layer.
        #
        # The stage variable expressions authored in the asset paths in
        # clip3.usda are evaluated using the variables authored in the
        # stage's root and session layer. Variables in the clip itself
        # are currently ignored.

        attr = stage.GetAttributeAtPath('/Model.assetPath')
        _CheckAssetPathValue(
            attr, time=Usd.TimeCode.PreTime(0),
            expected=os.path.abspath('assetPathValues/clip1/clip1.usda'))
        _CheckAssetPathValue(
            attr, time=0, 
            expected=os.path.abspath('assetPathValues/clip1/clip1.usda'))
        _CheckAssetPathValue(
            attr, time=Usd.TimeCode.PreTime(1), 
            expected=os.path.abspath('assetPathValues/clip1/clip1.usda'))
        _CheckAssetPathValue(
            attr, time=1, 
            expected=os.path.abspath('assetPathValues/manifest/manifest.usda'))
        _CheckAssetPathValue(
            attr, time=Usd.TimeCode.PreTime(2),
            expected=os.path.abspath('assetPathValues/manifest/manifest.usda'))
        _CheckAssetPathValue(
            attr, time=2,
            expected=os.path.abspath('assetPathValues/clip2/clip2.usda'))
        _CheckAssetPathValue(
            attr, time=Usd.TimeCode.PreTime(3),
            expected=os.path.abspath('assetPathValues/clip2/clip2.usda'))
        _CheckAssetPathValue(
            attr, time=3,
            expected=os.path.abspath('assetPathValues/clip3/clip3.usda'))
        _CheckAssetPathValue(
            attr, time=Usd.TimeCode.PreTime(4),
            expected=os.path.abspath('assetPathValues/clip3/clip3.usda'))

        attr = stage.GetAttributeAtPath('/Model.assetPathArray')
        _CheckAssetPathArrayValue(
            attr, time=Usd.TimeCode.PreTime(0),
            expected=[os.path.abspath('assetPathValues/clip1/clip1.usda')])
        _CheckAssetPathArrayValue(
            attr, time=0, 
            expected=[os.path.abspath('assetPathValues/clip1/clip1.usda')])
        _CheckAssetPathArrayValue(
            attr, time=Usd.TimeCode.PreTime(1),
            expected=[os.path.abspath('assetPathValues/clip1/clip1.usda')])
        _CheckAssetPathArrayValue(
            attr, time=1, 
            expected=[os.path.abspath('assetPathValues/manifest/manifest.usda')])
        _CheckAssetPathArrayValue(
            attr, time=Usd.TimeCode.PreTime(2),
            expected=[os.path.abspath('assetPathValues/manifest/manifest.usda')])
        _CheckAssetPathArrayValue(
            attr, time=2,
            expected=[os.path.abspath('assetPathValues/clip2/clip2.usda')])
        _CheckAssetPathArrayValue(
            attr, time=Usd.TimeCode.PreTime(3),
            expected=[os.path.abspath('assetPathValues/clip2/clip2.usda')])
        _CheckAssetPathArrayValue(
            attr, time=3,
            expected=[os.path.abspath('assetPathValues/clip3/clip3.usda')])
        _CheckAssetPathArrayValue(
            attr, time=Usd.TimeCode.PreTime(4),
            expected=[os.path.abspath('assetPathValues/clip3/clip3.usda')])

    def test_ComputeClipAssetPaths(self):
        """Test Usd.ClipsAPI.ComputeClipAssetPaths"""
        def _CheckPaths(p1, p2):
            self.assertEqual(os.path.normcase(p1), os.path.normcase(p2))

        def _CheckAssetPathArrays(array, expected):
            self.assertEqual(len(array), len(expected))
            for (p1, p2) in zip(array, expected):
                _CheckPaths(p1, p2)

        stage = Usd.Stage.Open('assetPathValues/root.usda')
        clipsAPI = Usd.ClipsAPI(stage.GetPrimAtPath('/Model'))
        computedAssetPaths = clipsAPI.ComputeClipAssetPaths()
        _CheckAssetPathArrays(
            [p.resolvedPath for p in computedAssetPaths],
            [os.path.abspath('assetPathValues/clip1/clip1.usda'),
             os.path.abspath('assetPathValues/nosamples.usda'),
             os.path.abspath('assetPathValues/clip2/clip2.usda'),
             os.path.abspath('assetPathValues/clip3/clip3.usda')])

        stage = Usd.Stage.Open('template/int1/result_int_1.usda')
        clipsAPI = Usd.ClipsAPI(
            stage.GetPrimAtPath('/World/fx/Particles_Splash/points'))
        computedAssetPaths = clipsAPI.ComputeClipAssetPaths()
        _CheckAssetPathArrays(
            [p.resolvedPath for p in computedAssetPaths],
            [os.path.abspath('template/int1/p.001.usd'),
             os.path.abspath('template/int1/p.002.usd'),
             os.path.abspath('template/int1/p.003.usd'),
             os.path.abspath('template/int1/p.004.usd')])

    def test_TemplateFileFormatArguments(self):
        stage = Usd.Stage.Open('template/args/root.usda')
        prim = stage.GetPrimAtPath('/World/points')
        attr = prim.GetAttribute('extent')

        self.CheckValue(attr, time=1, expected=Vt.Vec3fArray(2, (1,1,1)))
        self.CheckValue(attr, time=2, expected=Vt.Vec3fArray(2, (2,2,2)))

        layerId = Sdf.Layer.CreateIdentifier(
                os.path.abspath("template/args/p.002.usd"), 
                {'a': '1', 'b': 'str'})
        layer = Sdf.Layer.Find(layerId)
        self.assertTrue(layer)
        self.assertEqual(layer.GetFileFormatArguments(), {'a': '1', 'b': 'str'})

    def test_SublayerChanges(self):
        """Test that clip layers are loaded successfully when sublayers
        are added or removed before the clip layers are pulled on."""

        def _test(stage):
            # Query our test attribute's property stack and verify that it
            # contains the opinions we expect. This will open the clip layer.
            a = stage.GetAttributeAtPath('/SingleClip.attr_1')
            propertyStack = a.GetPropertyStack(0)

            rootLayer = stage.GetRootLayer()
            sublayerWithClip = Sdf.Layer.FindRelativeToLayer(
                rootLayer, 'layers/sublayer.usda')
            self.assertTrue(sublayerWithClip)

            clipLayer = Sdf.Layer.FindRelativeToLayer(
                sublayerWithClip, 'clip.usda')
            self.assertTrue(clipLayer)

            refLayer = Sdf.Layer.FindRelativeToLayer(
                rootLayer, 'layers/ref.usda')
            self.assertTrue(refLayer)

            self.assertEqual(
                propertyStack,
                [sublayerWithClip.GetAttributeAtPath('/SingleClip.attr_1'),
                 clipLayer.GetAttributeAtPath('/Model.attr_1'),
                 refLayer.GetAttributeAtPath('/Model.attr_1')])

        # Test combinations of inserting and removing sublayers prior to
        # pulling on attributes and opening clip layers. Clip layers are
        # opened the first time the _test function is called, so these
        # tests are separated into insert-first and remove-first to cover
        # both cases. Empty and non-empty sublayers are also tested 
        # separately since there's an optimization that avoids significant
        # resyncs in the former case.

        def _TestInsertAndRemoveEmptySublayer():
            dummySublayer = Sdf.Layer.CreateAnonymous('.usda')
            rootLayer = Sdf.Layer.FindOrOpen('sublayerChanges/root.usda')

            stage = Usd.Stage.Open(rootLayer)
            rootLayer.subLayerPaths.insert(0, dummySublayer.identifier)
            _test(stage)

            del rootLayer.subLayerPaths[0]
            _test(stage)

        def _TestRemoveAndInsertEmptySublayer():
            dummySublayer = Sdf.Layer.CreateAnonymous('.usda')

            rootLayer = Sdf.Layer.FindOrOpen('sublayerChanges/root.usda')
            rootLayer.subLayerPaths.insert(0, dummySublayer.identifier)

            stage = Usd.Stage.Open(rootLayer)
            del rootLayer.subLayerPaths[0]
            _test(stage)

            rootLayer.subLayerPaths.insert(0, dummySublayer.identifier)
            _test(stage)

        def _TestInsertAndRemoveNonEmptySublayer():
            dummySublayer = Sdf.Layer.CreateAnonymous('.usda')
            Sdf.CreatePrimInLayer(dummySublayer, '/Dummy')

            rootLayer = Sdf.Layer.FindOrOpen('sublayerChanges/root.usda')

            stage = Usd.Stage.Open(rootLayer)
            rootLayer.subLayerPaths.insert(0, dummySublayer.identifier)
            _test(stage)

            del rootLayer.subLayerPaths[0]
            _test(stage)

        def _TestRemoveAndInsertNonEmptySublayer():
            dummySublayer = Sdf.Layer.CreateAnonymous('.usda')
            Sdf.CreatePrimInLayer(dummySublayer, '/Dummy')

            rootLayer = Sdf.Layer.FindOrOpen('sublayerChanges/root.usda')
            rootLayer.subLayerPaths.insert(0, dummySublayer.identifier)

            stage = Usd.Stage.Open(rootLayer)
            del rootLayer.subLayerPaths[0]
            _test(stage)

            rootLayer.subLayerPaths.insert(0, dummySublayer.identifier)
            _test(stage)
            
        _TestInsertAndRemoveNonEmptySublayer()
        _TestRemoveAndInsertNonEmptySublayer()
        _TestInsertAndRemoveEmptySublayer()
        _TestRemoveAndInsertEmptySublayer()

    def test_ExpectedAttributeFormat(self):
        """Test syntax that results in "spline vs time samples" being expected
        in cases when manifests are "explicitly authored" or
        "generated at runtime".
        """
        stage = Usd.Stage.Open("dataFormat/root.usda")
        model = stage.GetPrimAtPath("/Model")

        def _CheckTimeSamples(attrName, expectedNumSamples):
            """Checks the following expected characteristics for attrs that
            resolve to time samples based on the manifest.
            """
            attr = model.GetAttribute(attrName)
            if (expectedNumSamples > 1):
                self.assertTrue(attr.ValueMightBeTimeVarying())
            else:
                self.assertFalse(attr.ValueMightBeTimeVarying())

            self.assertFalse(attr.HasSpline())
            self.assertIsNone(attr.Get())
            self.assertEqual(len(attr.GetTimeSamples()), expectedNumSamples)
            self.CheckTimeSamples(attr)
        
        def _CheckSpline(attrName, expectedNumKnots):
            """Checks the following expected characteristics for attrs
            that resolve to spline based on the manifest.
            """
            attr = model.GetAttribute(attrName)
            self.assertTrue(attr.HasSpline())
            self.assertTrue(attr.ValueMightBeTimeVarying())
            self.assertIsNone(attr.Get())
            spline = attr.GetSpline()
            self.assertEqual(len(spline.GetKnots()), expectedNumKnots)
            self.CheckSpline(attr)

        def _TestAuthoredManifest():
            """Data format should be determined purely from the manifest;
            data authored in clips shouldn't be taken into account.

            Choose "time samples" when an attribute in an authored
            manifest doesn't author explicit "spline" or "time samples" syntax.

            Note that UsdAttribute time sample getters insert a sample
            at the starting active time of each clip if a sample is not already
            present. Splines' knot counts are potentially more complicated and
            documented in each case.
            """
            _CheckTimeSamples("aWithDefault", 1)
            _CheckTimeSamples("aDeclared", 1)
            _CheckTimeSamples("aTimeSamples", 1)
            # Defined time samples are ignored in the manifest, but still
            # resolves to time samples
            _CheckTimeSamples("aDefinedTimeSamples", 1)
            # Time samples "wins" over spline declaration when both are present
            _CheckTimeSamples("aBothFormats", 1)
            # Time samples with value blocks resolves to time samples
            _CheckTimeSamples("aTimeSamplesSkipClipsAtTimes", 1)
            # An additional sample is inserted at the clip start time if
            # it doesn't already exist.
            _CheckTimeSamples("aTimeSamplesNonEmptyClip", 2)
            # The first clip's time samples are preserved in the interval
            # (-inf, first clip's end time). This is also true for the last
            # last clip and the interval [last clip's start time, +inf) 
            _CheckTimeSamples("aTimeSamplesPreActiveTime", 3)

            # One knot is inserted at the clip active time because the clip
            # doesn't have an authored spline.
            _CheckSpline("aSpline", 1)
            # If there is a default, a knot is inserted at the beginning of
            # each contiguous clip (potentially multiple-clip) region.
            _CheckSpline("aSplineWithDefault", 1)
            _CheckSpline("aSplineSkipClipsAtTimes", 1)
            _CheckSpline("aSplineNonEmptyClip", 1)
            # Analogue to "aTimeSamplesPreActiveTime"
            _CheckSpline("aSplinePreActiveTime", 3)

        def _TestGeneratedManifest():
            """Data format is determined by parsing clip files.
            Time samples "win" over splines if both are specified. Note that
            empty time samples in clips *do not* cause resolution to time
            samples, whereas empty splines do cause resolution to splines.
            """
            # Empty time samples in clips don't cause the overall attribute to
            # register time samples.
            _CheckTimeSamples("gEmptyTimeSamples", 0)
            _CheckTimeSamples("gDefinedTimeSamples", 3)
            # Defined timeSamples wins over defined spline (intra-clip)
            _CheckTimeSamples("gBothFormats", 2)
            # Defined timeSamples wins over defined spline (inter-clip)
            _CheckTimeSamples("gBothFormatsAcrossClips", 2)
            # Time samples that fall outside the active range don't directly
            # contribute samples, but still cause clip value resolution to
            # register the whole attribute as time samples.
            _CheckTimeSamples("gTimeSamplesOutsideClipRange", 2)

            # For the below splines, knots at the active time for the clip
            # without values is generated to convey a "value block" because
            # interpolateMissingClipValues=false

            # Unlike time samples, an empty spline in clips is meaningful.
            _CheckSpline("gSpline", 2)
            # gDefinedSpline in the first clip contributes one knot.
            _CheckSpline("gDefinedSpline", 2)
            _CheckSpline("gSplineAcrossClips", 2)
            # Splines win when time samples are empty (intra-clip)
            _CheckSpline("gBothFormatsSplineWins", 1)
            # Splines win when time samples are empty (inter-clip)
            _CheckSpline("gBothFormatsSplineWinsAcrossClips", 1)

        _TestAuthoredManifest()
        _TestGeneratedManifest()

    def test_ClipSplineTiming(self):
        """Exercises clip retiming of splines via clipTimes metadata.

        On a single clip, this checks:
        - Offsetting clip times
        - Spline behavior at jump discontinuities
        - Slowed clip sections relative to stage time
        - Sped-up clip sections relative to stage time
        - Reversed clip sections
        """
        stage = Usd.Stage.Open("timingSpline/root.usda")
        model = stage.GetPrimAtPath("/Model")
        attr1 = model.GetAttribute('a')
        self.CheckSpline(attr1)

        # Test that splines are held outside of the clips time range
        self.CheckValue(attr1, time=-5, expected=10)
        self.CheckValue(attr1, time=0, expected=10)
        # The expected value is 15 because there are knots in clip
        # time at 12(value 12) and 20(value 18). The section is curved
        # but exactly halfway at clip time 16, value is 15. This
        # clip time is where the entire clip times metadata ends.
        self.CheckValueClose(attr1, time=29.5, expected=15)
        self.CheckValueClose(attr1, time=100, expected=15)

        # Check jump discontinuity behavior
        self.CheckValueClose(attr1, time=Usd.TimeCode.PreTime(10),
                             expected=18)

        # Check that the spline is stretched over ext time [10, 20)
        self.CheckValue(attr1, time=10, expected=10)
        self.CheckValue(attr1, time=12, expected=11)
        self.CheckValue(attr1, time=14, expected=12)

        # Check that the spline is shrunk over ext time [20, 25)
        self.CheckValueClose(attr1, time=20.5, expected=15)
        self.CheckValueClose(attr1, time=Usd.TimeCode.PreTime(22.5),
                             expected=18)
        self.CheckValueClose(attr1, time=22.5, expected=20)
        self.CheckValueClose(attr1, time=25, expected=30)

        # Check reversed section. Note that the expected preTime/time
        # queries at time 27.5 are the reverse of those above at 22.5
        self.CheckValueClose(attr1, time=25, expected=30)
        self.CheckValueClose(attr1, time=Usd.TimeCode.PreTime(27.5),
                             expected=20)
        self.CheckValueClose(attr1, time=27.5, expected=18)
        self.CheckValueClose(attr1, time=29.5, expected=15)

        model2 = stage.GetPrimAtPath("/Model2")
        attr2 = model2.GetAttribute("attr2")
        self.CheckSpline(attr2)
        self.CheckValue(attr2, time=10, expected=9)

    def test_MultipleSplinesInClipsMissing(self):
        """Test expected fallback behavior when clips are missing
           in the following scenarios:
            - attr has no manifest defaults
            - attr has manifest defaults
            - attr has no manifest defaults and
                interpolateMissingClipValues=true
        """
        stage = Usd.Stage.Open("multiclipSpline/root.usda")

        def _TestMissing(self, primPath, manifestDefault):
            model = stage.GetPrimAtPath(primPath)
            fallback = manifestDefault

            missingAll = model.GetAttribute("missingAll")
            self.CheckSpline(missingAll)
            self.CheckValue(missingAll, time=-1, expected=fallback)
            self.CheckValue(missingAll, time=2.5, expected=fallback)
            self.CheckValue(missingAll, time=4, expected=fallback)

            missingFirst = model.GetAttribute("missingFirst")
            self.CheckSpline(missingFirst)
            self.CheckValue(missingFirst, time=-1, expected=fallback)
            self.CheckValue(missingFirst, time=0, expected=fallback)
            self.CheckValue(missingFirst, time=1, expected=1.0)
            self.CheckValue(missingFirst, time=4, expected=3.0)

            missingMiddle = model.GetAttribute("missingMiddle")
            self.CheckSpline(missingMiddle)
            self.CheckValue(missingMiddle, time=-1, expected=0.0)
            self.CheckValue(missingMiddle, time=0, expected=0.0)
            self.CheckValue(missingMiddle, time=1, expected=fallback)
            self.CheckValue(missingMiddle, time=Usd.TimeCode.PreTime(2),
                            expected=fallback)
            self.CheckValue(missingMiddle, time=2, expected=2.0)

            missingLast = model.GetAttribute("missingLast")
            self.CheckSpline(missingLast)
            self.CheckValue(missingLast, time=-1, expected=0.0)
            self.CheckValue(missingLast, time=Usd.TimeCode.PreTime(3),
                            expected=2.0)
            self.CheckValue(missingLast, time=3, expected=fallback)
            self.CheckValue(missingLast, time=4, expected=fallback)

            missingMiddle2 = model.GetAttribute("missingMiddle2")
            self.CheckSpline(missingMiddle2)
            self.CheckValue(missingMiddle2, time=Usd.TimeCode.PreTime(1),
                            expected=0.0)
            self.CheckValue(missingMiddle2, time=1, expected=fallback)
            self.CheckValue(missingMiddle2, time=2.5, expected=fallback)
            self.CheckValue(missingMiddle2, time=3, expected=3.0)

            missingLast2 = model.GetAttribute("missingLast2")
            self.CheckSpline(missingLast2)
            self.CheckValue(missingLast2, time=Usd.TimeCode.PreTime(2),
                            expected=1.0)
            self.CheckValue(missingLast2, time=2, expected=fallback)
            self.CheckValue(missingLast2, time=3, expected=fallback)
            self.CheckValue(missingLast2, time=4, expected=fallback)

            missingFirst2 = model.GetAttribute("missingFirst2")
            self.CheckSpline(missingFirst2)
            self.CheckValue(missingFirst2, time=-1, expected=fallback)
            self.CheckValue(missingFirst2, time=0, expected=fallback)
            self.CheckValue(missingFirst2, time=Usd.TimeCode.PreTime(2),
                            expected=fallback)
            self.CheckValue(missingFirst2, time=2, expected=2.0)

        def _TestMissingWithInterpolation(self):
            """Tests interpolation over missing splines in clips assuming
               default stage linear interpolation"""
            model = stage.GetPrimAtPath(
                "/ModelMissingWithNoManifestDefaultsWithInterpolation")

            missingAll = model.GetAttribute("missingAll")
            self.CheckSpline(missingAll)
            self.CheckValue(missingAll, time=-1, expected=None)
            self.CheckValue(missingAll, time=2.5, expected=None)
            self.CheckValue(missingAll, time=4, expected=None)

            # missing_clip1's value for missingFirst is held back infinitely
            missingFirst = model.GetAttribute("missingFirst")
            self.CheckSpline(missingFirst)
            self.CheckValue(missingFirst, time=-1, expected=1.0)
            self.CheckValue(missingFirst, time=0, expected=1.0)
            self.CheckValue(missingFirst, time=1, expected=1.0)
            self.CheckValue(missingFirst, time=4, expected=3.0)

            # Interpolate between missingMiddle values in clip0 and clip2
            missingMiddle = model.GetAttribute("missingMiddle")
            self.CheckSpline(missingMiddle)
            self.CheckValue(missingMiddle, time=-1, expected=0.0)
            self.CheckValue(missingMiddle, time=0, expected=0.0)
            self.CheckValue(missingMiddle, time=1, expected=0.0)
            self.CheckValue(missingMiddle, time=1.5, expected=1.0)
            self.CheckValue(missingMiddle, time=1.75, expected=1.5)
            self.CheckValue(missingMiddle, time=Usd.TimeCode.PreTime(2),
                            expected=2.0)
            self.CheckValue(missingMiddle, time=2, expected=2.0)

            # Hold forward the knot at time 2 through the last missing clip;
            # when only one side of the missing clip region to be interpolated
            # contributes a value, that value is held through the missing
            # clip region.
            missingLast = model.GetAttribute("missingLast")
            self.CheckSpline(missingLast)
            self.CheckValue(missingLast, time=-1, expected=0.0)
            self.CheckValue(missingLast, time=Usd.TimeCode.PreTime(3),
                            expected=2.0)
            self.CheckValue(missingLast, time=3, expected=2.0)
            self.CheckValue(missingLast, time=4, expected=2.0)

            # Interpolate between missingMiddle2 values in clip0 and clip3
            missingMiddle2 = model.GetAttribute("missingMiddle2")
            self.CheckSpline(missingMiddle2)
            self.CheckValue(missingMiddle2, time=Usd.TimeCode.PreTime(1),
                            expected=0.0)
            self.CheckValue(missingMiddle2, time=1, expected=0.0)
            self.CheckValue(missingMiddle2, time=2, expected=1.5)
            self.CheckValue(missingMiddle2, time=2.5, expected=2.25)
            self.CheckValue(missingMiddle2, time=3, expected=3.0)

            # Hold forward the knot at time 1 through the last missing clip;
            # when only one side of the missing clip region to be interpolated
            # contributes a value, that value is held through the missing
            # clip region.
            missingLast2 = model.GetAttribute("missingLast2")
            self.CheckSpline(missingLast2)
            self.CheckValue(missingLast2, time=Usd.TimeCode.PreTime(2),
                            expected=1.0)
            self.CheckValue(missingLast2, time=2, expected=1.0)
            self.CheckValue(missingLast2, time=3, expected=1.0)
            self.CheckValue(missingLast2, time=4, expected=1.0)

            # Hold backward the knot at time 2
            missingFirst2 = model.GetAttribute("missingFirst2")
            self.CheckSpline(missingFirst2)
            self.CheckValue(missingFirst2, time=-1, expected=2.0)
            self.CheckValue(missingFirst2, time=0, expected=2.0)
            self.CheckValue(missingFirst2, time=Usd.TimeCode.PreTime(2),
                            expected=2.0)
            self.CheckValue(missingFirst2, time=2, expected=2.0)

        _TestMissing(self, "/ModelMissingWithNoManifestDefaults", None)

        # Note that manifest defaults are stronger than interpolation,
        # so these two tests expect the same results.
        _TestMissing(self, "/ModelMissingWithManifestDefaults", 10)
        _TestMissing(self,
            "/ModelMissingWithManifestDefaultsWithInterpolation", 10)

        _TestMissingWithInterpolation(self)

    def test_MultipleSplinesInClipsWithNoTimes(self):
        """Test sequencing multiple clips together with no times metadata
        to remap times."""
        stage = Usd.Stage.Open("multiclipSpline/root.usda")
        model = stage.GetPrimAtPath("/ModelWithNoTimes")

        attr1 = model.GetAttribute("attrDualValuedBoundary")
        self.CheckSpline(attr1)
        self.CheckValue(attr1, time=Usd.TimeCode.PreTime(0), expected=None)
        self.CheckValue(attr1, time=0, expected=0)
        self.CheckValue(attr1, time=5, expected=0.5)
        self.CheckValue(attr1, time=Usd.TimeCode.PreTime(10), expected=1)
        # clip boundary here
        self.CheckValueClose(attr1, time=10, expected=10)
        self.CheckValue(attr1, time=Usd.TimeCode.PreTime(15), expected=15)
        self.CheckValue(attr1, time=15, expected=None)
        self.CheckValue(attr1, time=Usd.TimeCode.PreTime(20), expected=None)
        # clip boundary here
        self.CheckValue(attr1, time=20, expected=20)
        self.CheckValue(attr1, time=25, expected=25)
        self.CheckValue(attr1, time=30, expected=30)

        attr2 = model.GetAttribute("attrTruncatedLooping")
        self.CheckSpline(attr2)
        self.assertEqual(attr2.GetSpline().GetPreExtrapolation().mode,
                         Ts.ExtrapLoopRepeat)
        self.CheckValue(attr2, time=-10, expected=5)
        self.CheckValue(attr2, time=-5, expected=10)
        self.CheckValue(attr2, time=0, expected=5)
        self.CheckValue(attr2, time=5, expected=10)
        self.CheckValue(attr2, time=Usd.TimeCode.PreTime(10), expected=5)
        # clip boundary here
        self.CheckValue(attr2, time=10, expected=0)
        self.CheckValue(attr2, time=Usd.TimeCode.PreTime(15), expected=5)
        self.CheckValue(attr2, time=15, expected=0)
        self.CheckValue(attr2, time=Usd.TimeCode.PreTime(20), expected=5)
        # clip boundary here
        self.CheckValue(attr2, time=20, expected=20)
        self.CheckValue(attr2, time=Usd.TimeCode.PreTime(25), expected=15)
        self.CheckValue(attr2, time=25, expected=15)
        self.CheckValue(attr2, time=30, expected=20)

    def test_MultipleSplinesInClipsWithTimesSpanningClips(self):
        """Tests that clip time mappings that span multiple splines in clips
        work as expected"""
        stage = Usd.Stage.Open("multiclipSpline/root.usda")
        model = stage.GetPrimAtPath("/ModelWithTimesSpanningClips")

        attr1 = model.GetAttribute("attr1")
        self.CheckSpline(attr1)
        # Note that stage time of -1 (or any time before 0) maps to clip time 0
        # for this test
        self.CheckValue(attr1, time=-1, expected=None)
        self.CheckValue(attr1, time=Usd.TimeCode.PreTime(1), expected=None)
        self.CheckValue(attr1, time=1, expected=3)
        self.CheckValue(attr1, time=Usd.TimeCode.PreTime(5), expected=3)
        self.CheckValue(attr1, time=5, expected=10)
        self.CheckValue(attr1, time=Usd.TimeCode.PreTime(7), expected=10)
        self.CheckValue(attr1, time=7, expected=11)
        self.CheckValue(attr1, time=Usd.TimeCode.PreTime(10), expected=11)
        # clip boundary here
        self.CheckValueClose(attr1, time=10, expected=7.5)
        self.CheckValue(attr1, time=Usd.TimeCode.PreTime(15), expected=10)
        self.CheckValue(attr1, time=15, expected=10)
        self.CheckValue(attr1, time=17.5, expected=5)
        self.CheckValue(attr1, time=20, expected=10)
        self.CheckValue(attr1, time=22.5, expected=10)
        self.CheckValue(attr1, time=25, expected=10)

        attr2 = model.GetAttribute("attr2")
        self.CheckSpline(attr2)
        self.CheckValue(attr2, time=-1, expected=None)
        self.CheckValue(attr2, time=0, expected=None)
        self.CheckValue(attr2, time=0.5, expected=0)
        self.CheckValue(attr2, time=Usd.TimeCode.PreTime(2.5), expected=0)
        # Note spanning_clip1's post extrapolation none kicks in at clip time 5
        self.CheckValue(attr2, time=2.5, expected=None)
        self.CheckValue(attr2, time=5, expected=None)
        self.CheckValue(attr2, time=Usd.TimeCode.PreTime(10), expected=None)
        # clip boundary here
        self.CheckValue(attr2, time=10, expected=5)
        self.CheckValue(attr2, time=Usd.TimeCode.PreTime(15), expected=5)
        self.CheckValue(attr2, time=15, expected=None)
        self.CheckValue(attr2, time=20, expected=None)
        self.CheckValue(attr2, time=50, expected=None)

        attr3 = model.GetAttribute("attr3")
        self.CheckSpline(attr3)
        self.CheckValue(attr3, time=-1, expected=None)
        self.CheckValue(attr3, time=5, expected=None)
        self.CheckValue(attr3, time=19, expected=None)
        self.CheckValue(attr3, time=21, expected=None)

        attr4 = model.GetAttribute("attr4")
        self.CheckSpline(attr4)
        self.CheckValue(attr4, time=-42, expected=5)
        self.CheckValue(attr4, time=-1, expected=5)
        self.CheckValue(attr4, time=0, expected=5)
        self.CheckValue(attr4, time=Usd.TimeCode.PreTime(2.5), expected=10)
        self.CheckValue(attr4, time=2.5, expected=5)
        self.CheckValue(attr4, time=Usd.TimeCode.PreTime(5), expected=10)
        self.CheckValue(attr4, time=5, expected=10)
        self.CheckValue(attr4, time=Usd.TimeCode.PreTime(10), expected=12.5)
        # clip boundary here
        self.CheckValue(attr4, time=10, expected=7.5)
        self.CheckValue(attr4, time=15, expected=10)
        self.CheckValue(attr4, time=17.5, expected=5)
        self.CheckValue(attr4, time=Usd.TimeCode.PreTime(20), expected=10)
        self.CheckValue(attr4, time=20, expected=None)

if __name__ == "__main__":
    unittest.main()
