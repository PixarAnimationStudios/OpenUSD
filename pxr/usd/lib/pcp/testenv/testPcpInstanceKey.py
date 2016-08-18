#!/pxrpythonsubst

from pxr import Pcp, Sdf
from Mentor.Runtime import (AssertEqual, AssertNotEqual, 
                            ExitTest, FindDataFile)

def LoadPcpCache(layerPath):
    rootLayer = Sdf.Layer.FindOrOpen(FindDataFile(layerPath))
    cache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer), usd=True)
    return cache

def GetInstanceKey(cache, primPath):
    (pi, err) = cache.ComputePrimIndex(primPath)
    AssertEqual(err, [])

    key = Pcp.InstanceKey(pi)
    AssertEqual(key, key)
    if pi.IsInstanceable():
        AssertNotEqual(key, Pcp.InstanceKey())
    else:
        AssertEqual(key, Pcp.InstanceKey())

    print "Pcp.InstanceKey('%s'): " % primPath
    print key, "\n"
    return key

def TestDefault():
    """Test default constructed (invalid) instance key for 
    code coverage"""
    invalidKey = Pcp.InstanceKey()
    AssertEqual(invalidKey, invalidKey)
    print "Pcp.InstanceKey(): "
    print invalidKey

def TestBasic():
    """Test instance key functionality on simple
    asset structure including references and inherits"""
    cache = LoadPcpCache('testPcpInstanceKey.testenv/basic.usda')

    prop1Key = GetInstanceKey(cache, '/Set_1/Prop_1')
    prop2Key = GetInstanceKey(cache, '/Set_1/Prop_2')
    prop3Key = GetInstanceKey(cache, '/Set_1/Prop_3')

    AssertEqual(prop1Key, prop2Key)
    AssertNotEqual(prop1Key, prop3Key)
    AssertNotEqual(prop2Key, prop3Key)

    # Even though /NotAnInstance is tagged with instance = True,
    # it does not introduce any instance-able data via a composition arc
    # and is not treated as a real instance. Thus, it's instance key
    # is empty.
    notAnInstanceKey = GetInstanceKey(cache, '/NotAnInstance')
    AssertEqual(notAnInstanceKey, Pcp.InstanceKey())

def TestVariants():
    """Test instance key functionality on asset
    structure involving references and variants."""
    cache = LoadPcpCache('testPcpInstanceKey.testenv/variants.usda')

    key1 = GetInstanceKey(cache, '/Model_1')
    key2 = GetInstanceKey(cache, '/Model_2')
    key3 = GetInstanceKey(cache, '/Model_3')
    key4 = GetInstanceKey(cache, '/Model_4')
    key5 = GetInstanceKey(cache, '/Model_5')
    key6 = GetInstanceKey(cache, '/Model_6')

    # Model_1, 2, and 3 should all have the same instance key because
    # they share the same reference and the same composed variant selection,
    # even though 1 and 2 have the variant selection authored locally and
    # 3 has the selection authored within the reference.
    AssertEqual(key1, key2)
    AssertEqual(key1, key3)

    # Model_4, 5, and 6 have different variant selections and composition
    # arcs, so they should have different keys.
    AssertNotEqual(key1, key4)
    AssertNotEqual(key1, key5)
    AssertNotEqual(key1, key6)

    # Model_5 and 6 have locally-defined variant sets, but this doesn't
    # matter for the instance key because those variant sets can't
    # have opinions on name children. This means that composing values
    # under any descendants of Model_5 and 6 will have the same value,
    # so long as they both have the same variant selection.
    AssertEqual(key5, key6)

if __name__ == "__main__":
    TestDefault()
    TestBasic()
    TestVariants()
    ExitTest()
