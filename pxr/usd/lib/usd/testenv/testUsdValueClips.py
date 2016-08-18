#!/pxrpythonsubst

from pxr import Sdf, Tf, Usd, Vt, Gf
from Mentor.Runtime import (SetAssertMode, MTR_EXIT_TEST, FindDataFile,
                            Assert, AssertTrue, AssertFalse, AssertEqual,
                            AssertNotEqual, AssertException,
                            ExpectedErrors, ExpectedWarnings, ExitTest)

def ValidateAttributeTimeSamples(attr):
    """Verifies attribute time samples are as expected via
    the time sample API"""
    allTimeSamples = attr.GetTimeSamples()
    AssertEqual(attr.GetNumTimeSamples(), len(allTimeSamples))

    for i in range(0, len(allTimeSamples) - 1):
        (lowerSample, upperSample) = \
            (int(allTimeSamples[i]), int(allTimeSamples[i+1]))
        
        # The attribute's bracketing time samples at each time returned
        # by GetTimeSamples() should be equal to the time.
        AssertEqual(attr.GetBracketingTimeSamples(lowerSample),
                    (lowerSample, lowerSample))
        AssertEqual(attr.GetBracketingTimeSamples(upperSample),
                    (upperSample, upperSample))

        # The attribute's bracketing time samples should be the same
        # at every time in the interval (lowerSample, upperSample)
        for t in range(lowerSample + 1, upperSample - 1):
            AssertEqual(attr.GetBracketingTimeSamples(t), 
                        (lowerSample, upperSample))

        # The attribute should return the same value at every time in the
        # interval [lowerSample, upperSample)
        for t in range(lowerSample, upperSample - 1):
            AssertEqual(attr.Get(t), attr.Get(lowerSample))

    # Verify that getting the complete time sample map for this
    # attribute is equivalent to asking for the value at each time
    # returned by GetTimeSamples()
    timeSampleMap = dict([(t, attr.Get(t)) for t in allTimeSamples])

    AssertEqual(timeSampleMap, attr.GetMetadata('timeSamples'))

    # Verify that getting ranges of time samples works
    if len(allTimeSamples) > 2:
        startClip = min(allTimeSamples) 
        endClip = startClip

        while endClip < max(allTimeSamples):
            AssertEqual(attr.GetTimeSamplesInInterval(
                            Gf.Interval(startClip, endClip)),
                        [t for t in allTimeSamples if t <= endClip])
            endClip += 1

def TestBasicClipBehavior():
    """Exercises basic clip behavior."""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/basic/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)

    model = stage.GetPrimAtPath('/Model_1')

    localAttr = model.GetAttribute('local')
    refAttr = model.GetAttribute('ref')
    clsAttr = model.GetAttribute('cls')
    payloadAttr = model.GetAttribute('payload')
    varAttr = model.GetAttribute('var')
    AssertTrue(localAttr)
    AssertTrue(clsAttr)
    AssertTrue(refAttr)
    AssertTrue(payloadAttr)
    AssertTrue(varAttr)

    # These attributes all have multiple time samples either locally
    # or from the single clip, so they all might be time varying.
    Assert(localAttr.ValueMightBeTimeVarying())
    Assert(clsAttr.ValueMightBeTimeVarying())
    Assert(refAttr.ValueMightBeTimeVarying())
    Assert(payloadAttr.ValueMightBeTimeVarying())
    Assert(varAttr.ValueMightBeTimeVarying())

    # Model_1 has clips authored starting at time 10. Values for earlier times
    # should not come from the clips, even if the clip happens to have values
    # for those times.
    AssertEqual(localAttr.Get(5), 5.0)
    AssertEqual(refAttr.Get(5), -5.0)
    AssertEqual(clsAttr.Get(5), -5.0)
    AssertEqual(payloadAttr.Get(5), -5.0)
    AssertEqual(varAttr.Get(5), -5.0)

    # Clips are never consulted for default values.
    AssertEqual(localAttr.Get(), 1.0)
    AssertEqual(refAttr.Get(), 1.0)
    AssertEqual(clsAttr.Get(), 1.0)
    AssertEqual(payloadAttr.Get(), 1.0)
    AssertEqual(varAttr.Get(), 1.0)
    
    # Starting at time 10, clips should be consulted for values.
    #
    # The strength order using during time sample resolution is 
    # L(ocal)C(lip)I(nherit)V(ariant)R(eference)P(ayload), so
    # local opinions should win over the clip, but the clip should win
    # over all other opinions
    AssertEqual(localAttr.Get(10), 10.0)
    AssertEqual(refAttr.Get(10), -10.0)
    AssertEqual(clsAttr.Get(10), -10.0)
    AssertEqual(payloadAttr.Get(10), -10.0)
    AssertEqual(varAttr.Get(10), -10.0)

    # Attributes in prims that are descended from where the clip
    # metadata was authored should pick up opinions from the clip
    # too, just like above.
    child = stage.GetPrimAtPath('/Model_1/Child')
    childAttr = child.GetAttribute('attr')

    AssertEqual(childAttr.Get(), 1.0)
    AssertEqual(childAttr.Get(5), -5.0)
    AssertEqual(childAttr.Get(10), -10.0)

    ValidateAttributeTimeSamples(localAttr)
    ValidateAttributeTimeSamples(refAttr)
    ValidateAttributeTimeSamples(clsAttr)
    ValidateAttributeTimeSamples(payloadAttr)
    ValidateAttributeTimeSamples(varAttr)
    ValidateAttributeTimeSamples(childAttr)

