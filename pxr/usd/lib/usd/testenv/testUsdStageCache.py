#!/pxrpythonsubst

import sys
from pxr import Sdf,Usd,Tf,Ar
from Mentor.Runtime import (AssertEqual, AssertNotEqual, FindDataFile,
                            ExpectedErrors, ExpectedWarnings, RequiredException)

def CheckEmptyCache(cache):
    assert cache.IsEmpty() and cache.Size() == 0
    assert not cache.Find(Usd.StageCache.Id.FromLongInt(1))
    assert len(cache.GetAllStages()) == 0
    assert not cache.FindOneMatching(Sdf.Layer.CreateAnonymous())
    assert not cache.FindOneMatching(Sdf.Layer.CreateAnonymous(),
                                     Sdf.Layer.CreateAnonymous())

    context = Ar.GetResolver().CreateDefaultContext()
    assert not cache.FindOneMatching(Sdf.Layer.CreateAnonymous(),
                                     context)
    assert not cache.FindOneMatching(Sdf.Layer.CreateAnonymous(),
                                     Sdf.Layer.CreateAnonymous(),
                                     context)

    assert len(cache.FindAllMatching(Sdf.Layer.CreateAnonymous())) == 0
    assert len(cache.FindAllMatching(Sdf.Layer.CreateAnonymous(),
                                     Sdf.Layer.CreateAnonymous())) == 0
    assert len(cache.FindAllMatching(Sdf.Layer.CreateAnonymous(),
                                     context)) == 0
    assert len(cache.FindAllMatching(Sdf.Layer.CreateAnonymous(),
                                     Sdf.Layer.CreateAnonymous(),
                                     context)) == 0

    assert not cache.GetId(None)
    assert not cache.Erase(Usd.StageCache.Id())
    assert not cache.Erase(None)
    assert cache.EraseAll(Sdf.Layer.CreateAnonymous()) == 0
    assert cache.EraseAll(Sdf.Layer.CreateAnonymous(),
                          Sdf.Layer.CreateAnonymous()) == 0
    assert cache.EraseAll(Sdf.Layer.CreateAnonymous(),
                          Sdf.Layer.CreateAnonymous(),
                          context) == 0

def Basics():
    cache = Usd.StageCache()

    CheckEmptyCache(cache)

    with RequiredException(Tf.ErrorException):
        cache.Insert(None)

    CheckEmptyCache(cache)

    # Insert a single stage.
    stage = Usd.Stage.CreateInMemory()
    stageId = cache.Insert(stage)
    assert stageId
    assert not cache.IsEmpty() and cache.Size() == 1
    assert len(cache.GetAllStages()) == 1
    assert cache.GetAllStages()[0] == stage
    assert cache.Find(stageId) == stage
    assert cache.FindOneMatching(stage.GetRootLayer(),
                                 stage.GetSessionLayer(),
                                 stage.GetPathResolverContext()) == stage
    assert cache.FindOneMatching(stage.GetRootLayer()) == stage
    assert cache.FindOneMatching(stage.GetRootLayer(),
                                 stage.GetSessionLayer()) == stage
    assert cache.FindOneMatching(stage.GetRootLayer(),
                                 stage.GetPathResolverContext()) == stage
    
    assert cache.FindAllMatching(stage.GetRootLayer(),
                                 stage.GetSessionLayer(),
                                 stage.GetPathResolverContext()) == [stage]
    assert cache.FindAllMatching(stage.GetRootLayer()) == [stage]
    assert cache.FindAllMatching(stage.GetRootLayer(),
                                 stage.GetSessionLayer()) == [stage]
    assert cache.FindAllMatching(stage.GetRootLayer(),
                                 stage.GetPathResolverContext()) == [stage]

    assert cache.GetId(stage) == stageId

    assert cache.Erase(stageId)
    stage = Usd.Stage.CreateInMemory()
    cache.Insert(stage)
    assert cache.Erase(stage)
    stage = Usd.Stage.CreateInMemory()
    cache.Insert(stage)
    assert cache.EraseAll(stage.GetRootLayer()) == 1
    stage = Usd.Stage.CreateInMemory()
    cache.Insert(stage)
    assert cache.EraseAll(stage.GetRootLayer(),
                          stage.GetSessionLayer()) == 1
    stage = Usd.Stage.CreateInMemory()
    cache.Insert(stage)
    assert cache.EraseAll(stage.GetRootLayer(),
                          stage.GetSessionLayer(),
                          stage.GetPathResolverContext()) == 1
    stage = Usd.Stage.CreateInMemory()
    cache.Insert(stage)
    cache.Clear()
    assert cache.IsEmpty() and cache.Size() == 0

