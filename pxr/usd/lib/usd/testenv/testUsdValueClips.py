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

import os
import shutil
import unittest
from pxr import Sdf, Tf, Usd, Vt, Gf

def ValidateAttributeTimeSamples(assertEqFn, attr):
    """Verifies attribute time samples are as expected via
    the time sample API"""
    allTimeSamples = attr.GetTimeSamples()
    assertEqFn(attr.GetNumTimeSamples(), len(allTimeSamples))

    for i in range(0, len(allTimeSamples) - 1):
        (lowerSample, upperSample) = \
            (int(allTimeSamples[i]), int(allTimeSamples[i+1]))
        
        # The attribute's bracketing time samples at each time returned
        # by GetTimeSamples() should be equal to the time.
        assertEqFn(attr.GetBracketingTimeSamples(lowerSample), 
                   (lowerSample, lowerSample))
        assertEqFn(attr.GetBracketingTimeSamples(upperSample), 
                   (upperSample, upperSample))

        # The attribute's bracketing time samples should be the same
        # at every time in the interval (lowerSample, upperSample)
        for t in range(lowerSample + 1, upperSample - 1):
            assertEqFn(attr.GetBracketingTimeSamples(t), 
                       (lowerSample, upperSample))

        # The attribute should return the same value at every time in the
        # interval [lowerSample, upperSample)
        for t in range(lowerSample, upperSample - 1):
            assertEqFn(attr.Get(t), attr.Get(lowerSample))

    # Verify that getting the complete time sample map for this
    # attribute is equivalent to asking for the value at each time
    # returned by GetTimeSamples()
    timeSampleMap = dict([(t, attr.Get(t)) for t in allTimeSamples])

    assertEqFn(timeSampleMap, attr.GetMetadata('timeSamples'))

    # Verify that getting ranges of time samples works
    if len(allTimeSamples) > 2:
        startClip = min(allTimeSamples) 
        endClip = startClip

        while endClip < max(allTimeSamples):
            assertEqFn(attr.GetTimeSamplesInInterval(Gf.Interval(startClip,
                                                                 endClip)), 
                        [t for t in allTimeSamples if t <= endClip])
            endClip += 1

def _Check(assertFn, attr, expected, time=None, query=True):
    if time is not None:
        assertFn(attr.Get(time), expected)
        if query:
            assertFn(Usd.AttributeQuery(attr).Get(time), expected)
    else:
        assertFn(attr.Get(), expected)
        if query:
            assertFn(Usd.AttributeQuery(attr).Get(), expected)