def TestClipTiming():
    """Exercises clip retiming via clipTimes metadata"""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/timing/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)
    
    model = stage.GetPrimAtPath('/Model')
    attr = model.GetAttribute('size')

    # Default value should come through regardless of clip timing.
    AssertEqual(attr.Get(), 1.0)

    # The 'clipTimes' metadata authored in the test asset offsets the 
    # time samples in the clip by 10 frames and scales it slower by 50%,
    # repeating at frame 21.
    AssertEqual(attr.Get(0), 10.0)
    AssertEqual(attr.Get(5), 10.0)
    AssertEqual(attr.Get(10), 15.0)
    AssertEqual(attr.Get(15), 15.0)
    AssertEqual(attr.Get(20), 20.0)
    AssertEqual(attr.Get(21), 10.0)
    AssertEqual(attr.Get(26), 10.0)
    AssertEqual(attr.Get(31), 15.0)
    AssertEqual(attr.Get(36), 15.0)
    AssertEqual(attr.Get(41), 20.0)

    # Requests for samples before and after the mapping specified in
    # 'clipTimes' just pick up the first or last time sample.
    AssertEqual(attr.Get(-1), 10.0)
    AssertEqual(attr.Get(42), 20.0)

    # The clip has time samples authored every 5 frames, but
    # since we've scaled everything by 50%, we should have samples
    # every 10 frames.
    AssertEqual(attr.GetTimeSamples(), [0, 10, 20, 21, 31, 41])
    AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0, 30)),
            [0, 10, 20, 21])

    # Test trickier cases where time samples in the clip fall outside
    # of the time domain specified by the 'clipTimes' metadata.
    model2 = stage.GetPrimAtPath('/Model2')
    attr2 = model2.GetAttribute('size')

    AssertEqual(attr2.Get(0), 15.0)
    AssertEqual(attr2.Get(20), 15.0)
    AssertEqual(attr2.Get(30), 25.0)
    AssertEqual(attr2.GetTimeSamples(), [10, 25])
    AssertEqual(attr2.GetTimeSamplesInInterval(Gf.Interval(0, 25)), [10, 25])

    ValidateAttributeTimeSamples(attr)
    ValidateAttributeTimeSamples(attr2)