def FindByPartialKey():
    # Create a cache with multiple stages, try finding by various elements.
    cache = Usd.StageCache()

    # sameRoot1 and sameRoot2 share root layers.
    sameRoot1 = Usd.Stage.CreateInMemory()
    sameRoot2 = Usd.Stage.Open(sameRoot1.GetRootLayer())
    
    # same1 and same2 share both root and session layers and have null path
    # resolver contexts
    same1 = Usd.Stage.CreateInMemory()
    same2 = Usd.Stage.Open(same1.GetRootLayer(), same1.GetSessionLayer())

    # prDiff1 and prDiff2 share root and session layers, but have different path
    # resolver contexts.
    prDiff1 = Usd.Stage.CreateInMemory()

    assetFile = FindDataFile('testUsdStageCache/asset.usd')
    prDiff2 = Usd.Stage.Open(prDiff1.GetRootLayer(), prDiff1.GetSessionLayer(),
        Ar.GetResolver().CreateDefaultContextForAsset(assetFile))

    # Create a cache and insert all the above stages.
    allStages = [sameRoot1, sameRoot2, same1, same2, prDiff1, prDiff2]

    cache = Usd.StageCache()
    assert all(map(cache.Insert, allStages))

    # Now check finding them by partial key.
    def CheckMatching(args, expected):
        def makeIterable(x):
            try:
                iter(x)
                return x
            except TypeError:
                return (x,)
        args = makeIterable(args)
        assert sorted(cache.FindAllMatching(*args)) == sorted(expected)
        assert cache.FindOneMatching(*args) in expected
        
    CheckMatching(sameRoot1.GetRootLayer(), [sameRoot1, sameRoot2])
    CheckMatching((sameRoot1.GetRootLayer(), sameRoot1.GetSessionLayer()),
                  [sameRoot1])
    CheckMatching((sameRoot1.GetRootLayer(),
                   sameRoot1.GetPathResolverContext()),
                  [sameRoot1, sameRoot2])

    CheckMatching(same1.GetRootLayer(), [same1, same2])
    CheckMatching((same1.GetRootLayer(), same1.GetSessionLayer()),
                  [same1, same2])
    CheckMatching((same1.GetRootLayer(), same1.GetPathResolverContext()),
                  [same1, same2])

    CheckMatching(prDiff1.GetRootLayer(), [prDiff1, prDiff2])
    CheckMatching((prDiff1.GetRootLayer(), prDiff1.GetSessionLayer()),
                  [prDiff1, prDiff2])
    CheckMatching((prDiff1.GetRootLayer(), prDiff1.GetPathResolverContext()),
                  [prDiff1])

