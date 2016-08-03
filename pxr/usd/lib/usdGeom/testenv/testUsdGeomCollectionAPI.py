#!/pxrpythonsubst

from pxr import Usd, UsdGeom, Vt, Sdf

from Mentor.Runtime import *

# Configure mentor so failed assertions terminate execution.
SetAssertMode(MTR_EXIT_TEST)

testFile = FindDataFile("testUsdGeomCollectionAPI/Test.usda")
stage = Usd.Stage.Open(testFile)
AssertTrue(stage)

root = stage.GetPrimAtPath("/CollectionTest")
AssertTrue(root)

sphere = stage.GetPrimAtPath("/CollectionTest/Geom/pSphere1")
AssertTrue(sphere)

cube = stage.GetPrimAtPath("/CollectionTest/Geom/pCube1")
AssertTrue(cube)

cylinder = stage.GetPrimAtPath("/CollectionTest/Geom/pCylinder1")
AssertTrue(cylinder)

cone = stage.GetPrimAtPath("/CollectionTest/Geom/pCone1")
AssertTrue(cone)

def TestCreateCollection():
    collection = UsdGeom.CollectionAPI.Create(root, "includes")
    (valid, reason) = collection.Validate()
    AssertTrue(valid)

    collection.AppendTarget(sphere.GetPath())
    collection.AppendTarget(cube.GetPath(), [1, 3, 5])
    collection.AppendTarget(cylinder.GetPath())
    collection.AppendTarget(cone.GetPath(), [2, 4, 6, 8])
    
    AssertEqual(collection.GetTargets(), 
                [Sdf.Path('/CollectionTest/Geom/pSphere1'), 
                 Sdf.Path('/CollectionTest/Geom/pCube1'), 
                 Sdf.Path('/CollectionTest/Geom/pCylinder1'), 
                 Sdf.Path('/CollectionTest/Geom/pCone1')])
    AssertEqual(list(collection.GetTargetFaceCounts()), [0, 3, 0, 4])
    AssertEqual(list(collection.GetTargetFaceIndices()), [1, 3, 5, 2, 4, 6, 8])

    rootModel = Usd.ModelAPI(root)
    # Try invoking Create() with a schema object. 
    # Also test re-creating an existing collection.
    sameCollection = UsdGeom.CollectionAPI.Create(rootModel, "includes")
    AssertEqual(sameCollection.GetTargets(), 
                [Sdf.Path('/CollectionTest/Geom/pSphere1'), 
                 Sdf.Path('/CollectionTest/Geom/pCube1'), 
                 Sdf.Path('/CollectionTest/Geom/pCylinder1'), 
                 Sdf.Path('/CollectionTest/Geom/pCone1')])
    AssertEqual(list(sameCollection.GetTargetFaceCounts()), [0, 3, 0, 4])
    AssertEqual(list(sameCollection.GetTargetFaceIndices()), [1, 3, 5, 2, 4, 6, 8])

    # Call Create() with a new set of targets, targetFaceCounts and targetFaceIndices.
    collection = UsdGeom.CollectionAPI.Create(rootModel, "includes",
        targets=[Sdf.Path('/CollectionTest/Geom/pCylinder1'), 
                 Sdf.Path('/CollectionTest/Geom/pCone1')],
        targetFaceCounts=[2, 5],
        targetFaceIndices=[0, 1, 0, 1, 2, 3, 4])
    AssertEqual(collection.GetTargets(), 
                [Sdf.Path('/CollectionTest/Geom/pCylinder1'), 
                 Sdf.Path('/CollectionTest/Geom/pCone1')])
    AssertEqual(list(collection.GetTargetFaceCounts()), [2, 5])
    AssertEqual(list(collection.GetTargetFaceIndices()), [0, 1, 0, 1, 2, 3, 4])