def TestClipTimingOutsideRange():
    """Tests clip retiming behavior when the mapped clip times are outside
    the range of time samples in the clip"""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/timingOutsideClip/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)

    model = stage.GetPrimAtPath('/Model')
    attr = model.GetAttribute('size')

    # This test case maps frames [0, 10] on the stage to [30, 40] on
    # the clip. However, the clip only has time samples on frames [5, 25].
    # The expected behavior is to clamp to the nearest clip time sample,
    # which is on frame 25 in the clip.
    for t in xrange(11):
        AssertEqual(attr.Get(t), 25.0)
        AssertEqual(attr.GetBracketingTimeSamples(t), (0.0, 0.0))
        
    # Asking for frames outside the mapped times should also clamp to
    # the nearest time sample.
    for t in xrange(-10, 0):
        AssertEqual(attr.Get(t), 25.0)
        AssertEqual(attr.GetBracketingTimeSamples(t), (0.0, 0.0))

    for t in xrange(11, 20):
        AssertEqual(attr.Get(t), 25.0)
        AssertEqual(attr.GetBracketingTimeSamples(t), (0.0, 0.0))

    AssertEqual(attr.GetTimeSamples(), [0.0])
    AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(-1.0, 1.0)), [0.0])
    AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0.0, 0.0)), [0.0])
    ValidateAttributeTimeSamples(attr)

def TestClipsWithLayerOffsets():
    """Tests behavior of clips when layer offsets are involved"""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/layerOffsets/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)

    model1 = stage.GetPrimAtPath('/Model_1')
    attr1 = model1.GetAttribute('size')

    # Default value should be unaffected by layer offsets.
    AssertEqual(attr1.Get(), 1.0)

    # The clip should be active starting from frame -10.0 due to the
    # offset; outside of that we should get the value from the reference.
    AssertEqual(attr1.Get(-11), -5.0)

    # Sublayer offset of 10 frames is present, so attribute value at
    # frame 0 should be from the clip at frame 10, etc.
    AssertEqual(attr1.Get(0), -10.0)
    AssertEqual(attr1.Get(-5), -5.0)
    AssertEqual(attr1.Get(-10), -5.0)
    AssertEqual(attr1.GetTimeSamples(), [-5, 0, 5, 10])
    AssertEqual(attr1.GetTimeSamplesInInterval(Gf.Interval(-10, 10)), [-5, 0, 5, 10])

    # Test that layer offsets on layers where clipTimes/clipActive are
    # authored are taken into account. The test case is similar to above,
    # except clipTimes/clipActive have been authored in a sublayer that
    # is offset by 20 frames instead of 10. 
    model2 = stage.GetPrimAtPath('/Model_2')
    attr2 = model2.GetAttribute('size')

    AssertEqual(attr2.Get(), 1.0)
    AssertEqual(attr2.Get(-21), -5.0)
    AssertEqual(attr2.Get(0), -20.0)
    AssertEqual(attr2.Get(-5), -15.0)
    AssertEqual(attr2.Get(-10), -10.0)
    AssertEqual(attr2.GetTimeSamples(), [-15, -10, -5, 0])
    AssertEqual(attr2.GetTimeSamplesInInterval(Gf.Interval(-3, 1)), [0])

    # Test that reference offsets are taken into account. An offset
    # of 10 frames is authored on the reference; this should be combined
    # with the offset of 10 frames on the sublayer.
    model3 = stage.GetPrimAtPath('/Model_3')
    attr3 = model3.GetAttribute('size')

    AssertEqual(attr3.Get(), 1.0)
    AssertEqual(attr3.Get(-21), -5.0)
    AssertEqual(attr3.Get(0), -20.0)
    AssertEqual(attr3.Get(-5), -15.0)
    AssertEqual(attr3.Get(-10), -10.0)
    AssertEqual(attr3.GetTimeSamples(), [-15, -10, -5, 0])
    AssertEqual(attr3.GetTimeSamplesInInterval(Gf.Interval(-5, 5)), [-5, 0])

    ValidateAttributeTimeSamples(attr1)
    ValidateAttributeTimeSamples(attr2)
    ValidateAttributeTimeSamples(attr3)

