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

from pxr import Tf, Usd, UsdGeom, Gf
import sys

# Direct TF_DEBUG output to stderr.
Tf.Debug.SetOutputFile(sys.__stderr__)

def AssertBBoxesClose(cachedBox, directBox, msg):
    cachedRange = cachedBox.ComputeAlignedRange()
    directRange = directBox.ComputeAlignedRange()
    assert Gf.IsClose(cachedRange.min, directRange.min, 1e-5), msg
    assert Gf.IsClose(cachedRange.max, directRange.max, 1e-5), msg

def TestAtCurTime(stage, bboxCache):
    p = stage.GetPrimAtPath("/parent/primWithLocalXform")
    assert bboxCache.ComputeWorldBound(p) == bboxCache.ComputeWorldBound(p)
    bboxCache.SetIncludedPurposes([UsdGeom.Tokens.default_])
    print
    print "Untransformed bound:", p
    print bboxCache.ComputeUntransformedBound(p)
    print
    print bboxCache.ComputeUntransformedBound(p).ComputeAlignedRange()
    print
    # The baseline is predicated on TfDebug output.  We do not care to clutter
    # it up with the duplicate computation that calling the UsdGeom.Imageable
    # API will induce, so here and below we disable TfDebug output while
    # testing
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    db = UsdGeom.Imageable(p).ComputeUntransformedBound(bboxCache.GetTime(),
                                                        UsdGeom.Tokens.default_)
    AssertBBoxesClose(bboxCache.ComputeUntransformedBound(p), db, "Untransformed")
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 1)
                      
    print
    print "Local bound:", p
    print bboxCache.ComputeLocalBound(p)
    print
    print bboxCache.ComputeLocalBound(p).ComputeAlignedRange()
    print
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    db = UsdGeom.Imageable(p).ComputeLocalBound(bboxCache.GetTime(),
                                                UsdGeom.Tokens.default_)
    AssertBBoxesClose(bboxCache.ComputeLocalBound(p), db, "Local")
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 1)

    print
    print "World bound:", p
    print bboxCache.ComputeWorldBound(p)
    print
    print bboxCache.ComputeWorldBound(p).ComputeAlignedRange()
    print
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    db = UsdGeom.Imageable(p).ComputeWorldBound(bboxCache.GetTime(),
                                                UsdGeom.Tokens.default_)
    AssertBBoxesClose(bboxCache.ComputeWorldBound(p), db, "World")
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 1)

    print
    pp = stage.GetPrimAtPath(str(p.GetPath()) + "/InvisibleChild")
    print "Invisible Bound:", pp
    print bboxCache.ComputeWorldBound(pp)
    print
    print bboxCache.ComputeWorldBound(pp).ComputeAlignedRange()
    print
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    db = UsdGeom.Imageable(pp).ComputeWorldBound(bboxCache.GetTime(),
                                                 UsdGeom.Tokens.default_)
    AssertBBoxesClose(bboxCache.ComputeWorldBound(pp), db, "Invis World")
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 1)


    print
    print "Visit Guides:", p
    bboxCache.SetIncludedPurposes([UsdGeom.Tokens.guide])
    assert bboxCache.GetIncludedPurposes() == [UsdGeom.Tokens.guide]
    print bboxCache.ComputeWorldBound(p)
    print
    print bboxCache.ComputeWorldBound(p).ComputeAlignedRange()
    print
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    db = UsdGeom.Imageable(p).ComputeWorldBound(bboxCache.GetTime(),
                                                UsdGeom.Tokens.guide)
    AssertBBoxesClose(bboxCache.ComputeWorldBound(p), db, "World Guide")
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 1)


    print
    print "Visit Render:", p
    bboxCache.SetIncludedPurposes([UsdGeom.Tokens.render])
    assert bboxCache.GetIncludedPurposes() == [UsdGeom.Tokens.render]
    print bboxCache.ComputeWorldBound(p)
    print
    print bboxCache.ComputeWorldBound(p).ComputeAlignedRange()
    print
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    db = UsdGeom.Imageable(p).ComputeWorldBound(bboxCache.GetTime(),
                                                UsdGeom.Tokens.render)
    AssertBBoxesClose(bboxCache.ComputeWorldBound(p), db, "World Render")
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 1)


    print
    print "Visit Proxy:", p
    bboxCache.SetIncludedPurposes([UsdGeom.Tokens.proxy])
    assert bboxCache.GetIncludedPurposes() == [UsdGeom.Tokens.proxy]
    print bboxCache.ComputeWorldBound(p)
    print
    print bboxCache.ComputeWorldBound(p).ComputeAlignedRange()
    print
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    db = UsdGeom.Imageable(p).ComputeWorldBound(bboxCache.GetTime(),
                                                UsdGeom.Tokens.proxy)
    AssertBBoxesClose(bboxCache.ComputeWorldBound(p), db, "World Proxy")

    # Test multi-purpose
    bboxCache.SetIncludedPurposes([UsdGeom.Tokens.default_,
                                   UsdGeom.Tokens.proxy,
                                   UsdGeom.Tokens.render])
    assert bboxCache.GetIncludedPurposes() == [UsdGeom.Tokens.default_,
                                               UsdGeom.Tokens.proxy,
                                               UsdGeom.Tokens.render]
    db = UsdGeom.Imageable(p).ComputeWorldBound(bboxCache.GetTime(),
                                                UsdGeom.Tokens.default_,
                                                UsdGeom.Tokens.proxy,
                                                UsdGeom.Tokens.render)
    AssertBBoxesClose(bboxCache.ComputeWorldBound(p), db, "Multi-purpose")
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 1)


    p = stage.GetPrimAtPath("/Rotated")
    print
    print "Visit Rotated:", p
    bboxCache.SetIncludedPurposes([UsdGeom.Tokens.default_])
    assert bboxCache.GetIncludedPurposes() == [UsdGeom.Tokens.default_]
    print bboxCache.ComputeWorldBound(p)
    print
    print bboxCache.ComputeWorldBound(p).ComputeAlignedRange()
    print
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    db = UsdGeom.Imageable(p).ComputeWorldBound(bboxCache.GetTime(),
                                                UsdGeom.Tokens.default_)
    AssertBBoxesClose(bboxCache.ComputeWorldBound(p), db, "Rotated")
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 1)


    p = stage.GetPrimAtPath("/Rotated/Rotate135AndTranslate/Rot45")
    print
    print "Visit Rotated:", p
    bboxCache.SetIncludedPurposes([UsdGeom.Tokens.default_])
    assert bboxCache.GetIncludedPurposes() == [UsdGeom.Tokens.default_]
    print bboxCache.ComputeWorldBound(p)
    print
    print bboxCache.ComputeWorldBound(p).ComputeAlignedRange()
    print
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    db = UsdGeom.Imageable(p).ComputeWorldBound(bboxCache.GetTime(),
                                                UsdGeom.Tokens.default_)
    AssertBBoxesClose(bboxCache.ComputeWorldBound(p), db, "Rotated Twice")
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 1)

    p = Usd.Prim()
    assert not p 
    
    try:
        bboxCache.ComputeWorldBound(p)
    except RuntimeError:
        pass
    else:
        assert False, 'Failed to raise expected exception.'
    
    try:
        bboxCache.ComputeLocalBound(p)
    except RuntimeError:
        pass
    else:
       assert False, 'Failed to raise expected exception.'

    try:
        bboxCache.ComputeUntransformedBound(p)
    except RuntimeError:
        pass
    else:
        assert False, 'Failed to raise expected exception.'

