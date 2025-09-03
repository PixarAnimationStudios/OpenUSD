#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Usd 

startTime = 101
endTime   = 120

# Generate a stage with a default attribute as well as an 
# attribute with time samples authored on it. These are the 
# two cases in which we can author blocks
def CreateTestAssets(fileName):
    stage = Usd.Stage.CreateNew(fileName)
    prim = stage.DefinePrim("/Sphere")
    defAttr = prim.CreateAttribute("size", Sdf.ValueTypeNames.Double, True)
    defAttr.Set(1.0)
    sampleAttr = prim.CreateAttribute("points", Sdf.ValueTypeNames.Double, False)
    for sample in range(startTime, endTime):
        sampleAttr.Set(sample, sample)

    return stage, defAttr, sampleAttr

# Test blocking of default value through the time-sample-wiping API
# Ensures that all samples are blown away upon calling Block()
def TestBlock(sampleAttr, defAttr):
    assert sampleAttr.GetNumTimeSamples() != 0
    assert not sampleAttr.GetResolveInfo().ValueIsBlocked()
    sampleAttr.Block()
    assert sampleAttr.GetNumTimeSamples() == 0
    assert sampleAttr.GetResolveInfo().ValueIsBlocked()
    assert not sampleAttr.Get()
    for sample in range(startTime, endTime):
        assert not sampleAttr.Get(sample)

# Test blocking of individual time samples
# Ensures users are able to pass the sentinel value through 
# for fine grained control over attribute blocking.
def TestIndividualTimeSampleBlocking(sampleAttr, defAttr):
    for sample in range(startTime, endTime):
        sampleAttr.Set(Sdf.ValueBlock(), sample)
        assert sampleAttr.Get(sample) is None
        # Time sample blocking is different than whole-value blocking
        assert not sampleAttr.GetResolveInfo().ValueIsBlocked()

# Ensure that passing the empty time code through works as expected
def TestDefaultValueBlocking(sampleAttr, defAttr):
    assert defAttr.Get()
    assert not defAttr.GetResolveInfo().ValueIsBlocked()
    defAttr.Set(Sdf.ValueBlock())
    assert not defAttr.Get()
    assert defAttr.GetResolveInfo().ValueIsBlocked()


def CreateTestAssetsForAnimationBlock(fileName):
    # Create the weakest layer first
    weakerLayer = Sdf.Layer.CreateAnonymous("animationBlocks_weaker")
    weakerLayer.ImportFromString("""#usda 1.0
over "Human"
{
    int c = 1
    double d = 2.0
}
""")

    # Next, create the weak middle layer
    weakLayer = Sdf.Layer.CreateAnonymous("animationBlocks_weak")
    weakLayer.ImportFromString("""#usda 1.0
over "Human"
{
    int a = AnimationBlock
    int a.timeSamples = {
        1: 5,
        2: 18,
    }

    double b.spline = {
        1: 5; post held,
        2: 18; post held,
    }

    int c.timeSamples = {
        0: 456,
        1: 789
    }

    double d.spline = {
        1: 5; post held,
        2: 18; post held,
    }
}
""")

    # Then the strongest layer
    strongLayer = Sdf.Layer.CreateAnonymous("animationBlocks_strong")
    strongLayer.ImportFromString("""#usda 1.0
def Xform "Human"
{
    double b = AnimationBlock
    double b.spline = {
        1: 10; post held,
        2: 20; post held,
    }

    int c = AnimationBlock

    double d = AnimationBlock

    double e = AnimationBlock
}
""")

    # Finally, the root layer that sublayers the above
    rootLayer = Sdf.Layer.CreateAnonymous(fileName)
    rootLayer.subLayerPaths = [
        strongLayer.identifier,
        weakLayer.identifier,
        weakerLayer.identifier
    ]

    # Create the UsdStage from the root layer
    stage = Usd.Stage.Open(rootLayer)
    return stage

def TestAnimationBlock(stage):
    prim = stage.GetPrimAtPath("/Human")
    # Since attribute "a"'s strongest time samples are not blocked by an
    # animation block, its time samples shine through. Also even though it has a
    # default animation block, but its weaker and hence doesn't affect its
    # stronger time samples.
    # do also note that default Animation block in the same layer, doesn't
    # affect time samples in the same layer, time samples still win.
    a = prim.GetAttribute("a")
    # source is time samples
    assert (a.GetResolveInfo().GetSource() == Usd.ResolveInfoSourceTimeSamples)
    # only default is animation block
    assert (a.Get() is None)
    # time samples shine through
    assert (a.Get(1) == 5.0)

    # Since attribute "b"'s strongest spline values are not blocked by an
    # animation block, its spline values shine through. Also even though it has
    # a default animation block, but its weaker and hence doesn't affect its
    # strongest spline values.
    # do also note that default Animation block in the same stronger layer, 
    # doesn't affect spline values in the same layer, splines still win.
    b = prim.GetAttribute("b")
    # source is spline
    assert (b.GetResolveInfo().GetSource() == Usd.ResolveInfoSourceSpline)
    # default is animation block
    assert (b.Get() is None)
    # stronger spline value shine through (and not the weaker spline or
    # animation block)
    assert (b.Get(1) == 10.0)

    # Since attribute "c"'s strongest value is an Animation Block, it blocks any
    # time sample, and results in any non-animation block default value to 
    # shine through from the weaker layer.
    c = prim.GetAttribute("c")
    # source is default
    assert (c.GetResolveInfo().GetSource() == Usd.ResolveInfoSourceDefault)
    # default is 1 and not animation block
    assert (c.Get() == 1)
    # time sample is blocked and default shines through
    assert (c.Get(1) == 1)

    # Since attribute "d"'s strongest value is an Animation Block, it blocks any
    # spline, and results in any non-animation block default value to shine
    # through from the weaker layer.
    d = prim.GetAttribute("d")
    # source is default
    assert (d.GetResolveInfo().GetSource() == Usd.ResolveInfoSourceDefault)
    # default is 2.0 and not animation block
    assert (d.Get() == 2.0)
    # spline is blocked and default shines through
    assert (d.Get(1) == 2.0)

    #Attr with just animation block, we should get an empty default value with
    # resolve info source as None
    e = prim.GetAttribute("e")
    # source is None
    assert (e.GetResolveInfo().GetSource() == Usd.ResolveInfoSourceNone)
    # default should return None
    assert (e.Get() is None)

if __name__ == '__main__':
    # We ensure that this is supported on all file formats
    formats = [".usda", ".usdc"]

    for fmt in formats:
        stage, defAttr, sampleAttr = CreateTestAssets('test' + fmt)
        TestBlock(sampleAttr, defAttr)
        del stage, defAttr, sampleAttr

        stage, defAttr, sampleAttr = CreateTestAssets('test' + fmt)
        TestIndividualTimeSampleBlocking(sampleAttr, defAttr)
        del stage, defAttr, sampleAttr

        stage, defAttr, sampleAttr = CreateTestAssets('test' + fmt)
        TestDefaultValueBlocking(sampleAttr, defAttr)
        del stage, defAttr, sampleAttr

        stage = CreateTestAssetsForAnimationBlock('test' + fmt)
        TestAnimationBlock(stage)
        del stage