def TestClipStrengthOrdering():
    '''Tests strength of clips during resolution'''

    rootLayerFile = FindDataFile('testUsdValueClips/ordering/root.usda')
    clipFile = FindDataFile('testUsdValueClips/ordering/clip.usda')
    subLayerClipIntroFile = FindDataFile(
        'testUsdValueClips/ordering/sublayer_with_clip_intro.usda')
    subLayerWithOpinionFile = FindDataFile(
        'testUsdValueClips/ordering/sublayer_with_opinion.usda')

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
    AssertEqual(attr.GetPropertyStack(10.0),
                [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                 [subLayerClipIntroLayer, clipLayer, subLayerWithOpinionLayer]])
    # With a default time code, clips won't show up
    AssertEqual(attr.GetPropertyStack(Usd.TimeCode.Default()),
                [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                 [subLayerClipIntroLayer, subLayerWithOpinionLayer]])
    AssertEqual(attr.Get(10), 5.0)

    # Ensure that a clip opinion wins out over a weaker sublayer
    propName = 'foo'
    attr = model.GetAttribute(propName)
    AssertEqual(attr.GetPropertyStack(5.0),
                [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                 [clipLayer, subLayerWithOpinionLayer]])
    # With a default time code, clips won't show up
    AssertEqual(attr.GetPropertyStack(Usd.TimeCode.Default()),
                [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                 [subLayerWithOpinionLayer]])
    AssertEqual(attr.Get(5), 50.0) 

    # Ensure fallback to weaker layers works as intended 
    propName = 'bar'
    attr = model.GetAttribute(propName)
    AssertEqual(attr.GetPropertyStack(15.0),
                [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                 [subLayerWithOpinionLayer]])
    # With a default time code, clips won't show up
    AssertEqual(attr.GetPropertyStack(Usd.TimeCode.Default()),
                [p.GetPropertyAtPath(primPath.AppendProperty(propName)) for p in 
                 [subLayerWithOpinionLayer]])
    AssertEqual(attr.Get(15), 500.0)

def TestSingleClip():
    """Verifies behavior with a single clip being applied to a prim"""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/singleclip/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)

    model = stage.GetPrimAtPath('/SingleClip')

    # This prim has a single clip that contributes just one time sample
    # for this attribute. That value will be used over all time.
    attr_1 = model.GetAttribute('attr_1')

    AssertFalse(attr_1.ValueMightBeTimeVarying())
    AssertEqual(attr_1.Get(0), 10.0)
    AssertEqual(attr_1.GetTimeSamples(), [0.0])
    AssertEqual(attr_1.GetTimeSamplesInInterval(
        Gf.Interval.GetFullInterval()), [0.0])

    ValidateAttributeTimeSamples(attr_1)

    # This attribute has no time samples in the clip or elsewhere. Value 
    # resolution will fall back to the default value, which will be used over 
    # all time.
    attr_2 = model.GetAttribute('attr_2')

    AssertFalse(attr_2.ValueMightBeTimeVarying())
    AssertEqual(attr_2.Get(0), 2.0)
    AssertEqual(attr_2.GetTimeSamples(), [])
    AssertEqual(attr_2.GetTimeSamplesInInterval( 
        Gf.Interval.GetFullInterval()), [])

    ValidateAttributeTimeSamples(attr_2)

def TestMultipleClips():
    """Verifies behavior with multiple clips being applied to a single prim"""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/multiclip/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)

    model = stage.GetPrimAtPath('/Model_1')
    attr = model.GetAttribute('size')

    # This prim has multiple clips that contribute values to this attribute,
    # so it should be detected as potentially time varying.
    Assert(attr.ValueMightBeTimeVarying())

    # Doing this check should only have caused the first clip to be opened.
    Assert(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/multiclip/clip1.usda')))
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/multiclip/clip2.usda')))

    # clip1 is active in the range [..., 16)
    # clip2 is active in the range [16, ...)
    # Check that we get time samples from the right clip when querying
    # in those ranges.
    AssertEqual(attr.Get(5), -5)
    AssertEqual(attr.Get(10), -10)
    AssertEqual(attr.Get(15), -15)
    AssertEqual(attr.Get(19), -23)
    AssertEqual(attr.Get(22), -26)
    AssertEqual(attr.Get(25), -29)

    # Value clips introduce time samples at their boundaries, even if there
    # isn't an actual time sample in the clip at that time. This is to
    # isolate them from surrounding clips. So, the value from frame 16 comes
    # from clip 2.
    AssertEqual(attr.Get(16), -23)
    AssertEqual(attr.GetBracketingTimeSamples(16), (16, 16))

    # Verify that GetTimeSamples() returns time samples from both clips.
    AssertEqual(attr.GetTimeSamples(), [5, 10, 15, 16, 19, 22, 25])
    AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0, 30)), 
            [5, 10, 15, 16, 19, 22, 25])

    ValidateAttributeTimeSamples(attr)

