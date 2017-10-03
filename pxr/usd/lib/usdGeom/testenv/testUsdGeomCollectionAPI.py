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

from pxr import Usd, UsdGeom, Vt, Sdf
import unittest

testFile = "Test.usda"
stage = Usd.Stage.Open(testFile)
root = stage.GetPrimAtPath("/CollectionTest")
sphere = stage.GetPrimAtPath("/CollectionTest/Geom/pSphere1")
cube = stage.GetPrimAtPath("/CollectionTest/Geom/pCube1")
cylinder = stage.GetPrimAtPath("/CollectionTest/Geom/pCylinder1")
cone = stage.GetPrimAtPath("/CollectionTest/Geom/pCone1")

class TestUsdGeomCollectionAPI(unittest.TestCase):
    def test_CreateCollection(self):
        collection = UsdGeom.CollectionAPI.Create(root, "includes")
        (valid, reason) = collection.Validate()
        self.assertTrue(valid)

        collection.AddTarget(sphere.GetPath())
        collection.AddTarget(cube.GetPath(), [1, 3, 5])
        collection.AddTarget(cylinder.GetPath())
        collection.AddTarget(cone.GetPath(), [2, 4, 6, 8])
        
        self.assertEqual(collection.GetTargets(), 
                    [Sdf.Path('/CollectionTest/Geom/pSphere1'), 
                     Sdf.Path('/CollectionTest/Geom/pCube1'), 
                     Sdf.Path('/CollectionTest/Geom/pCylinder1'), 
                     Sdf.Path('/CollectionTest/Geom/pCone1')])
        self.assertEqual(list(collection.GetTargetFaceCounts()), [0, 3, 0, 4])
        self.assertEqual(list(collection.GetTargetFaceIndices()), [1, 3, 5, 2, 4, 6, 8])

        rootModel = Usd.ModelAPI(root)
        # Try invoking Create() with a schema object. 
        # Also test re-creating an existing collection.
        sameCollection = UsdGeom.CollectionAPI.Create(rootModel, "includes")
        self.assertEqual(sameCollection.GetTargets(), 
                    [Sdf.Path('/CollectionTest/Geom/pSphere1'), 
                     Sdf.Path('/CollectionTest/Geom/pCube1'), 
                     Sdf.Path('/CollectionTest/Geom/pCylinder1'), 
                     Sdf.Path('/CollectionTest/Geom/pCone1')])
        self.assertEqual(list(sameCollection.GetTargetFaceCounts()), [0, 3, 0, 4])
        self.assertEqual(list(sameCollection.GetTargetFaceIndices()), [1, 3, 5, 2, 4, 6, 8])

        # Call Create() with a new set of targets, targetFaceCounts and targetFaceIndices.
        collection = UsdGeom.CollectionAPI.Create(rootModel, "includes",
            targets=[Sdf.Path('/CollectionTest/Geom/pCylinder1'), 
                     Sdf.Path('/CollectionTest/Geom/pCone1')],
            targetFaceCounts=[2, 5],
            targetFaceIndices=[0, 1, 0, 1, 2, 3, 4])
        self.assertEqual(collection.GetTargets(), 
                    [Sdf.Path('/CollectionTest/Geom/pCylinder1'), 
                     Sdf.Path('/CollectionTest/Geom/pCone1')])
        self.assertEqual(list(collection.GetTargetFaceCounts()), [2, 5])
        self.assertEqual(list(collection.GetTargetFaceIndices()), [0, 1, 0, 1, 2, 3, 4])

    def test_CollectionRetrieval(self):
        geometryCollection = UsdGeom.CollectionAPI(root, "geometry")
        (valid, reason) = geometryCollection.Validate()
        self.assertTrue(valid)

        self.assertEqual(geometryCollection.GetTargets(),
                    [Sdf.Path('/CollectionTest/Geom/pSphere1'),
                     Sdf.Path('/CollectionTest/Geom/pCube1')])
        self.assertEqual(list(geometryCollection.GetTargetFaceCounts()), [0, 3])
        self.assertEqual(list(geometryCollection.GetTargetFaceIndices()), 
                    [1, 3, 5])

        animatedCollection = UsdGeom.CollectionAPI(Usd.ModelAPI(root), "animated")
        self.assertEqual(animatedCollection.GetTargets(),
                    [Sdf.Path('/CollectionTest/Geom/pSphere1'),
                     Sdf.Path('/CollectionTest/Geom/pCone1')])
        self.assertEqual(list(animatedCollection.GetTargetFaceCounts()), [2, 6])
        self.assertEqual(list(animatedCollection.GetTargetFaceIndices()), 
                    [1, 2, 1, 3, 5, 7, 9, 11])

        time = Usd.TimeCode(1)
        self.assertEqual(list(animatedCollection.GetTargetFaceCounts(time)), [0, 0])
        self.assertEqual(list(animatedCollection.GetTargetFaceIndices(time)), [])

        time = Usd.TimeCode(3)
        self.assertEqual(list(animatedCollection.GetTargetFaceCounts(time)), [1, 2])
        self.assertEqual(list(animatedCollection.GetTargetFaceIndices(time)), [0, 0, 1])

        time = Usd.TimeCode(5)
        self.assertEqual(list(animatedCollection.GetTargetFaceCounts(time)), [2, 2])
        self.assertEqual(list(animatedCollection.GetTargetFaceIndices(time)), [1, 2, 0, 1])

        hasRelCollection = UsdGeom.CollectionAPI(root, "hasRel")
        self.assertEqual(hasRelCollection.GetTargets(),
                    [Sdf.Path('/CollectionTest/Geom/pSphere1.cube'),
                     Sdf.Path('/CollectionTest/Geom/pCube1.sphere')])

        hasInstancedTargetCollection = UsdGeom.CollectionAPI(root, 
                                                             "hasInstancedTarget")
        targetPath = hasInstancedTargetCollection.GetTargets()[0]
        self.assertEqual(targetPath, 
                    Sdf.Path('/CollectionTest/Geom/iCube/Geom/cube'))

    def test_ValidCases(self):
        validCollectionNames = ("includes", "geometry", "animated")
        for name in validCollectionNames:
            collection = UsdGeom.CollectionAPI(root, name)
            (valid, reason) = collection.Validate()
            self.assertTrue(valid, "Collection '%s' was found to be invalid: %s" % 
                (name, reason))

    def test_ErrorCases(self):
        invalidCollectionNames = ("nonExistentCollection", "faceCountsMismatch", 
            "indicesMismatch")
        for name in invalidCollectionNames:
            collection = UsdGeom.CollectionAPI(root, name)
            (valid, reason) = collection.Validate()
            print "Collection '%s' is invalid because: %s" % \
                (name, reason)
            self.assertFalse(valid)

    def test_NumCollections(self):
        collections = UsdGeom.CollectionAPI.GetCollections(root)
        self.assertEqual(len(collections), 8)

    def test_AddTarget(self):
        appendTestCollection = UsdGeom.CollectionAPI.Create(root, "AppendTest")

        # Appending an empty target should result in a coding error.
        with self.assertRaises(RuntimeError):    
            appendTestCollection.AddTarget(Sdf.Path())
        
        appendTestCollection.AddTarget(sphere.GetPath(), [], Usd.TimeCode(1))
        appendTestCollection.AddTarget(cube.GetPath(), [0, 1, 2, 3, 4 ], Usd.TimeCode(1))
        appendTestCollection.AddTarget(cone.GetPath(), [], Usd.TimeCode(1))
        appendTestCollection.AddTarget(cylinder.GetPath(), [5, 6, 7, 8], Usd.TimeCode(1))

        self.assertEqual(appendTestCollection.GetTargets(), [sphere.GetPath(), 
            cube.GetPath(), cone.GetPath(), cylinder.GetPath()])
        self.assertEqual(list(appendTestCollection.GetTargetFaceCounts(Usd.TimeCode(1))), 
                    [0, 5, 0, 4])
        self.assertEqual(list(appendTestCollection.GetTargetFaceIndices(Usd.TimeCode(1))), 
                    [0, 1, 2, 3, 4, 5, 6, 7, 8])

if __name__ == "__main__":
    unittest.main()