def StageIds():
    # Create a cache with multiple stages, try finding by various elements.
    cache = Usd.StageCache()

    # sameRoot1 and sameRoot2 share root layers.
    sameRoot1 = Usd.Stage.CreateInMemory()
    sameRoot2 = Usd.Stage.Open(sameRoot1.GetRootLayer())
    
    # same1 and same2 share both root and session layers and have null path
    # resolver contexts
    same1 = Usd.Stage.CreateInMemory()
    same2 = Usd.Stage.Open(same1.GetRootLayer(), same1.GetSessionLayer())

    # prDiff1 and prDiff2 share root and session layers, but have different path
    # resolver contexts.
    prDiff1 = Usd.Stage.CreateInMemory()

    assetFile = FindDataFile('testUsdStageCache/asset.usd')
    prDiff2 = Usd.Stage.Open(prDiff1.GetRootLayer(), prDiff1.GetSessionLayer(),
        Ar.GetResolver().CreateDefaultContextForAsset(assetFile))

    # Create a cache and insert all the above stages.
    allStages = [sameRoot1, sameRoot2, same1, same2, prDiff1, prDiff2]

    cache = Usd.StageCache()

    ids = map(cache.Insert, allStages)
    assert all(ids)

    for stage, i in zip(allStages, ids):
        assert cache.GetId(stage) == i
        assert cache.Find(i) == stage
        # round trip from/to string and int.
        assert cache.Find(Usd.StageCache.Id.FromLongInt(i.ToLongInt())) == stage
        assert cache.Find(Usd.StageCache.Id.FromString(i.ToString())) == stage

    assert all(map(cache.Erase, ids))
    assert cache.IsEmpty() and cache.Size() == 0
    

def CacheContext():
    layer1 = Sdf.Layer.CreateAnonymous()
    layer2 = Sdf.Layer.CreateAnonymous()

    cache1 = Usd.StageCache()

    # Populate cache1 with a stage, by binding a context and using the
    # Stage.Open API.
    with Usd.StageCacheContext(cache1):
        stage = Usd.Stage.Open(layer1)

    assert cache1.Size() == 1
    assert cache1.Contains(stage)
    assert cache1.FindOneMatching(layer1) == stage

    # Read the stage from the read-only cache, assert a different opened stage
    # doesn't populate the cache.
    with Usd.StageCacheContext(Usd.UseButDoNotPopulateCache(cache1)):
        stageAgain = Usd.Stage.Open(layer1)
        newStage = Usd.Stage.Open(layer2)
        
    assert stageAgain is stage
    assert newStage
    assert not cache1.Contains(newStage)

    # Create a new cache, make a context for both, and check that a newly
    # created stage publishes to both caches.
    cache1.Clear()
    cache2 = Usd.StageCache()
    with Usd.StageCacheContext(cache1):
        with Usd.StageCacheContext(cache2):
            newStage = Usd.Stage.Open(layer2)

    assert cache1.Contains(newStage)
    assert cache2.Contains(newStage)

    # Publish a stage with a specific session layer to a cache, then check that
    # a call to Stage.Open() that doesn't specify a session layer finds that
    # layer in the cache, but that a call to Stage.Open() that demands no
    # session layer finds no layer in the cache.
    cache1.Clear()
    with Usd.StageCacheContext(cache1):
        newStage = Usd.Stage.Open(layer1, layer2)
        newStage2 = Usd.Stage.Open(layer1)
        assert newStage is newStage2
        newStage3 = Usd.Stage.Open(layer1, sessionLayer=None)
        assert newStage3 != newStage

    # Verify that blocking caches works as expected.
    cache1.Clear()
    with Usd.StageCacheContext(cache1):
        # Populate a stage into the cache.
        newStage = Usd.Stage.Open(layer1)
        assert cache1.Contains(newStage)
        with Usd.StageCacheContext(Usd.BlockStageCaches):
            # Open() should create a new stage, since cache is blocked.
            newStage2 = Usd.Stage.Open(layer1)
            assert newStage2 != newStage
            assert cache1.Size() == 1
            # Opening a different stage should not populate the cache.
            newStage3 = Usd.Stage.Open(layer2)
            assert not cache1.Contains(newStage3)
            assert cache1.Size() == 1

    # Try blocking cache writes only.
    cache1.Clear()
    with Usd.StageCacheContext(cache1):
        # Populate a stage into the cache.
        newStage = Usd.Stage.Open(layer1)
        assert cache1.Contains(newStage)
        with Usd.StageCacheContext(Usd.BlockStageCachePopulation):
            # Open() should read the stage from the cache.
            newStage2 = Usd.Stage.Open(layer1)
            assert newStage2 == newStage
            # Opening a different stage should not populate the cache.
            newStage3 = Usd.Stage.Open(layer2)
            assert not cache1.Contains(newStage3)
            assert cache1.Size() == 1
            