def TestMultipleClipsWithNoTimeSamples():
    """Tests behavior when multiple clips are specified on a prim and none
    have time samples for an attributed owned by that prim."""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/multiclip/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)

    model = stage.GetPrimAtPath('/ModelWithNoClipSamples')
    attr = model.GetAttribute('size')
    
    # Since none of the clips provide samples for this attribute, we should
    # fall back to the default value and report that this attribute's values
    # are constant over time.
    AssertFalse(attr.ValueMightBeTimeVarying())

    # Doing this check should have caused all clips to be opened, since
    # we need to check each one to see if any of them provide a time sample.
    Assert(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/multiclip/nosamples_clip.usda')))
    Assert(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/multiclip/nosamples_clip2.usda')))

    # This prim has multiple clips specified from frames [0.0, 31.0] but
    # none provide samples for the size attribute. The value in this
    # time range should be equal to the default value from the reference.
    # The value outside this time range should also be the default
    # value, since no clips are active in those times.
    for t in xrange(-10, 40):
        AssertEqual(attr.Get(t), 1.0)

    # Since none of the clips provide samples, there should be no
    # time samples or bracketing time samples at any of these times.
    for t in xrange(-10, 40):
        AssertEqual(attr.GetBracketingTimeSamples(t), ())

    AssertEqual(attr.GetTimeSamples(), [])
    AssertEqual(attr.GetTimeSamplesInInterval(
        Gf.Interval.GetFullInterval()), [])

    ValidateAttributeTimeSamples(attr)

def TestMultipleClipsWithSomeTimeSamples():
    """Tests behavior when multiple clips are specified on a prim and
    some of them have samples for an attribute owned by that prim, while
    others do not."""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/multiclip/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)

    model = stage.GetPrimAtPath('/ModelWithSomeClipSamples')
    attr = model.GetAttribute('size')
    
    # The clip in the range [..., 16) has no samples for the attribute,
    # so the value should be the default value from the reference.
    for t in xrange(-10, 16):
        AssertEqual(attr.Get(t), 1.0, 
                    "Got value %s at time %f" % (attr.Get(t), t))

    # This attribute should be detected as potentially time-varying
    # since multiple clips are involved and at least one of them has
    # samples.
    Assert(attr.ValueMightBeTimeVarying())

    # The clip in the range [16, ...) has samples on frames 3, 6, 9 so
    # we expect time samples for this attribute at frames 19, 22, and 25.
    for t in xrange(16, 22):
        AssertEqual(attr.Get(t), -23.0)
    for t in xrange(22, 25):
        AssertEqual(attr.Get(t), -26.0)
    for t in xrange(25, 31):
        AssertEqual(attr.Get(t), -29.0)

    AssertEqual(attr.GetTimeSamples(), [16.0, 19.0, 22.0, 25.0])
    AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(-5, 50)), 
        [16.0, 19.0, 22.0, 25.0])

    ValidateAttributeTimeSamples(attr)

def TestMultipleClipsWithSomeTimeSamples2():
    """Another test case similar to TestMultipleClipsWithSomeTimeSamples2."""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/multiclip/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)

    model = stage.GetPrimAtPath('/ModelWithSomeClipSamples2')
    attr = model.GetAttribute('size')

    # This attribute should be detected as potentially time-varying
    # since multiple clips are involved and at least one of them has
    # samples.
    Assert(attr.ValueMightBeTimeVarying())

    # Clips are active in the range [..., 4.0), [4.0, 8.0), and [8.0, ...).
    # The first and last clips have time samples for the size attribute,
    # while the middle clip does not.

    # First clip.
    AssertEqual(attr.Get(-1), -23.0)
    AssertEqual(attr.Get(0), -23.0)
    AssertEqual(attr.Get(1), -23.0)
    AssertEqual(attr.Get(2), -23.0)
    AssertEqual(attr.Get(3), -26.0)

    # Middle clip with no samples. Since the middle clip has no time samples,
    # we get the default value specified in the reference, since that's next
    # in the value resolution order.
    AssertEqual(attr.Get(4), 1.0)
    AssertEqual(attr.Get(5), 1.0)
    AssertEqual(attr.Get(6), 1.0)
    AssertEqual(attr.Get(7), 1.0)

    # Last clip.
    AssertEqual(attr.Get(8), -26.0)
    AssertEqual(attr.Get(9), -26.0)
    AssertEqual(attr.Get(10), -26.0)
    AssertEqual(attr.Get(11), -29.0)
    AssertEqual(attr.Get(12), -29.0)

    AssertEqual(attr.GetTimeSamples(), [0.0, 3.0, 4.0, 8.0, 11.0])
    AssertEqual(attr.GetTimeSamplesInInterval(Gf.Interval(0, 10)), 
            [0.0, 3.0, 4.0, 8.0])

    ValidateAttributeTimeSamples(attr)