def Main():
    stage = Usd.Stage.Open("cubeSbdv.usda")

    bboxCache = UsdGeom.BBoxCache(Usd.TimeCode.Default(), 
                                  includedPurposes=[UsdGeom.Tokens.default_])
    print "-"*80
    print "Running tests at UsdTimeCode::Default()"
    print "-"*80
    print "UseExtentsHint is %s" % bboxCache.GetUseExtentsHint()
    TestAtCurTime(stage, bboxCache)
    print "-"*80
    print 
    print "-"*80
    print "Running tests at UsdTimeCode(1.0)"
    print "-"*80
    bboxCache.SetTime(1.0)
    TestAtCurTime(stage, bboxCache)

    # Test the use of cached extents hint.
    bboxCache2 = UsdGeom.BBoxCache(Usd.TimeCode.Default(), 
                                   includedPurposes=[UsdGeom.Tokens.default_],
                                   useExtentsHint=True)
    print "-"*80
    print "Running tests at UsdTimeCode::Default()"
    print "-"*80
    print "useExtentsHint is %s" % bboxCache2.GetUseExtentsHint()
    TestAtCurTime(stage, bboxCache2)
    print "-"*80
    print 
    print "-"*80
    print "Running tests at UsdTimeCode(1.0)"
    print "-"*80
    bboxCache2.SetTime(1.0)
    TestAtCurTime(stage, bboxCache2)

