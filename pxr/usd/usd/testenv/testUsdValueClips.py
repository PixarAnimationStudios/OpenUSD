#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

import contextlib
import os
import shutil
import unittest
from pxr import Sdf, Tf, Usd, Vt, Gf

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

            while endClip < max(allTimeSamples):
                self.assertEqual(
                    attr.GetTimeSamplesInInterval(
                        Gf.Interval(startClip, endClip)), 
                    [t for t in allTimeSamples if t <= endClip])
                endClip += 1

    def CheckValue(self, attr, expected, time=None, query=True):
        if time is not None:
            self.assertEqual(attr.Get(time), expected)
            if query:
                self.assertEqual(Usd.AttributeQuery(attr).Get(time), expected)
        else:
            self.assertEqual(attr.Get(), expected)
            if query:
                self.assertEqual(Usd.AttributeQuery(attr).Get(), expected)

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
        # objects, since that act *does* need to load clips is the attr is 
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

        # Model_1 has active clips authored starting at time 10. However, the first
        # active clip is "held active" to -inf, and for any given time t, the prior
        # active clip at time t is still considered active.  So even when querying
        # a timeSample prior to the first "active time", we expect the first clip
        # to be loaded and consulted, with a linear time-mapping from stage time
        # to time-within-clip-earlier-than-first-clipTimes-knot.  In our test case,
        # this means all attrs except localAttr (which has local timeSamples in the
        # clip-anchoring layer) should get their values from the first clip.
        self.CheckValue(localAttr, time=5, expected=5.0)
        self.CheckValue(refAttr, time=5, expected=-5.0)
        self.CheckValue(clsAttr, time=5, expected=-5.0)
        self.CheckValue(payloadAttr, time=5, expected=-5.0)
        self.CheckValue(varAttr, time=5, expected=-5.0)

        # We expect the manifest and the first clip to be opened at this point.
        self.assertTrue(Sdf.Layer.Find('basic/clip.usda'))
        self.assertTrue(Sdf.Layer.Find('basic/manifest.usda'))

        # Starting at time 10, clips should be consulted for values.
        #
        # The strength order using during time sample resolution is 
        # L1(ocal)C(lip)L2(ocal)I(nherit)V(ariant)R(eference)P(ayload), so
        # local opinions in layers stronger than the layer that anchors the clip
        # metadata (L1 above, which *includes* the anchoring subLayer) should win
        # over the clip, but the clip should win over all other opinions, including
        # those from loal subLayers weaker than the anchoring layer (L2).
        self.CheckValue(localAttr, time=10, expected=10.0)
        self.CheckValue(refAttr, time=10, expected=-10.0)
        self.CheckValue(clsAttr, time=10, expected=-10.0)
        self.CheckValue(payloadAttr, time=10, expected=-10.0)
        self.CheckValue(varAttr, time=10, expected=-10.0)

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
        """Exercises clip retiming via clipTimes metadata for timecode value 
        attributes"""
        stage = Usd.Stage.Open('timeCodeTiming/root.usda')

        model = stage.GetPrimAtPath('/Model')
        attr = model.GetAttribute('time')
        attr2 = model.GetAttribute('timeArray')

        # Default value should come through regardless of clip timing.
        self.CheckValue(attr, expected=1.0)
        self.CheckValue(attr2, expected=Sdf.TimeCodeArray([1.0,2.0]))

        stage.SetInterpolationType(Usd.InterpolationTypeLinear)

        # The 'clipTimes' metadata authored in the test asset offsets the 
        # time samples in the clip by 10 frames and scales it slower by 50%,
        # then at frame 21 it repeats the clip from frame 0 without the offset
        # and scaling..
        self.CheckValue(attr, time=0, expected=5.0)
        self.CheckValue(attr, time=5, expected=5.0)
        self.CheckValue(attr, time=10, expected=5.0)
        self.CheckValue(attr, time=15, expected=5.0)
        self.CheckValue(attr, time=20, expected=45.0)
        self.CheckValue(attr, time=25, expected=40.0)
        self.CheckValue(attr, time=30, expected=35.0)
        self.CheckValue(attr, time=35, expected=30.0)
        self.CheckValue(attr, time=40, expected=25.0)

        # Requests for samples before and after the mapping specified in
        # 'clipTimes' just pick up the first or last time sample.
        self.CheckValue(attr, time=-1, expected=5.0)
        self.CheckValue(attr, time=41, expected=25.0)

        # Repeat getting values at the same times for the SdfTimeCodeArray 
        # valued attribute.
        self.CheckValue(attr2, time=0, expected=Sdf.TimeCodeArray([0.0, 5.0]))
        self.CheckValue(attr2, time=5, expected=Sdf.TimeCodeArray([5.0, 5.0]))
        self.CheckValue(attr2, time=10, expected=Sdf.TimeCodeArray([10.0, 5.0]))
        self.CheckValue(attr2, time=15, expected=Sdf.TimeCodeArray([15.0, 5.0]))
        self.CheckValue(attr2, time=20, expected=Sdf.TimeCodeArray([20.0, 45.0]))
        self.CheckValue(attr2, time=25, expected=Sdf.TimeCodeArray([25.0, 40.0]))
        self.CheckValue(attr2, time=30, expected=Sdf.TimeCodeArray([30.0, 35.0]))
        self.CheckValue(attr2, time=35, expected=Sdf.TimeCodeArray([35.0, 30.0]))
        self.CheckValue(attr2, time=40, expected=Sdf.TimeCodeArray([40.0, 25.0]))

        self.CheckValue(attr2, time=-1, expected=Sdf.TimeCodeArray([0.0, 5.0]))
        self.CheckValue(attr2, time=41, expected=Sdf.TimeCodeArray([40.0, 25.0]))

        # Repeat the test over again with held interpolation.
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        # The 'clipTimes' metadata authored in the test asset offsets the 
        # time samples in the clip by 10 frames and scales it slower by 50%,
        # then at frame 21 it repeats the clip from frame 0 without the offset
        # and scaling..
        self.CheckValue(attr, time=0, expected=5.0)
        self.CheckValue(attr, time=5, expected=5.0)
        self.CheckValue(attr, time=10, expected=5.0)
        self.CheckValue(attr, time=15, expected=5.0)
        self.CheckValue(attr, time=20, expected=45.0)
        self.CheckValue(attr, time=25, expected=40.0)
        self.CheckValue(attr, time=30, expected=35.0)
        self.CheckValue(attr, time=35, expected=30.0)
        self.CheckValue(attr, time=40, expected=25.0)

        # Requests for samples before and after the mapping specified in
        # 'clipTimes' just pick up the first or last time sample.
        self.CheckValue(attr, time=-1, expected=5.0)
        self.CheckValue(attr, time=41, expected=25.0)

        # Repeat getting values at the same times for the SdfTimeCodeArray 
        # valued attribute.
        self.CheckValue(attr2, time=0, expected=Sdf.TimeCodeArray([0.0, 5.0]))
        self.CheckValue(attr2, time=5, expected=Sdf.TimeCodeArray([0.0, 5.0]))
        self.CheckValue(attr2, time=10, expected=Sdf.TimeCodeArray([10.0, 5.0]))
        self.CheckValue(attr2, time=15, expected=Sdf.TimeCodeArray([10.0, 5.0]))
        self.CheckValue(attr2, time=20, expected=Sdf.TimeCodeArray([20.0, 45.0]))
        self.CheckValue(attr2, time=25, expected=Sdf.TimeCodeArray([25.0, 40.0]))
        self.CheckValue(attr2, time=30, expected=Sdf.TimeCodeArray([30.0, 35.0]))
        self.CheckValue(attr2, time=35, expected=Sdf.TimeCodeArray([35.0, 30.0]))
        self.CheckValue(attr2, time=40, expected=Sdf.TimeCodeArray([40.0, 25.0]))

        self.CheckValue(attr2, time=-1, expected=Sdf.TimeCodeArray([0.0, 5.0]))
        self.CheckValue(attr2, time=41, expected=Sdf.TimeCodeArray([40.0, 25.0]))

        # The clip has time samples authored every 5 frames, but
        # since we've scaled everything by 50%, we should have samples
        # every 10 frames.
        self.assertEqual(
            attr.GetTimeSamples(), 
            [0, 10, 20 - Usd.TimeCode.SafeStep(), 20, 25, 30, 35, 40])
        self.assertEqual(
            attr.GetTimeSamplesInInterval(Gf.Interval(0, 30)),
            [0, 10, 20 - Usd.TimeCode.SafeStep(), 20, 25, 30])

        self.CheckTimeSamples(attr)
        self.CheckTimeSamples(attr2)

    def test_ClipsWithLayerOffsets(self):
        """Tests behavior of clips when layer offsets are involved"""
        stage = Usd.Stage.Open('layerOffsets/root.usda')

        model1 = stage.GetPrimAtPath('/Model_1')
        attr1 = model1.GetAttribute('size')
        model2 = stage.GetPrimAtPath('/Model_2')
        attr2 = model2.GetAttribute('size')
        model3 = stage.GetPrimAtPath('/Model_3')
        attr3 = model3.GetAttribute('size')

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

        self.CheckTimeSamples(attr1)
        self.CheckTimeSamples(attr2)
        self.CheckTimeSamples(attr3)

    def test_TimeCodeClipsWithLayerOffsets(self):
        """Tests behavior of clips when layer offsets are involved and the
        attributes are SdfTimeCode values. This test is almost identical to 
        test_ClipsWithLayerOffsets except that values returned themselves are
        also offset by the layer offsets."""
        stage = Usd.Stage.Open('layerOffsets/root.usda')

        model1 = stage.GetPrimAtPath('/Model_1')
        attr1 = model1.GetAttribute('time')
        model2 = stage.GetPrimAtPath('/Model_2')
        attr2 = model2.GetAttribute('time')
        model3 = stage.GetPrimAtPath('/Model_3')
        attr3 = model3.GetAttribute('time')

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

        self.CheckTimeSamples(attr1)
        self.CheckTimeSamples(attr2)
        self.CheckTimeSamples(attr3)

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
            self.CheckValue(attr, time=16, expected=-23.0)
            self.CheckValue(attr, time=17, expected=-23.0)
            self.CheckValue(attr, time=18, expected=-23.0)
            self.CheckValue(attr, time=19, expected=-23.0)
            self.CheckValue(attr, time=20, expected=-23.0)
            self.CheckValue(attr, time=21, expected=-23.0)
            self.CheckValue(attr, time=22, expected=-26.0)
            self.CheckValue(attr, time=23, expected=-26.0)
            self.CheckValue(attr, time=24, expected=-26.0)
            self.CheckValue(attr, time=25, expected=-29.0)
            self.CheckValue(attr, time=26, expected=-29.0)
            self.CheckValue(attr, time=27, expected=-29.0)
            self.CheckValue(attr, time=28, expected=-29.0)
            self.CheckValue(attr, time=29, expected=-29.0)
            self.CheckValue(attr, time=30, expected=-29.0)
            self.CheckValue(attr, time=31, expected=-29.0)

        # Repeat test with linear interpolation
        with InterpolationType(stage, Usd.InterpolationTypeLinear):
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
            self.CheckValue(attr, time=3, expected=-26.0)

            # Middle clip with no samples. Since the middle clip has no 
            # time samples and there is no default value specified in the
            # manifest, we get a value of None.
            self.CheckValue(attr, time=4, expected=None)
            self.CheckValue(attr, time=5, expected=None)
            self.CheckValue(attr, time=6, expected=None)
            self.CheckValue(attr, time=7, expected=None)

            # Last clip.
            self.CheckValue(attr, time=8, expected=-26.0)
            self.CheckValue(attr, time=9, expected=-26.0)
            self.CheckValue(attr, time=10, expected=-26.0)
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

            # Middle clip with no samples. Since the middle clip has no 
            # time samples and there is no default value specified in the
            # manifest, we get a value of None.
            self.CheckValue(attr, time=4, expected=None)
            self.CheckValue(attr, time=5, expected=None)
            self.CheckValue(attr, time=6, expected=None)
            self.CheckValue(attr, time=7, expected=None)

            # Last clip.
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
        
        # Verify the time samples from the second clip.
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
        self.CheckValue(attr, time=2, expected=200.0)
        self.CheckValue(attr, time=3, expected=300.0)
        self.CheckValue(attr, time=4, expected=400.0)

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
        self.CheckValue(fallbackInManifest, time=2.0, expected=50.0)
        self.CheckValue(fallbackInManifest, time=4.0, expected=20.0)
        self.assertEqual(fallbackInManifest.GetTimeSamples(),
                         [0.0, 1.0, 2.0 - Usd.TimeCode.SafeStep(), 2.0,
                          4.0 - Usd.TimeCode.SafeStep(), 4.0])
        self.CheckTimeSamples(fallbackInManifest)
        
        fallbackBlockInManifest = \
            stage.GetAttributeAtPath('/Model.fallbackBlockInManifest')
        self.CheckValue(fallbackBlockInManifest, time=0.0, expected=10.0)
        self.CheckValue(fallbackBlockInManifest, time=2.0, expected=None)
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
        self.CheckValue(noFallbackInManifest, time=2.0, expected=None)
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

        attr = stage.GetAttributeAtPath('/Model.assetPath')
        _CheckAssetPathValue(
            attr, time=0, 
            expected=os.path.abspath('assetPathValues/clip1/clip1.usda'))
        _CheckAssetPathValue(
            attr, time=1, 
            expected=os.path.abspath('assetPathValues/manifest/manifest.usda'))
        _CheckAssetPathValue(
            attr, time=2,
            expected=os.path.abspath('assetPathValues/clip2/clip2.usda'))

        attr = stage.GetAttributeAtPath('/Model.assetPathArray')
        _CheckAssetPathArrayValue(
            attr, time=0, 
            expected=[os.path.abspath('assetPathValues/clip1/clip1.usda')])
        _CheckAssetPathArrayValue(
            attr, time=1, 
            expected=[os.path.abspath('assetPathValues/manifest/manifest.usda')])
        _CheckAssetPathArrayValue(
            attr, time=2,
            expected=[os.path.abspath('assetPathValues/clip2/clip2.usda')])

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
             os.path.abspath('assetPathValues/clip2/clip2.usda')])

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

if __name__ == "__main__":
    unittest.main()