def TestOverrideOfAncestralClips():
    """Tests that clips specified on a descendant model will override
    clips specified on an ancestral model"""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/ancestral/root.usda'))
    stage.SetInterpolationType(Usd.InterpolationTypeHeld)

    ancestor = stage.GetPrimAtPath('/ModelGroup')
    ancestorAttr = ancestor.GetAttribute('attr')
    
    AssertEqual(ancestorAttr.GetTimeSamples(), [5, 10, 15])
    AssertEqual(ancestorAttr.GetTimeSamplesInInterval(Gf.Interval(0, 15)), 
            [5, 10, 15])
    AssertEqual(ancestorAttr.Get(5), -5)
    AssertEqual(ancestorAttr.Get(10), -10)
    AssertEqual(ancestorAttr.Get(15), -15)

    descendant = stage.GetPrimAtPath('/ModelGroup/Model')
    descendantAttr = descendant.GetAttribute('attr')

    AssertEqual(descendantAttr.GetTimeSamples(), [1, 2, 3])
    AssertEqual(descendantAttr.GetTimeSamplesInInterval(Gf.Interval(0, 2.95)), 
            [1, 2])
    AssertEqual(descendantAttr.Get(1), -1)
    AssertEqual(descendantAttr.Get(2), -2)
    AssertEqual(descendantAttr.Get(3), -3)

    ValidateAttributeTimeSamples(ancestorAttr)
    ValidateAttributeTimeSamples(descendantAttr)

def TestClipFlatten():
    """Ensure that UsdStages with clips are flattened as expected.
    In particular, the time samples in the flattened stage should incorporate
    data from clips, and no clip metadata should be present"""

    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/flatten/root.usda'))

    expectedFlatStage = Sdf.Layer.FindOrOpen(
        FindDataFile('testUsdValueClips/flatten/flat.usda'))

    AssertEqual(stage.ExportToString(addSourceFileComment=False),
                expectedFlatStage.ExportToString())

def TestClipValidation():
    """Tests validation of clip metadata"""

    # Number of expected warnings due to invalid clip metadata.
    # This should correspond to the number of 'Error' prims in the test file.
    numExpectedWarnings = 2

    # class Listener(object):
    #     def __init__(self):
    #         self.warnings = []
    #         self._listener = Tf.Notice.RegisterGlobally(
    #             'TfDiagnosticNotice::IssuedWarning', 
    #             self._OnNotice)

    #     def _OnNotice(self, notice, sender):
    #         self.warnings.append(notice.warning)

    # l = Listener()

    with ExpectedWarnings(numExpectedWarnings):
        stage = Usd.Stage.Open(
            FindDataFile('testUsdValueClips/validation/root.usda'))

    # XXX: The notice listening portion of this test is disabled for now, since
    # parallel UsdStage population causes these warnings to be emitted from
    # separate threads.  The diagnostic system does not issue notices for
    # warnings and errors not issued from "the main thread".

    # AssertEqual(len(l.warnings), numExpectedWarnings)

    # # Each 'Error' prim should have caused a warning to be posted.
    # for i in range(1, numExpectedWarnings):
    #     errorPrimName = 'Error%d' % i
    #     numErrorsForPrim = sum(1 if errorPrimName in str(e) else 0 
    #                            for e in l.warnings)
    #     AssertEqual(numErrorsForPrim, 1)

    # # The 'NoError' prims should not have caused any errors to be posted.
    # AssertFalse(any(['NoError' in str(e) for e in l.warnings]))