class TestUsdValueClips(unittest.TestCase):
    def test_BasicClipBehavior(self):
        """Exercises basic clip behavior."""
        stage = Usd.Stage.Open('basic/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

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

        # Clips are never consulted for default values.  This implies also
        # that no clips should even get loaded as a result of the queries.
        # However, we must tell _Check() not to construct UsdAttributeQuery
        # objects, since that act *does* need to load clips is the attr is 
        # affected by clips
        _Check(self.assertEqual, localAttr, expected=1.0, query=False)
        _Check(self.assertEqual, refAttr, expected=1.0, query=False)
        _Check(self.assertEqual, clsAttr, expected=1.0, query=False)
        _Check(self.assertEqual, payloadAttr, expected=1.0, query=False)
        _Check(self.assertEqual, varAttr, expected=1.0, query=False)
        
        # Still shouldn't have loaded any clip layers! 
        self.assertEqual(stage.GetUsedLayers(includeClipLayers=True), 
                    stage.GetUsedLayers(includeClipLayers=False))

        # These attributes all have multiple time samples either locally
        # or from the single clip, so they all might be time varying.
        self.assertTrue(localAttr.ValueMightBeTimeVarying())
        self.assertTrue(clsAttr.ValueMightBeTimeVarying())
        self.assertTrue(refAttr.ValueMightBeTimeVarying())
        self.assertTrue(payloadAttr.ValueMightBeTimeVarying())
        self.assertTrue(varAttr.ValueMightBeTimeVarying())

        # Since this test case does not have a clipManifest, we must have
        # opened *all* clips to answer the ValueMightBeTimeVarying() queries.
        # Perfect example of how clipManifestAssetPath helps performance.
        self.assertNotEqual(stage.GetUsedLayers(includeClipLayers=True), 
                            stage.GetUsedLayers(includeClipLayers=False))

        # Model_1 has active clips authored starting at time 10. However, the first
        # active clip is "held active" to -inf, and for any given time t, the prior
        # active clip at time t is still considered active.  So even when querying
        # a timeSample prior to the first "active time", we expect the first clip
        # to be loaded and consulted, with a linear time-mapping from stage time
        # to time-within-clip-earlier-than-first-clipTimes-knot.  In our test case,
        # this means all attrs except localAttr (which has local timeSamples in the
        # clip-anchoring layer) should get their values from the first clip.
        
        _Check(self.assertEqual, localAttr, time=5, expected=5.0)
        _Check(self.assertEqual, refAttr, time=5, expected=-5.0)
        _Check(self.assertEqual, clsAttr, time=5, expected=-5.0)
        _Check(self.assertEqual, payloadAttr, time=5, expected=-5.0)
        _Check(self.assertEqual, varAttr, time=5, expected=-5.0)

        # Starting at time 10, clips should be consulted for values.
        #
        # The strength order using during time sample resolution is 
        # L1(ocal)C(lip)L2(ocal)I(nherit)V(ariant)R(eference)P(ayload), so
        # local opinions in layers stronger than the layer that anchors the clip
        # metadata (L1 above, which *includes* the anchoring subLayer) should win
        # over the clip, but the clip should win over all other opinions, including
        # those from loal subLayers weaker than the anchoring layer (L2).
        _Check(self.assertEqual, localAttr, time=10, expected=10.0)
        _Check(self.assertEqual, refAttr, time=10, expected=-10.0)
        _Check(self.assertEqual, clsAttr, time=10, expected=-10.0)
        _Check(self.assertEqual, payloadAttr, time=10, expected=-10.0)
        _Check(self.assertEqual, varAttr, time=10, expected=-10.0)

        # Attributes in prims that are descended from where the clip
        # metadata was authored should pick up opinions from the clip
        # too, just like above.
        child = stage.GetPrimAtPath('/Model_1/Child')
        childAttr = child.GetAttribute('attr')

        _Check(self.assertEqual, childAttr, expected=1.0)
        _Check(self.assertEqual, childAttr, time=5, expected=-5.0)
        _Check(self.assertEqual, childAttr, time=10, expected=-10.0)

        ValidateAttributeTimeSamples(self.assertEqual, localAttr)
        ValidateAttributeTimeSamples(self.assertEqual, refAttr)
        ValidateAttributeTimeSamples(self.assertEqual, clsAttr)
        ValidateAttributeTimeSamples(self.assertEqual, payloadAttr)
        ValidateAttributeTimeSamples(self.assertEqual, varAttr)
        ValidateAttributeTimeSamples(self.assertEqual, childAttr)

        # Before reload, stage should still be getting the old value
        clipAttr = stage.GetPrimAtPath('/Model_1/Child').GetAttribute('attr')
        _Check(self.assertEqual, clipAttr, expected=-5, time=5)

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
            _Check(self.assertEqual, clipAttr, expected=1005, time=5)
        finally:
            shutil.move('basic/clip.usda.old', 'basic/clip.usda')

    def test_ClipTiming(self):
        """Exercises clip retiming via clipTimes metadata"""
        stage = Usd.Stage.Open('timing/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)
        
        model = stage.GetPrimAtPath('/Model')
        attr = model.GetAttribute('size')

        # Default value should come through regardless of clip timing.
        _Check(self.assertEqual, attr, expected=1.0)

        # The 'clipTimes' metadata authored in the test asset offsets the 
        # time samples in the clip by 10 frames and scales it slower by 50%,
        # repeating at frame 21.
        _Check(self.assertEqual, attr, time=0, expected=10.0)
        _Check(self.assertEqual, attr, time=5, expected=10.0)
        _Check(self.assertEqual, attr, time=10, expected=15.0)
        _Check(self.assertEqual, attr, time=15, expected=15.0)
        _Check(self.assertEqual, attr, time=20, expected=20.0)
        _Check(self.assertEqual, attr, time=21, expected=10.0)
        _Check(self.assertEqual, attr, time=26, expected=10.0)
        _Check(self.assertEqual, attr, time=31, expected=15.0)
        _Check(self.assertEqual, attr, time=36, expected=15.0)
        _Check(self.assertEqual, attr, time=41, expected=20.0)

        # Requests for samples before and after the mapping specified in
        # 'clipTimes' just pick up the first or last time sample.
        _Check(self.assertEqual, attr, time=-1, expected=10.0)
        _Check(self.assertEqual, attr, time=42, expected=20.0)

        # The clip has time samples authored every 5 frames, but
        # since we've scaled everything by 50%, we should have samples
        # every 10 frames.
        self.assertEqual(attr.GetTimeSamples(), [0, 10, 20, 21, 31, 41])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0, 30)),
                [0, 10, 20, 21])

        # Test trickier cases where time samples in the clip fall outside
        # of the time domain specified by the 'clipTimes' metadata.
        model2 = stage.GetPrimAtPath('/Model2')
        attr2 = model2.GetAttribute('size')

        _Check(self.assertEqual, attr2, time=20, expected=15.0)
        _Check(self.assertEqual, attr2, time=30, expected=25.0)

        self.assertEqual(attr2.GetTimeSamples(),
            [0.0, 10.0, 20.0, 25.0, 30.0])
        self.assertEqual(attr2.GetTimeSamplesInInterval(Gf.Interval(0, 25)), 
            [0.0, 10.0, 20.0, 25.0])

        ValidateAttributeTimeSamples(self.assertEqual, attr)
        ValidateAttributeTimeSamples(self.assertEqual, attr2)

    def test_ClipTimingOutsideRange(self):
        """Tests clip retiming behavior when the mapped clip times are outside
        the range of time samples in the clip"""
        stage = Usd.Stage.Open('timingOutsideClip/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        model = stage.GetPrimAtPath('/Model')
        attr = model.GetAttribute('size')

        # Asking for frames outside the mapped times should also clamp to
        # the nearest time sample.
        for t in xrange(-10, 0):
            _Check(self.assertEqual, attr, time=t, expected=25.0)
            self.assertEqual(attr.GetBracketingTimeSamples(t), (0.0, 0.0))

        for t in xrange(11, 20):
            _Check(self.assertEqual, attr, time=t, expected=25.0)
            self.assertEqual(attr.GetBracketingTimeSamples(t), (10.0, 10.0))

        self.assertEqual(attr.GetTimeSamples(), 
            [0.0, 10.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(-1.0, 1.0)), 
            [0.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0.0, 0.0)), 
            [0.0])
        ValidateAttributeTimeSamples(self.assertEqual, attr)

    def test_ClipsWithLayerOffsets(self):
        """Tests behavior of clips when layer offsets are involved"""
        stage = Usd.Stage.Open('layerOffsets/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        model1 = stage.GetPrimAtPath('/Model_1')
        attr1 = model1.GetAttribute('size')

        # Default value should be unaffected by layer offsets.
        _Check(self.assertEqual, attr1, expected=1.0)

        # The clip should be active starting from frame -10.0 due to the
        # offset; outside of that we should get the value from the reference.
        _Check(self.assertEqual, attr1, time=-11, expected=-5.0)

        # Sublayer offset of 10 frames is present, so attribute value at
        # frame 0 should be from the clip at frame 10, etc.
        _Check(self.assertEqual, attr1, time=0, expected=-10.0)
        _Check(self.assertEqual, attr1, time=-5, expected=-5.0)
        _Check(self.assertEqual, attr1, time=-10, expected=-5.0)
        self.assertEqual(attr1.GetTimeSamples(), 
           [-10.0, -5.0, 0.0, 5.0, 10.0])
        self.assertEqual(attr1.GetTimeSamplesInInterval(Gf.Interval(-10, 10)), 
           [-10.0, -5.0, 0.0, 5.0, 10.0])
 
        # Test that layer offsets on layers where clipTimes/clipActive are
        # authored are taken into account. The test case is similar to above,
        # except clipTimes/clipActive have been authored in a sublayer that
        # is offset by 20 frames instead of 10. 
        model2 = stage.GetPrimAtPath('/Model_2')
        attr2 = model2.GetAttribute('size')

        _Check(self.assertEqual, attr2, expected=1.0)
        _Check(self.assertEqual, attr2, time=-21, expected=-5.0)
        _Check(self.assertEqual, attr2, time=0, expected=-20.0)
        _Check(self.assertEqual, attr2, time=-5, expected=-15.0)
        _Check(self.assertEqual, attr2, time=-10, expected=-10.0)
        self.assertEqual(attr2.GetTimeSamples(), 
            [-20.0, -15.0, -10.0, -5.0, 0.0])
        self.assertEqual(attr2.GetTimeSamplesInInterval(Gf.Interval(-3, 1)), 
            [0.0])

        # Test that reference offsets are taken into account. An offset
        # of 10 frames is authored on the reference; this should be combined
        # with the offset of 10 frames on the sublayer.
        model3 = stage.GetPrimAtPath('/Model_3')
        attr3 = model3.GetAttribute('size')

        _Check(self.assertEqual, attr3, expected=1.0)
        _Check(self.assertEqual, attr3, time=-21, expected=-5.0)
        _Check(self.assertEqual, attr3, time=0, expected=-20.0)
        _Check(self.assertEqual, attr3, time=-5, expected=-15.0)
        _Check(self.assertEqual, attr3, time=-10, expected=-10.0)
        self.assertEqual(attr3.GetTimeSamples(), 
            [-20, -15, -10, -5, 0])
        self.assertEqual(attr3.GetTimeSamplesInInterval(Gf.Interval(-5, 5)), 
            [-5, 0])

        ValidateAttributeTimeSamples(self.assertEqual, attr1)
        ValidateAttributeTimeSamples(self.assertEqual, attr2)
        ValidateAttributeTimeSamples(self.assertEqual, attr3)

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
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

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
        _Check(self.assertEqual, attr, time=10, expected=5.0)

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
        _Check(self.assertEqual, attr, time=5, expected=50.0) 

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
        _Check(self.assertEqual, attr, time=15, expected=500.0)

    def test_SingleClip(self):
        """Verifies behavior with a single clip being applied to a prim"""
        stage = Usd.Stage.Open('singleclip/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        model = stage.GetPrimAtPath('/SingleClip')

        # This prim has a single clip that contributes just one time sample
        # for this attribute. That value will be used over all time.
        attr_1 = model.GetAttribute('attr_1')

        self.assertFalse(attr_1.ValueMightBeTimeVarying())
        _Check(self.assertEqual, attr_1, time=0, expected=10.0)
        self.assertEqual(attr_1.GetTimeSamples(), [0.0])
        self.assertEqual(attr_1.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [0.0])

        ValidateAttributeTimeSamples(self.assertEqual, attr_1)

        # This attribute has no time samples in the clip or elsewhere. Value 
        # resolution will fall back to the default value, which will be used over 
        # all time.
        attr_2 = model.GetAttribute('attr_2')

        self.assertFalse(attr_2.ValueMightBeTimeVarying())
        _Check(self.assertEqual, attr_2, time=0, expected=2.0)
        self.assertEqual(attr_2.GetTimeSamples(), [])
        self.assertEqual(attr_2.GetTimeSamplesInInterval( 
            Gf.Interval.GetFullInterval()), [])

        ValidateAttributeTimeSamples(self.assertEqual, attr_2)

    def test_MultipleClips(self):
        """Verifies behavior with multiple clips being applied to a single prim"""
        stage = Usd.Stage.Open('multiclip/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        model = stage.GetPrimAtPath('/Model_1')
        attr = model.GetAttribute('size')

        # This prim has multiple clips that contribute values to this attribute,
        # so it should be detected as potentially time varying.
        self.assertTrue(attr.ValueMightBeTimeVarying())

        # Doing this check should only have caused the first clip to be opened.
        self.assertTrue(Sdf.Layer.Find('multiclip/clip1.usda'))
        self.assertFalse(Sdf.Layer.Find('multiclip/clip2.usda'))
        
        # clip1 is active in the range [..., 16)
        # clip2 is active in the range [16, ...)
        # Check that we get time samples from the right clip when querying
        # in those ranges.
        _Check(self.assertEqual, attr, time=5, expected=-5)
        _Check(self.assertEqual, attr, time=10, expected=-10)
        _Check(self.assertEqual, attr, time=15, expected=-15)
        _Check(self.assertEqual, attr, time=19, expected=-23)
        _Check(self.assertEqual, attr, time=22, expected=-26)
        _Check(self.assertEqual, attr, time=25, expected=-29)

        # Value clips introduce time samples at their boundaries, even if there
        # isn't an actual time sample in the clip at that time. This is to
        # isolate them from surrounding clips. So, the value from frame 16 comes
        # from clip 2.
        _Check(self.assertEqual, attr, time=16, expected=-23)
        self.assertEqual(attr.GetBracketingTimeSamples(16), (16, 16))

        # Verify that GetTimeSamples() returns time samples from both clips.
        self.assertEqual(attr.GetTimeSamples(), 
            [0.0, 5.0, 10.0, 15.0, 16.0, 19.0, 22.0, 25.0, 31.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0, 30)), 
            [0.0, 5.0, 10.0, 15.0, 16.0, 19.0, 22.0, 25.0])
        ValidateAttributeTimeSamples(self.assertEqual, attr)

    def test_MultipleClipsWithNoTimeSamples(self):
        """Tests behavior when multiple clips are specified on a prim and none
        have time samples for an attributed owned by that prim."""
        stage = Usd.Stage.Open('multiclip/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        model = stage.GetPrimAtPath('/ModelWithNoClipSamples')
        attr = model.GetAttribute('size')
        
        # Since none of the clips provide samples for this attribute, we should
        # fall back to the default value and report that this attribute's values
        # are constant over time.
        self.assertFalse(attr.ValueMightBeTimeVarying())
        self.assertEqual(attr.GetResolveInfo(0).GetSource(),
            Usd.ResolveInfoSourceDefault)

        # Doing this check should have caused all clips to be opened, since
        # we need to check each one to see if any of them provide a time sample.
        self.assertTrue(Sdf.Layer.Find('multiclip/nosamples_clip.usda'))
        self.assertTrue(Sdf.Layer.Find('multiclip/nosamples_clip2.usda'))

        # This prim has multiple clips specified from frames [0.0, 31.0] but
        # none provide samples for the size attribute. The value in this
        # time range should be equal to the default value from the reference.
        # The value outside this time range should also be the default
        # value, since no clips are active in those times.
        for t in xrange(-10, 40):
            _Check(self.assertEqual, attr, time=t, expected=1.0)

        # Since none of the clips provide samples, there should be no
        # time samples or bracketing time samples at any of these times.
        for t in xrange(-10, 40):
            self.assertEqual(attr.GetBracketingTimeSamples(t), ())

        self.assertEqual(attr.GetTimeSamples(), [])
        self.assertEqual(attr.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])

        ValidateAttributeTimeSamples(self.assertEqual, attr)

    def test_MultipleClipsWithSomeTimeSamples(self):
        """Tests behavior when multiple clips are specified on a prim and
        some of them have samples for an attribute owned by that prim, while
        others do not."""
        stage = Usd.Stage.Open('multiclip/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        model = stage.GetPrimAtPath('/ModelWithSomeClipSamples')
        attr = model.GetAttribute('size')
        
        # The clip in the range [..., 16) has no samples for the attribute,
        # so the value should be the default value from the reference.
        for t in xrange(-10, 16):
            _Check(self.assertEqual, attr, time=t, expected=1.0)

        # This attribute should be detected as potentially time-varying
        # since multiple clips are involved and at least one of them has
        # samples.
        self.assertTrue(attr.ValueMightBeTimeVarying())

        # The clip in the range [16, ...) has samples on frames 3, 6, 9 so
        # we expect time samples for this attribute at frames 19, 22, and 25.
        for t in xrange(16, 22):
            _Check(self.assertEqual, attr, time=t, expected=-23.0)
        for t in xrange(22, 25):
            _Check(self.assertEqual, attr, time=t, expected=-26.0)
        for t in xrange(25, 31):
            _Check(self.assertEqual, attr, time=t, expected=-29.0)

        self.assertEqual(attr.GetTimeSamples(), 
            [0.0, 15.0, 16.0, 19.0, 22.0, 25.0, 31.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(-5, 50)), 
            [0.0, 15.0, 16.0, 19.0, 22.0, 25.0, 31.0])

        ValidateAttributeTimeSamples(self.assertEqual, attr)

    def test_MultipleClipsWithSomeTimeSamples2(self):
        """Another test case similar to TestMultipleClipsWithSomeTimeSamples2."""
        stage = Usd.Stage.Open('multiclip/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        model = stage.GetPrimAtPath('/ModelWithSomeClipSamples2')
        attr = model.GetAttribute('size')

        # This attribute should be detected as potentially time-varying
        # since multiple clips are involved and at least one of them has
        # samples.
        self.assertTrue(attr.ValueMightBeTimeVarying())

        # Clips are active in the range [..., 4.0), [4.0, 8.0), and [8.0, ...).
        # The first and last clips have time samples for the size attribute,
        # while the middle clip does not.

        # First clip.
        _Check(self.assertEqual, attr, time=-1, expected=-23.0)
        _Check(self.assertEqual, attr, time=0, expected=-23.0)
        _Check(self.assertEqual, attr, time=1, expected=-23.0)
        _Check(self.assertEqual, attr, time=2, expected=-23.0)
        _Check(self.assertEqual, attr, time=3, expected=-26.0)

        # Middle clip with no samples. Since the middle clip has no time samples,
        # we get the default value specified in the reference, since that's next
        # in the value resolution order.
        _Check(self.assertEqual, attr, time=4, expected=1.0)
        _Check(self.assertEqual, attr, time=5, expected=1.0)
        _Check(self.assertEqual, attr, time=6, expected=1.0)
        _Check(self.assertEqual, attr, time=7, expected=1.0)

        # Last clip.
        _Check(self.assertEqual, attr, time=8, expected=-26.0)
        _Check(self.assertEqual, attr, time=9, expected=-26.0)
        _Check(self.assertEqual, attr, time=10, expected=-26.0)
        _Check(self.assertEqual, attr, time=11, expected=-29.0)
        _Check(self.assertEqual, attr, time=12, expected=-29.0)

        self.assertEqual(attr.GetTimeSamples(), [0.0, 3.0, 4.0, 7.0, 8.0, 11.0])
        self.assertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0, 10)), 
                [0.0, 3.0, 4.0, 7.0, 8.0])

        ValidateAttributeTimeSamples(self.assertEqual, attr)

    def test_OverrideOfAncestralClips(self):
        """Tests that clips specified on a descendant model will override
        clips specified on an ancestral model"""
        stage = Usd.Stage.Open('ancestral/root.usda')
        stage.SetInterpolationType(Usd.InterpolationTypeHeld)

        ancestor = stage.GetPrimAtPath('/ModelGroup')
        ancestorAttr = ancestor.GetAttribute('attr')
        
        self.assertEqual(ancestorAttr.GetTimeSamples(), [5, 10, 15])
        self.assertEqual(ancestorAttr.GetTimeSamplesInInterval(Gf.Interval(0, 15)), 
                [5, 10, 15])
        _Check(self.assertEqual, ancestorAttr, time=5, expected=-5)
        _Check(self.assertEqual, ancestorAttr, time=10, expected=-10)
        _Check(self.assertEqual, ancestorAttr, time=15, expected=-15)

        descendant = stage.GetPrimAtPath('/ModelGroup/Model')
        descendantAttr = descendant.GetAttribute('attr')

        self.assertEqual(descendantAttr.GetTimeSamples(), [1, 2, 3])
        self.assertEqual(descendantAttr.GetTimeSamplesInInterval(Gf.Interval(0, 2.95)), 
                [1, 2])
        _Check(self.assertEqual, descendantAttr, time=1, expected=-1)
        _Check(self.assertEqual, descendantAttr, time=2, expected=-2)
        _Check(self.assertEqual, descendantAttr, time=3, expected=-3)

        ValidateAttributeTimeSamples(self.assertEqual, ancestorAttr)
        ValidateAttributeTimeSamples(self.assertEqual, descendantAttr)

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
        _Check(self.assertEqual, attr, time=1.0, expected=-100.0)

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

        # This attribute doesn't exist in the manifest, so we should
        # not have looked in any clips for samples, and its value should
        # fall back to its default value.
        notInManifestAndInClip = prim.GetAttribute('notInManifestAndInClip')
        self.assertFalse(notInManifestAndInClip.ValueMightBeTimeVarying())
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))
        _Check(self.assertEqual, notInManifestAndInClip, time=0, expected=3.0)
        self.assertEqual(notInManifestAndInClip.GetTimeSamples(), [])
        self.assertEqual(notInManifestAndInClip.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])
        ValidateAttributeTimeSamples(self.assertEqual, notInManifestAndInClip)

        # This attribute also doesn't exist in the manifest and also
        # does not have any samples in the clips. It should behave exactly
        # as above; we should not have to open any of the clips.
        notInManifestNotInClip = prim.GetAttribute('notInManifestNotInClip')
        self.assertFalse(notInManifestNotInClip.ValueMightBeTimeVarying())
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))
        _Check(self.assertEqual, notInManifestNotInClip, time=0, expected=4.0)
        self.assertEqual(notInManifestNotInClip.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])
        ValidateAttributeTimeSamples(self.assertEqual, notInManifestNotInClip)
        
        # This attribute is in the manifest but is declared uniform,
        # so we should also not look in any clips for samples.
        uniformInManifestAndInClip = prim.GetAttribute('uniformInManifestAndInClip')
        self.assertFalse(uniformInManifestAndInClip.ValueMightBeTimeVarying())
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))
        _Check(self.assertEqual, uniformInManifestAndInClip, time=0, expected=5.0)
        self.assertEqual(uniformInManifestAndInClip.GetTimeSamples(), [])
        self.assertEqual(uniformInManifestAndInClip.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])
        ValidateAttributeTimeSamples(self.assertEqual, uniformInManifestAndInClip)

        # This attribute is in the manifest and has samples in the
        # first clip, but not the other. We should get the clip's samples
        # in the first time range, and the default value in the second
        # range.
        inManifestAndInClip = prim.GetAttribute('inManifestAndInClip')
        self.assertTrue(inManifestAndInClip.ValueMightBeTimeVarying())
        # We should only have needed to open the first clip to determine
        # if the attribute might be varying.
        self.assertTrue(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))
        _Check(self.assertEqual, inManifestAndInClip, time=0, expected=0.0)
        _Check(self.assertEqual, inManifestAndInClip, time=1, expected=-1.0)
        _Check(self.assertEqual, inManifestAndInClip, time=2, expected=1.0)
        self.assertEqual(inManifestAndInClip.GetTimeSamples(), 
                         [0.0, 1.0, 2.0, 3.0])
        self.assertEqual(inManifestAndInClip.GetTimeSamplesInInterval(
            Gf.Interval(0, 2.1)), [0.0, 1.0, 2.0])
        ValidateAttributeTimeSamples(self.assertEqual, inManifestAndInClip)

        # Close and reopen the stage to ensure the clip layers are closed
        # before we do the test below.
        stage.Close()
        self.assertFalse(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertFalse(Sdf.Layer.Find('manifest/clip_2.usda'))

        # Lastly, this attribute is in the manifest but has no
        # samples in the clip, so we should just fall back to the default
        # value.
        stage = Usd.Stage.Open('manifest/root.usda')
        prim = stage.GetPrimAtPath('/WithManifestClip')

        inManifestNotInClip = prim.GetAttribute('inManifestNotInClip')
        self.assertFalse(inManifestNotInClip.ValueMightBeTimeVarying())
        # Since the attribute is in the manifest, we have to search all
        # the clips to see which of them have samples. In this case, none
        # of them do, so we fall back to the default value.
        self.assertTrue(Sdf.Layer.Find('manifest/clip_1.usda'))
        self.assertTrue(Sdf.Layer.Find('manifest/clip_2.usda'))
        _Check(self.assertEqual, inManifestNotInClip, time=0, expected=2.0)
        self.assertEqual(inManifestNotInClip.GetTimeSamples(), [])
        self.assertEqual(inManifestNotInClip.GetTimeSamplesInInterval(
            Gf.Interval.GetFullInterval()), [])
        ValidateAttributeTimeSamples(self.assertEqual, inManifestNotInClip)

    def test_ClipTemplateBehavior(self):
        primPath = Sdf.Path('/World/fx/Particles_Splash/points')
        attrName = 'extent'

        stage = Usd.Stage.Open('template/int1/result_int_1.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)
        _Check(self.assertEqual, attr, time=1, expected=Vt.Vec3fArray(2, (1,1,1)))
        _Check(self.assertEqual, attr, time=2, expected=Vt.Vec3fArray(2, (2,2,2)))
        _Check(self.assertEqual, attr, time=3, expected=Vt.Vec3fArray(2, (3,3,3)))
        _Check(self.assertEqual, attr, time=4, expected=Vt.Vec3fArray(2, (4,4,4)))

        stage = Usd.Stage.Open('template/int2/result_int_2.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)
        _Check(self.assertEqual, attr, time=1, expected=Vt.Vec3fArray(2, (1,1,1)))
        _Check(self.assertEqual, attr, time=17, expected=Vt.Vec3fArray(2, (17,17,17)))
        _Check(self.assertEqual, attr, time=33, expected=Vt.Vec3fArray(2, (33,33,33)))
        _Check(self.assertEqual, attr, time=49, expected=Vt.Vec3fArray(2, (49,49,49)))

        stage = Usd.Stage.Open('template/subint1/result_subint_1.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)
        _Check(self.assertEqual, attr, time=101, expected=Vt.Vec3fArray(2, (101,101,101)))
        _Check(self.assertEqual, attr, time=102, expected=Vt.Vec3fArray(2, (102,102,102)))
        _Check(self.assertEqual, attr, time=103, expected=Vt.Vec3fArray(2, (103,103,103)))
        _Check(self.assertEqual, attr, time=104, expected=Vt.Vec3fArray(2, (104,104,104)))

        stage = Usd.Stage.Open('template/subint2/result_subint_2.usda')
        prim = stage.GetPrimAtPath(primPath)
        attr = prim.GetAttribute(attrName)
        _Check(self.assertEqual, attr, time=10.00, expected=Vt.Vec3fArray(2, (10.00, 10.00, 10.00)))
        _Check(self.assertEqual, attr, time=10.05, expected=Vt.Vec3fArray(2, (10.05, 10.05, 10.05)))
        _Check(self.assertEqual, attr, time=10.10, expected=Vt.Vec3fArray(2, (10.10, 10.10, 10.10)))
        _Check(self.assertEqual, attr, time=10.15, expected=Vt.Vec3fArray(2, (10.15, 10.15, 10.15)))

    def test_ClipTemplateWithOffsets(self):
        stage = Usd.Stage.Open('template/layerOffsets/root.usda')
        prim = stage.GetPrimAtPath('/Model')
        attr = prim.GetAttribute('a')

        # Times are offset by 2 via reference and layer offsets,
        # so we expect the value at time 0 to read from clip 2, etc.
        _Check(self.assertEqual, attr, time=-1.0, expected=1.0)
        _Check(self.assertEqual, attr, time=0.0, expected=2.0)
        _Check(self.assertEqual, attr, time=1.0, expected=3.0)

        # Because of the time offset, this should try to read clip 4,
        # but since we only have 3 clips we hold the value from the
        # last one.
        _Check(self.assertEqual, attr, time=2.0, expected=3.0)
    
    def test_ClipsWithSparseOverrides(self):
        # This layer overrides the clipActive metadata to flip
        # the active clips
        stage = Usd.Stage.Open('sparseOverrides/over_root.usda')
        prim = stage.GetPrimAtPath('/main')
        attr = prim.GetAttribute('foo')

        _Check(self.assertEqual, attr,  time=101.0, expected=3.0)
        _Check(self.assertEqual, attr,  time=103.0, expected=1.0)

        # This is the original layer with the clip metadata authored.
        stage = Usd.Stage.Open('sparseOverrides/root.usda')
        prim = stage.GetPrimAtPath('/main')
        attr = prim.GetAttribute('foo')

        _Check(self.assertEqual, attr,  time=101.0, expected=1.0)
        _Check(self.assertEqual, attr,  time=103.0, expected=3.0)

        # This layer overrides the startTime from the template metadata
        # to be equal to the endTime, effectively giving us only one clip
        stage = Usd.Stage.Open('sparseOverrides/template_over_root.usda')
        prim = stage.GetPrimAtPath('/main')
        attr = prim.GetAttribute('foo')

        _Check(self.assertEqual, attr,  time=101.0, expected=3.0)
        _Check(self.assertEqual, attr,  time=103.0, expected=3.0)

        # This is the original layer with the template metadata authored. 
        stage = Usd.Stage.Open('sparseOverrides/template_root.usda')
        prim = stage.GetPrimAtPath('/main')
        attr = prim.GetAttribute('foo')

        _Check(self.assertEqual, attr,  time=101.0, expected=1.0)
        _Check(self.assertEqual, attr,  time=103.0, expected=3.0)

    def test_MultipleClipSets(self):
        """Verifies behavior with multiple clip sets defined on
        the same prim that affect different prims"""
        # This test is not valid for legacy clips, so if the
        # test asset doesn't exist, just skip over it.
        if not os.path.isdir('clipsets'):
            return

        stage = Usd.Stage.Open('clipsets/root.usda')
        
        prim = stage.GetPrimAtPath('/Set/Child_1')
        attr = prim.GetAttribute('attr')
        _Check(self.assertEqual, attr, time=0, expected=-5.0)
        _Check(self.assertEqual, attr, time=1, expected=-10.0)
        _Check(self.assertEqual, attr, time=2, expected=-15.0)
        ValidateAttributeTimeSamples(self.assertEqual, attr)

        prim = stage.GetPrimAtPath('/Set/Child_2')
        attr = prim.GetAttribute('attr')
        _Check(self.assertEqual, attr, time=0, expected=-50.0)
        _Check(self.assertEqual, attr, time=1, expected=-100.0)
        _Check(self.assertEqual, attr, time=2, expected=-200.0)
        ValidateAttributeTimeSamples(self.assertEqual, attr)

    def test_ListEditClipSets(self):
        """Verifies reordering and deleting clip sets via list editing
        operations"""
        # This test is not valid for legacy clips, so if the
        # test asset doesn't exist, just skip over it.
        if not os.path.isdir('clipsetListEdits'):
            return
        
        stage = Usd.Stage.Open('clipsetListEdits/root.usda')

        prim = stage.GetPrimAtPath('/DefaultOrderTest')
        attr = prim.GetAttribute('attr')
        _Check(self.assertEqual, attr, time=0, expected=10.0)
        _Check(self.assertEqual, attr, time=1, expected=20.0)
        _Check(self.assertEqual, attr, time=2, expected=30.0)
        ValidateAttributeTimeSamples(self.assertEqual, attr)

        prim = stage.GetPrimAtPath('/ReorderTest')
        attr = prim.GetAttribute('attr')
        _Check(self.assertEqual, attr, time=0, expected=100.0)
        _Check(self.assertEqual, attr, time=1, expected=200.0)
        _Check(self.assertEqual, attr, time=2, expected=300.0)
        ValidateAttributeTimeSamples(self.assertEqual, attr)

        prim = stage.GetPrimAtPath('/DeleteTest')
        attr = prim.GetAttribute('attr')
        _Check(self.assertEqual, attr, time=0, expected=100.0)
        _Check(self.assertEqual, attr, time=1, expected=200.0)
        _Check(self.assertEqual, attr, time=2, expected=300.0)
        ValidateAttributeTimeSamples(self.assertEqual, attr)

    def test_InterpolateSamplesInClip(self):
        """Tests that time samples in clips are interpolated
        when a clip time is specified and no sample exists in
        the clip at that time."""
        stage = Usd.Stage.Open('interpolation/root.usda')

        prim = stage.GetPrimAtPath('/InterpolationTest')
        attr = prim.GetAttribute('attr')
        _Check(self.assertEqual, attr, time=0, expected=0.0)
        _Check(self.assertEqual, attr, time=1, expected=5.0)
        _Check(self.assertEqual, attr, time=2, expected=10.0)
        _Check(self.assertEqual, attr, time=3, expected=15.0)
        _Check(self.assertEqual, attr, time=4, expected=20.0)
        ValidateAttributeTimeSamples(self.assertEqual, attr)

if __name__ == "__main__":
    unittest.main()
