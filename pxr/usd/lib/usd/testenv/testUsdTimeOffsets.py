#!/pxrpythonsubst

import sys, os
from pxr import Sdf, Usd, Pcp, Vt, Tf

import Mentor.Runtime
from Mentor.Runtime import (AssertEqual, AssertClose, FindDataFile)

allFormats = ['usd' + x for x in 'abc']

# --------------------------------------------------------------------------- #
# Support code for generating & verifying offsets
# --------------------------------------------------------------------------- #

class AdjustedPrim(object):
    """A convenience object for bundling up a prim,stage and offset.
    The stage is only held here to keep the reference alive."""
    stage = None
    prim = None
    layerOffset = None


def MakePrim(stage, refLyr, path, offset, scale, matchPath=False):
    """Creates a reference at the given path and applies the offset and scale,
    if matchPath is true, it assumes the reference path is the same as the
    local path, otherwise </Foo> is used.
    """
    p = AdjustedPrim()
    p.prim = stage.OverridePrim(path)
    p.stage = stage
    p.layerOffset = Sdf.LayerOffset(offset, scale)

    refPath = "/Foo"
    if matchPath:
        refPath = path

    ref = Sdf.Reference(refLyr.identifier, refPath, p.layerOffset)
    assert p.prim.GetReferences().Add(ref)
    return p


def GenTestLayer(testId, fmt):
    """Generates a layer with three time samples at 1.0, 2.0, and 10.0
    at </Foo.attr>.
    """
    l = Sdf.Layer.CreateNew("sourceData-" + testId + "." + fmt)
    stage = Usd.Stage.Open(l)

    stage.OverridePrim("/Foo")
    foo = stage.GetPrimAtPath("/Foo")
    attr = foo.CreateAttribute("attr", Sdf.ValueTypeNames.Float)
    attr.Set(1.0, 1.0)
    attr.Set(2.0, 2.0)
    attr.Set(10.0, 10.0)

    # original:
    #         1.  2.                              10.
    # --------|---|-------------------------------|----
    # t:  0   1   2   3   4   5   6   7   8   9   10

    return l


def VerifyOffset(adjPrim):
    prim = adjPrim.prim
    offset = adjPrim.layerOffset.offset
    scale = adjPrim.layerOffset.scale

    print "Testing offset:", 
    print offset, "scale:", scale, "prim:", adjPrim.prim.GetPath()

    inverse = adjPrim.layerOffset.GetInverse()

    # When offset=1.0:
    #
    #     1.  2.                              10.
    # ----|---|-------------------------------|-------
    # t:  0   1   2   3   4   5   6   7   8   9   10
    #
    # So we expect value( inverse * t ) = original_t
    #
    expectedTimes = []
    attr = prim.GetAttribute("attr")
    for t in (1.,2.,10.):
        AssertEqual(t, attr.Get(inverse * t))
        expectedTimes.append(inverse * t)

    print "    Expected Times:", tuple(expectedTimes)
    print "    Authored Times:", attr.GetTimeSamples()
    AssertClose(tuple(expectedTimes), attr.GetTimeSamples())

    for t in (1.,2.,10.):
        stageTime = inverse * t
        print "    Bracketing time samples for t=%s: %s" \
            % (stageTime, attr.GetBracketingTimeSamples(stageTime))
        AssertEqual((stageTime, stageTime),
                    attr.GetBracketingTimeSamples(stageTime))

    stageTime = inverse * 0.
    print "    Bracketing time samples for t=%s: %s" \
        % (stageTime, attr.GetBracketingTimeSamples(stageTime))
    AssertEqual((inverse * 1., inverse * 1.),
                attr.GetBracketingTimeSamples(stageTime))

    for (lo, hi) in [(1., 2.), (2., 10.)]:
        stageTime = ((inverse * lo) + (inverse * hi)) / 2.
        print "    Bracketing time samples for t=%s: %s" \
            % (stageTime, attr.GetBracketingTimeSamples(stageTime))
        AssertEqual(((inverse * lo), (inverse *hi)),
                    attr.GetBracketingTimeSamples(stageTime))
        
    stageTime = inverse * 11.
    print "    Bracketing time samples for t=%s: %s" \
        % (stageTime, attr.GetBracketingTimeSamples(stageTime))
    AssertEqual((inverse * 10., inverse * 10.),
                attr.GetBracketingTimeSamples(stageTime))


def BuildReferenceOffsets(rootLyr, testLyr):
    stage = Usd.Stage.Open(rootLyr)

    cases = [
        #
        # Single offset or scale tests:
        #
        ('/Identity', 0.0, 1.0),
        ('/Offset_1', 1.0, 1.0),
        ('/Offset_neg1', -1.0, 1.0),
        ('/Offset_7', 7.0, 1.0),
        ('/Offset_neg7', -7.0, 1.0),
        ('/Scale_2', 0.0, 2.0),
        ('/Scale_1p5', 0.0, 1.5),
        ('/Scale_half', 0.0, 0.5),
        ('/Scale_negHalf', 0.0, -0.5),

        #
        # Combined offset and scale tests:
        #
        ('/Scale_half_Offset_1', 1.0, 0.5),
        ('/Scale_half_Offset_neg1', -1.0, 0.5),
        ('/Scale_negHalf_Offset_1', 1.0, -0.5),
        ('/Scale_negHalf_Offset_neg1', -1.0, -0.5)
        ]

    adjPrims = [MakePrim(stage, testLyr, path=c[0], offset=c[1], scale=c[2])
                for c in cases]

    rootLyr.Save()
    testLyr.Save()

    return adjPrims