def TestClipsOnNonModel():
    """Verifies that clips authored on non-models work"""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/nonmodel/root.usda'))

    nonModel = stage.GetPrimAtPath('/NonModel')
    AssertFalse(nonModel.IsModel())
    attr = nonModel.GetAttribute('a')
    AssertEqual(attr.Get(1.0), -100.0)

def TestClipsCannotIntroduceNewTopology():
    """Verifies that clips cannot introduce new scenegraph topology"""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/topology/root.usda'))

    prim = stage.GetPrimAtPath('/Model')
    AssertTrue(prim.IsModel())

    # Clips cannot introduce new topology. Prims and properties defined only
    # in the clip should not be visible on the stage.
    AssertFalse(prim.GetAttribute('clipOnly'))
    AssertEqual(prim.GetChildren(), [])

def TestClipAuthoring():
    """Tests clip authoring API on Usd.ClipsAPI"""
    allFormats = ['usd' + x for x in 'abc']
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
        AssertEqual(model.GetClipAssetPaths(), clipAssetPaths)

        model2.SetClipAssetPaths(
            Sdf.AssetPathArray([Sdf.AssetPath('clip1.usda'),
                                Sdf.AssetPath('clip2.usda')]))
        AssertEqual(model2.GetClipAssetPaths(), clipAssetPaths)

        clipPrimPath = "/Clip"
        model.SetClipPrimPath(clipPrimPath)
        AssertEqual(model.GetClipPrimPath(), clipPrimPath)

        clipTimes = Vt.Vec2dArray([(0.0, 0.0),(10.0, 10.0),(20.0, 20.0)])
        model.SetClipTimes(clipTimes)
        AssertEqual(model.GetClipTimes(), clipTimes)

        model2.SetClipTimes(
            Vt.Vec2dArray([Gf.Vec2d(0.0, 0.0),
                           Gf.Vec2d(10.0, 10.0),
                           Gf.Vec2d(20.0, 20.0)]))
        AssertEqual(model2.GetClipTimes(), clipTimes)

        clipActive = [(0.0, 0.0),(10.0, 1.0),(20.0, 0.0)]
        model.SetClipActive(clipActive)
        AssertEqual(model.GetClipActive(), Vt.Vec2dArray(clipActive))

        model2.SetClipActive(
            Vt.Vec2dArray([Gf.Vec2d(0.0, 0.0),
                           Gf.Vec2d(10.0, 1.0),
                           Gf.Vec2d(20.0, 0.0)]))
        AssertEqual(model2.GetClipActive(), Vt.Vec2dArray(clipActive))

        clipManifestAssetPath = Sdf.AssetPath('clip_manifest.usda')
        model.SetClipManifestAssetPath(clipManifestAssetPath)
        AssertEqual(model.GetClipManifestAssetPath(), clipManifestAssetPath)

