#!/pxrpythonsubst

from pxr import Usd, UsdGeom, Vt, Sdf

from Mentor.Runtime import *

# Configure mentor so failed assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)

testFile = FindDataFile("testUsdGeomFaceSetAPI/Sphere.usda")
stage = Usd.Stage.Open(testFile)
AssertTrue(stage)

sphere = stage.GetPrimAtPath("/Sphere/pSphere1")
AssertTrue(sphere)

def TestBasicFaceSetRetrieval():
    # Don't want to use the UsdShade API here since this is usdGeom.
    #lookFaceSet = UsdShade.Look.CreateLookFaceSet(sphere);
    lookFaceSet = UsdGeom.FaceSetAPI(sphere, "look")

    AssertEqual(lookFaceSet.GetFaceSetName(), "look")
    AssertEqual(lookFaceSet.GetPrim(), sphere)

    (valid, reason) = lookFaceSet.Validate()
    AssertTrue(valid, reason)

    AssertTrue(lookFaceSet.GetIsPartition())
    
    fc = lookFaceSet.GetFaceCounts()

    AssertEqual(list(lookFaceSet.GetFaceCounts()), [4, 8, 4])
    AssertEqual(list(lookFaceSet.GetFaceCounts(Usd.TimeCode(1))), [4, 4, 8])
    AssertEqual(list(lookFaceSet.GetFaceCounts(Usd.TimeCode(2))), [8, 4, 4])
    AssertEqual(list(lookFaceSet.GetFaceCounts(Usd.TimeCode(3))), [4, 8, 4])
    
    AssertEqual(list(lookFaceSet.GetFaceIndices()), 
                [12, 13, 14, 15, 0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7])
    AssertEqual(list(lookFaceSet.GetFaceIndices(Usd.TimeCode(1))),
                [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])
    AssertEqual(list(lookFaceSet.GetFaceIndices(Usd.TimeCode(2))),
                [12, 13, 14, 15, 0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7])
    AssertEqual(list(lookFaceSet.GetFaceIndices(Usd.TimeCode(3))),
                [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

    AssertEqual(lookFaceSet.GetBindingTargets(), 
                [Sdf.Path("/Sphere/Looks/initialShadingGroup"),
                 Sdf.Path("/Sphere/Looks/lambert2SG"),
                 Sdf.Path("/Sphere/Looks/lambert3SG")])

def TestCreateFaceSet():
    newLook = UsdGeom.FaceSetAPI.Create(sphere, "newLook", isPartition=False)
    AssertEqual(newLook.GetFaceSetName(), "newLook")
    AssertEqual(newLook.GetPrim(), sphere)
    AssertFalse(newLook.GetIsPartition())
    
    # Empty faceSet should be invalid.
    (valid, reason) = newLook.Validate()
    AssertFalse(valid)

    AssertTrue(newLook.SetIsPartition(True))
    AssertTrue(newLook.GetIsPartition())
    AssertTrue(newLook.SetIsPartition(False))
    AssertFalse(newLook.GetIsPartition())

    AssertTrue(newLook.SetFaceCounts([1, 2, 3]))
    AssertTrue(newLook.SetFaceCounts([3, 2, 1], Usd.TimeCode(1.0)))
    AssertTrue(newLook.SetFaceCounts([2, 3, 1], Usd.TimeCode(2.0)))
    AssertTrue(newLook.SetFaceCounts([1, 3, 2], Usd.TimeCode(3.0)))

    AssertTrue(newLook.SetFaceIndices([0, 1, 2, 3, 4, 5]))
    AssertTrue(newLook.SetFaceIndices([0, 1, 2, 3, 4, 5],
                                      Usd.TimeCode(1.0)))
    AssertTrue(newLook.SetFaceIndices([6, 7, 8, 9, 10, 11], 
                                      Usd.TimeCode(2.0)))
    AssertTrue(newLook.SetFaceIndices([10, 11, 12, 13, 14, 15], 
                                      Usd.TimeCode(3.0)))

    AssertTrue(newLook.SetBindingTargets([Sdf.Path("/Sphere/Looks/initialShadingGroup"),
                 Sdf.Path("/Sphere/Looks/lambert2SG"),
                 Sdf.Path("/Sphere/Looks/lambert3SG")]))

def TestValidCases():
    validFaceSetNames = ("newLook", "validVaryingFaceCounts")

    for faceSetName in validFaceSetNames:
        faceSet = UsdGeom.FaceSetAPI(sphere, faceSetName)
        (valid, reason) = faceSet.Validate()
        AssertTrue(valid, "FaceSet '%s' was found to be invalid: %s" % 
            (faceSetName, reason))

def TestErrorCases():
    invalidFaceSetNames = ("badPartition", "missingIndices", 
        "invalidVaryingFaceCounts", "bindingMismatch")

    for faceSetName in invalidFaceSetNames:
        faceSet= UsdGeom.FaceSetAPI(sphere, faceSetName)
        (valid, reason) = faceSet.Validate()
        print "FaceSet named '%s' should be invalid because: %s" % \
            (faceSetName, reason)
        AssertFalse(valid)

def TestNumFaceSets():
    faceSets = UsdGeom.FaceSetAPI.GetFaceSets(sphere)
    AssertEqual(len(faceSets), 7)

def TestAppendFaceGroupBindingTarget():
    btTestFaceSet1 = UsdGeom.FaceSetAPI.Create(sphere, "bindingTargetTest1")
    btTestFaceSet1.AppendFaceGroup(faceIndices=[0], 
                                  bindingTarget=Sdf.Path())
    with ExpectedErrors(1):
        AssertException("""btTestFaceSet1.AppendFaceGroup(
            faceIndices=[1,2],
            bindingTarget=Sdf.Path('/Sphere/Looks/lambert2SG'))""",
            RuntimeError)

    btTestFaceSet2 = UsdGeom.FaceSetAPI.Create(sphere, "bindingTargetTest2")
    btTestFaceSet2.AppendFaceGroup(faceIndices=[0], 
        bindingTarget=Sdf.Path('/Sphere/Looks/lambert2SG'))
    with ExpectedErrors(1):
        AssertException("""btTestFaceSet2.AppendFaceGroup(
            faceIndices=[1,2],
            bindingTarget=Sdf.Path())""",
            RuntimeError)

def TestAppendFaceGroupAnimated():
    animatedFaceSet = UsdGeom.FaceSetAPI(sphere, "animated")

    # Test appending at default time.
    animatedFaceSet.AppendFaceGroup(faceIndices=[0], time=Usd.TimeCode.Default())
    animatedFaceSet.AppendFaceGroup(faceIndices=[1],time=Usd.TimeCode.Default())
    faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode.Default())
    AssertEqual(len(faceCounts), 2)

    # Test appending at the same time ordinate.
    animatedFaceSet.AppendFaceGroup(faceIndices=[2], time=Usd.TimeCode(1))
    animatedFaceSet.AppendFaceGroup(faceIndices=[3], time=Usd.TimeCode(1))
    faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode(1))
    AssertEqual(len(faceCounts), 2)

    # Test appending at a new time ordinate.
    animatedFaceSet.AppendFaceGroup(faceIndices=[4], time=Usd.TimeCode(2))

    faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode.Default())
    AssertEqual(len(faceCounts), 2)
    faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode(1))
    AssertEqual(len(faceCounts), 2)
    faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode(2))
    AssertEqual(len(faceCounts), 1)

def Main():
    TestBasicFaceSetRetrieval()
    TestCreateFaceSet()
    TestValidCases()
    TestErrorCases()
    TestNumFaceSets()

    # Test testUsdGeomFaceSetAPI::AppendFaceGroup()
    TestAppendFaceGroupBindingTarget()
    TestAppendFaceGroupAnimated()

if __name__ == "__main__":
    Main()
    ExitTest()