def TestCollectionRetrieval():
    geometryCollection = UsdGeom.CollectionAPI(root, "geometry")
    (valid, reason) = geometryCollection.Validate()
    AssertTrue(valid)

    AssertEqual(geometryCollection.GetTargets(),
                [Sdf.Path('/CollectionTest/Geom/pSphere1'),
                 Sdf.Path('/CollectionTest/Geom/pCube1')])
    AssertEqual(list(geometryCollection.GetTargetFaceCounts()), [0, 3])
    AssertEqual(list(geometryCollection.GetTargetFaceIndices()), 
                [1, 3, 5])

    animatedCollection = UsdGeom.CollectionAPI(Usd.ModelAPI(root), "animated")
    AssertEqual(animatedCollection.GetTargets(),
                [Sdf.Path('/CollectionTest/Geom/pSphere1'),
                 Sdf.Path('/CollectionTest/Geom/pCone1')])
    AssertEqual(list(animatedCollection.GetTargetFaceCounts()), [2, 6])
    AssertEqual(list(animatedCollection.GetTargetFaceIndices()), 
                [1, 2, 1, 3, 5, 7, 9, 11])

    time = Usd.TimeCode(1)
    AssertEqual(list(animatedCollection.GetTargetFaceCounts(time)), [0, 0])
    AssertEqual(list(animatedCollection.GetTargetFaceIndices(time)), [])

    time = Usd.TimeCode(3)
    AssertEqual(list(animatedCollection.GetTargetFaceCounts(time)), [1, 2])
    AssertEqual(list(animatedCollection.GetTargetFaceIndices(time)), [0, 0, 1])

    time = Usd.TimeCode(5)
    AssertEqual(list(animatedCollection.GetTargetFaceCounts(time)), [2, 2])
    AssertEqual(list(animatedCollection.GetTargetFaceIndices(time)), [1, 2, 0, 1])

def TestValidCases():
    validCollectionNames = ("includes", "geometry", "animated")
    for name in validCollectionNames:
        collection = UsdGeom.CollectionAPI(root, name)
        (valid, reason) = collection.Validate()
        AssertTrue(valid, "Collection '%s' was found to be invalid: %s" % 
            (name, reason))

def TestErrorCases():
    invalidCollectionNames = ("nonExistentCollection", "faceCountsMismatch", 
        "indicesMismatch")
    for name in invalidCollectionNames:
        collection = UsdGeom.CollectionAPI(root, name)
        (valid, reason) = collection.Validate()
        print "Collection '%s' is invalid because: %s" % \
            (name, reason)
        AssertFalse(valid)

def TestNumCollections():
    collections = UsdGeom.CollectionAPI.GetCollections(root)
    AssertEqual(len(collections), 5)

def TestAppendTarget():
    appendTestCollection = UsdGeom.CollectionAPI.Create(root, "AppendTest")

    # Appending an empty target should result in a coding error.
    with ExpectedErrors(1):
        AssertException("appendTestCollection.AppendTarget(Sdf.Path())",
            RuntimeError)
    
    appendTestCollection.AppendTarget(sphere.GetPath(), [], Usd.TimeCode(1))
    appendTestCollection.AppendTarget(cube.GetPath(), [0, 1, 2, 3, 4 ], Usd.TimeCode(1))
    appendTestCollection.AppendTarget(cone.GetPath(), [], Usd.TimeCode(1))
    appendTestCollection.AppendTarget(cylinder.GetPath(), [5, 6, 7, 8], Usd.TimeCode(1))

    AssertEqual(appendTestCollection.GetTargets(), [sphere.GetPath(), 
        cube.GetPath(), cone.GetPath(), cylinder.GetPath()])
    AssertEqual(list(appendTestCollection.GetTargetFaceCounts(Usd.TimeCode(1))), 
                [0, 5, 0, 4])
    AssertEqual(list(appendTestCollection.GetTargetFaceIndices(Usd.TimeCode(1))), 
                [0, 1, 2, 3, 4, 5, 6, 7, 8])

def Main():
    TestCreateCollection()
    TestCollectionRetrieval()
    TestValidCases()
    TestErrorCases()
    TestNumCollections()

    # Test testUsdGeomCollectionAPI::AppendTarget()
    TestAppendTarget()

if __name__ == "__main__":
    Main()
    ExitTest()