def TestClipManifest():
    """Verifies behavior with value clips when a clip manifest is 
    specified."""
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/manifest/root.usda'))
    prim = stage.GetPrimAtPath('/WithManifestClip')

    # This attribute doesn't exist in the manifest, so we should
    # not have looked in any clips for samples, and its value should
    # fall back to its default value.
    notInManifestAndInClip = prim.GetAttribute('notInManifestAndInClip')
    AssertFalse(notInManifestAndInClip.ValueMightBeTimeVarying())
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_1.usda')))
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_2.usda')))
    AssertEqual(notInManifestAndInClip.Get(0), 3.0)
    AssertEqual(notInManifestAndInClip.GetTimeSamples(), [])
    AssertEqual(notInManifestAndInClip.GetTimeSamplesInInterval(
        Gf.Interval.GetFullInterval()), [])
    ValidateAttributeTimeSamples(notInManifestAndInClip)

    # This attribute also doesn't exist in the manifest and also
    # does not have any samples in the clips. It should behave exactly
    # as above; we should not have to open any of the clips.
    notInManifestNotInClip = prim.GetAttribute('notInManifestNotInClip')
    AssertFalse(notInManifestNotInClip.ValueMightBeTimeVarying())
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_1.usda')))
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_2.usda')))
    AssertEqual(notInManifestNotInClip.Get(0), 4.0)
    AssertEqual(notInManifestNotInClip.GetTimeSamplesInInterval(
        Gf.Interval.GetFullInterval()), [])
    ValidateAttributeTimeSamples(notInManifestNotInClip)
    
    # This attribute is in the manifest but is declared uniform,
    # so we should also not look in any clips for samples.
    uniformInManifestAndInClip = prim.GetAttribute('uniformInManifestAndInClip')
    AssertFalse(uniformInManifestAndInClip.ValueMightBeTimeVarying())
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_1.usda')))
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_2.usda')))
    AssertEqual(uniformInManifestAndInClip.Get(0), 5.0)
    AssertEqual(uniformInManifestAndInClip.GetTimeSamples(), [])
    AssertEqual(uniformInManifestAndInClip.GetTimeSamplesInInterval(
        Gf.Interval.GetFullInterval()), [])
    ValidateAttributeTimeSamples(uniformInManifestAndInClip)

    # This attribute is in the manifest and has samples in the
    # first clip, but not the other. We should get the clip's samples
    # in the first time range, and the default value in the second
    # range.
    inManifestAndInClip = prim.GetAttribute('inManifestAndInClip')
    Assert(inManifestAndInClip.ValueMightBeTimeVarying())
    # We should only have needed to open the first clip to determine
    # if the attribute might be varying.
    Assert(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_1.usda')))
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_2.usda')))
    AssertEqual(inManifestAndInClip.Get(0), 0.0)
    AssertEqual(inManifestAndInClip.Get(1), -1.0)
    AssertEqual(inManifestAndInClip.Get(2), 1.0)
    AssertEqual(inManifestAndInClip.GetTimeSamples(), [0.0, 1.0, 2.0])
    AssertEqual(inManifestAndInClip.GetTimeSamplesInInterval(
        Gf.Interval(0, 2.1)), [0.0, 1.0, 2.0])
    ValidateAttributeTimeSamples(inManifestAndInClip)

    # Close and reopen the stage to ensure the clip layers are closed
    # before we do the test below.
    stage.Close()
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_1.usda')))
    AssertFalse(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_2.usda')))

    # Lastly, this attribute is in the manifest but has no
    # samples in the clip, so we should just fall back to the default
    # value.
    stage = Usd.Stage.Open(
        FindDataFile('testUsdValueClips/manifest/root.usda'))
    prim = stage.GetPrimAtPath('/WithManifestClip')

    inManifestNotInClip = prim.GetAttribute('inManifestNotInClip')
    AssertFalse(inManifestNotInClip.ValueMightBeTimeVarying())
    # Since the attribute is in the manifest, we have to search all
    # the clips to see which of them have samples. In this case, none
    # of them do, so we fall back to the default value.
    Assert(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_1.usda')))
    Assert(Sdf.Layer.Find(
            FindDataFile('testUsdValueClips/manifest/clip_2.usda')))
    AssertEqual(inManifestNotInClip.Get(0), 2.0)
    AssertEqual(inManifestNotInClip.GetTimeSamples(), [])
    AssertEqual(inManifestNotInClip.GetTimeSamplesInInterval(
        Gf.Interval.GetFullInterval()), [])
    ValidateAttributeTimeSamples(inManifestNotInClip)

if __name__ == "__main__":
    TestBasicClipBehavior()
    TestClipTiming()
    TestClipTimingOutsideRange()
    TestClipsWithLayerOffsets()
    TestSingleClip()
    TestMultipleClips()
    TestMultipleClipsWithNoTimeSamples()
    TestMultipleClipsWithSomeTimeSamples()
    TestMultipleClipsWithSomeTimeSamples2()
    TestOverrideOfAncestralClips()
    TestClipFlatten()
    TestClipValidation()
    TestClipsOnNonModel()
    TestClipsCannotIntroduceNewTopology()
    TestClipAuthoring()
    TestClipManifest()
    TestClipStrengthOrdering()

    ExitTest()
