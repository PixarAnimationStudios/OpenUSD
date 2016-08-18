#!/pxrpythonsubst

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
    sampleAttr.Block()
    assert sampleAttr.GetNumTimeSamples() == 0
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

# Ensure that passing the empty time code through works as expected
def TestDefaultValueBlocking(sampleAttr, defAttr):
    assert defAttr.Get()
    defAttr.Set(Sdf.ValueBlock())
    assert not defAttr.Get()

if __name__ == '__main__':
    # We ensure that this is supported on all file formats
    formats = [".usda", ".usdb", ".usdc"]

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