def TestInstancedStage(stage, bboxCache):
    print "UseExtentsHint is %s" % bboxCache.GetUseExtentsHint()

    instancedPrim = stage.GetPrimAtPath("/instanced_parent")
    uninstancedPrim = stage.GetPrimAtPath("/uninstanced_parent")

    instancedRotated = stage.GetPrimAtPath("/instanced_Rotated")
    uninstancedRotated = stage.GetPrimAtPath("/uninstanced_Rotated")

    print
    print "Bound for instance prim /instanced_parent:", \
        bboxCache.ComputeWorldBound(instancedPrim)
    print
    print "Bound for prim /uninstanced_parent:", \
        bboxCache.ComputeWorldBound(uninstancedPrim)

    AssertBBoxesClose(bboxCache.ComputeWorldBound(instancedPrim),
                      bboxCache.ComputeWorldBound(uninstancedPrim), "Instanced")

    print
    print "Bound for instance prim /instanced_Rotated:", \
        bboxCache.ComputeWorldBound(instancedRotated)
    print
    print "Bound for prim /uninstanced_Rotated:", \
        bboxCache.ComputeWorldBound(uninstancedRotated)

    AssertBBoxesClose(bboxCache.ComputeWorldBound(instancedRotated),
                      bboxCache.ComputeWorldBound(uninstancedRotated), 
                      "Instanced")

def TestWithInstancing():
    stage = Usd.Stage.Open("cubeSbdv_instanced.usda")
    bboxCache = UsdGeom.BBoxCache(Usd.TimeCode.Default(), 
                                  includedPurposes=[UsdGeom.Tokens.default_])

    print "-"*80
    print "Testing bounding boxes on instanced prims"
    print "-"*80

    print "-"*80
    print "Running tests at UsdTimeCode::Default()"
    print "-"*80
    TestInstancedStage(stage, bboxCache)

    print 
    print "-"*80
    print "Running tests at UsdTimeCode(1.0)"
    print "-"*80
    bboxCache.SetTime(1.0)
    TestInstancedStage(stage, bboxCache)

    # Test the use of cached extents hint.
    bboxCache2 = UsdGeom.BBoxCache(Usd.TimeCode.Default(), 
                                   includedPurposes=[UsdGeom.Tokens.default_],
                                   useExtentsHint=True)
    
    print "-"*80
    print "Running tests at UsdTimeCode::Default()"
    print "-"*80
    TestInstancedStage(stage, bboxCache2)

    print 
    print "-"*80
    print "Running tests at UsdTimeCode(1.0)"
    print "-"*80
    bboxCache.SetTime(1.0)
    TestInstancedStage(stage, bboxCache2)

def TestBug113044():
    stage = Usd.Stage.Open("animVis.usda")
    bboxCache = UsdGeom.BBoxCache(Usd.TimeCode(0.0),
                                  includedPurposes=[UsdGeom.Tokens.default_])
    pseudoRoot = stage.GetPrimAtPath("/")
    assert bboxCache.ComputeWorldBound(pseudoRoot).GetRange().IsEmpty()

    # The cube is visible at frame 1. This should invalidate the bounds at '/'
    # and cause the bbox to be non-empty.
    bboxCache.SetTime(1.0)
    assert not bboxCache.ComputeWorldBound(pseudoRoot).GetRange().IsEmpty()

    bboxCache.SetTime(2.0)
    assert bboxCache.ComputeWorldBound(pseudoRoot).GetRange().IsEmpty()

    bboxCache.SetTime(3.0)
    assert not bboxCache.ComputeWorldBound(pseudoRoot).GetRange().IsEmpty()