def CacheContextLifetime():
    # Check that python object lifetimes are preserved as expected, to ensure
    # that the C++ objects they refer to do not expire prematurely.
    import weakref

    # Create a cache and a weak reference to it.
    cache = Usd.StageCache()
    weakCache = weakref.ref(cache)

    # Create a nonpopulating wrapper around the cache, and a weakref to it.
    nonPop = Usd.UseButDoNotPopulateCache(cache)
    weakNonPop = weakref.ref(nonPop)

    # del our local name 'cache'.  The rw object should keep it alive.
    del cache
    assert weakCache()

    # Create a CacheContext.
    ctx = Usd.StageCacheContext(nonPop)

    # del our local name 'rw'.  The ctx object should keep it alive (and
    # transitively, the cache object).
    del nonPop
    assert weakNonPop()
    assert weakCache()

    # Try populating into the cache, for fun.
    with ctx:
        stage = Usd.Stage.Open(Sdf.Layer.CreateAnonymous())
    
    assert weakNonPop()
    assert weakCache()
    assert not weakCache().Contains(stage) # nonpopulating cache usage.

    # Killing our ctx reference should let the objects expire.
    del ctx
    assert weakNonPop() is None
    assert weakCache() is None

    # Now try again with a context on the cache directly.
    cache = Usd.StageCache()
    weakCache = weakref.ref(cache)
    ctx = Usd.StageCacheContext(cache)
    del cache
    assert weakCache()
    with ctx:
        stage = Usd.Stage.Open(Sdf.Layer.CreateAnonymous())
    assert weakCache()
    assert weakCache().Contains(stage) # this ctx will populate.
    # Killing ctx should let the cache expire.
    del ctx
    assert weakCache() is None


def ImplicitSessionLayer():
    cache = Usd.StageCache()
    layer1 = Sdf.Layer.CreateAnonymous()
    sess = Sdf.Layer.CreateAnonymous()

    # Opening a stage that is not already present in the cache, specifying no
    # session layer should actually produce a stage with no session layer.
    with Usd.StageCacheContext(cache):
        implicitSession = Usd.Stage.Open(layer1)
        noSession = Usd.Stage.Open(layer1, sessionLayer=None)
        explicitSession = Usd.Stage.Open(layer1, sessionLayer=sess)
        dontCareSession = Usd.Stage.Open(layer1)

    # The implicitSession stage should have a usd-generated session layer, and
    # should not match explicitSession's session layer.
    assert implicitSession.GetSessionLayer()
    assert (implicitSession.GetSessionLayer() !=
            explicitSession.GetSessionLayer())

    # The noSession stage should have no session layer, and should not be the
    # same stage as either explicitSession or implicitSession.
    assert not noSession.GetSessionLayer()
    assert noSession not in (explicitSession, implicitSession)

    # The explicitSession stage should have the session layer we specified, and
    # should not be the same as noSession or implicitSession.
    assert explicitSession.GetSessionLayer() == sess
    assert explicitSession not in (noSession, implicitSession)

    # The dontCareSession should be either noSession or explicitSession or
    # implicitSession, since it didn't specify a sessionLayer argument.
    assert dontCareSession in (noSession, explicitSession, implicitSession)
    
def Main(argv):
    Basics()
    FindByPartialKey()
    StageIds()
    CacheContext()
    CacheContextLifetime()
    ImplicitSessionLayer()

if __name__ == "__main__":
    Main(sys.argv)
    print 'OK'