def BuildNestedReferenceOffsets(adjustedPrims, rootLyr, refLyr):
    adjPrims = []
    stage = Usd.Stage.Open(rootLyr)

    for p in adjustedPrims:
        if not p.prim.GetPath().IsRootPrimPath():
            continue
        offset = p.layerOffset.offset
        scale = p.layerOffset.scale
        adjPrim = MakePrim(stage, refLyr, p.prim.GetPath(),
                           offset, scale, matchPath=True)
        #
        # When nesting offsets, we need to combine the underlying offset with
        # ours so the verification code works. We're assuming that
        # SdLayerOffset operators are tested elsewhere.
        #
        adjPrim.layerOffset = adjPrim.layerOffset * p.layerOffset
        adjPrims += [adjPrim]

    rootLyr.Save()
    refLyr.Save()
    return adjPrims


# --------------------------------------------------------------------------- #
# Test methods
# --------------------------------------------------------------------------- #

def TestReferenceOffsets():
    for fmt in allFormats:
        testLyr = GenTestLayer("TestReferenceOffsets", fmt)
        rootLyr = Sdf.Layer.CreateNew("TestReferenceOffsets."+fmt)
        nestedRootLyr = Sdf.Layer.CreateNew("TestReferenceOffsetsNested."+fmt)

        print "-"*80
        print "Testing flat offsets:"
        print "-"*80
        adjPrims = BuildReferenceOffsets(rootLyr, testLyr)
        for adjPrim in adjPrims:
            VerifyOffset(adjPrim)

        print
        print "-"*80
        print "Testing nested offsets:"
        print "-"*80
        for adjPrim in BuildNestedReferenceOffsets(
            adjPrims, nestedRootLyr, rootLyr):
            VerifyOffset(adjPrim)

def TestOffsetsAuthoring():
    for fmt in allFormats:
        # Create a simple structure one rootLayer with one subLayer, a prim
        # 'Foo' in the rootLayer that references 'Bar' defined in refLayer.
        # Then we assign a layer offset to the reference and to the sublayer,
        # and we test authoring a time sample into the reference via an
        # EditTarget, as well as to the subLayer.  In both cases we check that
        # the time value was correctly transformed.
        rootLayer = Sdf.Layer.CreateAnonymous('root.'+fmt)
        subLayer = Sdf.Layer.CreateAnonymous('sub.'+fmt)
        refLayer = Sdf.Layer.CreateAnonymous('ref.'+fmt)

        # add subLayer to rootLayer and give it a layer offset.
        subOffset = Sdf.LayerOffset(scale=3.0, offset=4.0)
        rootLayer.subLayerPaths.append(subLayer.identifier)
        rootLayer.subLayerOffsets[0] = subOffset

        # add Foo root prim.
        fooRoot = Sdf.PrimSpec(rootLayer, 'Foo', Sdf.SpecifierDef)

        # add Bar target prim in refLayer.
        barRef = Sdf.PrimSpec(refLayer, 'Bar', Sdf.SpecifierDef)

        # make Foo reference Bar.
        refOffset = Sdf.LayerOffset(scale=2.0, offset=1.0)
        fooRoot.referenceList.Add(Sdf.Reference(refLayer.identifier,
                                                barRef.path, refOffset))

        # Create a UsdStage, get 'Foo'.
        stage = Usd.Stage.Open(rootLayer)
        foo = stage.GetPrimAtPath('/Foo')

        # Make an EditTarget to author into the referenced Bar.
        editTarget = Usd.EditTarget(refLayer,
                                    foo.GetPrimIndex().rootNode.children[0])
        with Usd.EditContext(stage, editTarget):
            attr = foo.CreateAttribute('attr', Sdf.ValueTypeNames.Double)
            attr.Set(1.0, time=2.0)
            assert attr.Get(time=2.0) == 1.0, (
                'expected value 1.0 at time=2.0, got %s' % attr.Get(time=2.0))
            # Check that the time value in the reference is correctly
            # transformed.
            authoredTime = barRef.attributes[
                'attr'].GetInfo('timeSamples').keys()[0]
            assert refOffset.GetInverse() * 2.0 == authoredTime

        # Make an EditTarget to author into the sublayer.
        editTarget = Usd.EditTarget(subLayer)
        with Usd.EditContext(stage, editTarget):
            attr = foo.GetAttribute('attr')
            attr.Set(1.0, time=2.0)
            assert attr.Get(time=2.0) == 1.0, (
                'expected value 1.0 at time=2.0, got %s' % attr.Get(time=2.0))
            # Check that the time value in the sublayer is correctly
            # transformed.
            authoredTime = subLayer.GetAttributeAtPath(
                '/Foo.attr').GetInfo('timeSamples').keys()[0]
            # XXX: The usdb format does not exactly preserve floating point
            # values.
            if fmt == 'usdb':
                AssertClose(subOffset.GetInverse() * 2.0, authoredTime)
            else:
                AssertEqual(subOffset.GetInverse() * 2.0, authoredTime)

if __name__ == '__main__':
    Mentor.Runtime.SetAssertMode(Mentor.Runtime.MTR_EXIT_TEST)
    TestReferenceOffsets()
    TestOffsetsAuthoring()

    # TODO:
    # Test authoring values across offsets. The code is present in
    # UsdStage::_SetValue, however the API to change the stage's authoring
    # layer does not yet exist.

    # TODO:
    # TestSublayerOffsets()
    # TestSublayerAndReferenceOffsets()
    # Note: Could set sublayers like this, until the API is done: 
    #       rootLyr.subLayerPaths = [refLyr.identifier] 

    # TODO (maybe in a combine test):
    # TestSublayerAndReferenceOffsetsWithVariants()
    # TestSublayerAndReferenceOffsetsWithClasses()
    # TestSublayerAndReferenceOffsetsWithClassesAndVariants()

    print 'OK'