def TestExtentCalculation():
    stage = Usd.Stage.Open(
        "pointsAndCurves.usda")
    bboxCache = UsdGeom.BBoxCache(Usd.TimeCode(0.0), 
        includedPurposes=[UsdGeom.Tokens.default_])

    print "-"*80
    print "Testing extent calculations on prims"
    print "-"*80
    print 

    validPrims = stage.GetPrimAtPath("/ValidPrims")
    warningPrims = stage.GetPrimAtPath("/WarningPrims")
    errorPrims = stage.GetPrimAtPath("/ErrorPrims")

    print "Visit Extent: " + str(validPrims)
    for prim in validPrims.GetChildren():
        print str(prim) + ": " + \
            str(bboxCache.ComputeWorldBound(prim).GetRange())
    print

    print "Visit Extent: " + str(warningPrims)
    for prim in warningPrims.GetChildren():
        print str(prim) + ": " + \
            str(bboxCache.ComputeWorldBound(prim).GetRange())
    print

    print "Visit Extent: " + str(errorPrims)
    for prim in errorPrims.GetChildren():
        bboxRange = bboxCache.ComputeWorldBound(prim).GetRange()
        print str(prim) + ": " + \
            str(bboxCache.ComputeWorldBound(prim).GetRange())
            # Note: The bbox has cached the result, and will not throw 
            #   another error here.

def TestUnloadedExtentsHints():
    stage = Usd.Stage.Open(
        "unloadedCubeModel.usda",
        load = Usd.Stage.LoadNone)
    bboxCacheNo = UsdGeom.BBoxCache(Usd.TimeCode(0.0), 
        includedPurposes=[UsdGeom.Tokens.default_], useExtentsHint=False)
    bboxCacheYes = UsdGeom.BBoxCache(Usd.TimeCode(0.0), 
        includedPurposes=[UsdGeom.Tokens.default_], useExtentsHint=True)

    print "-"*80
    print "Testing aggregate bounds with unloaded child prims"
    print "-"*80
    print 

    prim = stage.GetPseudoRoot()
    bboxNo  = bboxCacheNo.ComputeWorldBound(prim)
    bboxYes = bboxCacheYes.ComputeWorldBound(prim)
    
    assert bboxNo.GetRange().IsEmpty()
    assert not bboxYes.GetRange().IsEmpty()

def TestIgnoredPrims():
    stage = Usd.Stage.Open("cubeSbdv.usda")

    bboxCache = UsdGeom.BBoxCache(Usd.TimeCode.Default(), 
                                  includedPurposes=[UsdGeom.Tokens.default_])

    print "-"*80
    print "Testing computation for undefined, inactive and abstract prims"
    print "-"*80
    print

    undefinedPrim = stage.GetPrimAtPath("/undefinedCube1")
    assert bboxCache.ComputeWorldBound(undefinedPrim).GetRange().IsEmpty()

    inactivePrim = stage.GetPrimAtPath("/inactiveCube1")
    assert bboxCache.ComputeWorldBound(inactivePrim).GetRange().IsEmpty()

    abstractPrim = stage.GetPrimAtPath("/_class_UnitCube")
    assert bboxCache.ComputeWorldBound(abstractPrim).GetRange().IsEmpty()

def TestBug125048():
    stage = Usd.Stage.Open("testBug125048.usda")
    bboxCache = UsdGeom.BBoxCache(Usd.TimeCode.Default(), 
                                  includedPurposes=[UsdGeom.Tokens.default_],
                                  useExtentsHint=True)
    modelPrim = stage.GetPrimAtPath("/Model")
    geomPrim = stage.GetPrimAtPath("/Model/Geom/cube")
    bboxCache.ComputeUntransformedBound(modelPrim)
    # The following computation used to trip a verify.
    bboxCache.ComputeUntransformedBound(geomPrim)

def TestBug127801():
    """ Test that typeless defs are included in the traversal when computing 
        bounds. 
    """
    stage = Usd.Stage.CreateInMemory()
    world = stage.DefinePrim("/World")
    char = stage.DefinePrim("/World/anim/char")
    sphere = UsdGeom.Sphere.Define(stage, 
        char.GetPath().AppendChild('sphere'))

    bboxCache = UsdGeom.BBoxCache(Usd.TimeCode.Default(), 
                                  includedPurposes=[UsdGeom.Tokens.default_],
                                  useExtentsHint=True)
    bbox = bboxCache.ComputeUntransformedBound(world)
    assert not bbox.GetRange().IsEmpty()

if __name__ == "__main__":
    Main()
    TestWithInstancing()
    TestExtentCalculation()
    TestUnloadedExtentsHints()
    TestIgnoredPrims()

    # Turn off debug symbol for these regression tests.
    Tf.Debug.SetDebugSymbolsByName("USDGEOM_BBOX", 0)
    TestBug113044()
    TestBug125048()
    TestBug127801()

