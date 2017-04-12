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

testFile = "Sphere.usda"
stage = Usd.Stage.Open(testFile)
sphere = stage.GetPrimAtPath("/Sphere/pSphere1")

class TestUsdGeomFaceSetAPI(unittest.TestCase):
    def test_BasicFaceSetRetrieval(self):
        # Don't want to use the UsdShade API here since this is usdGeom.
        #lookFaceSet = UsdShade.Material.CreateMaterialFaceSet(sphere);
        lookFaceSet = UsdGeom.FaceSetAPI(sphere, "look")

        self.assertEqual(lookFaceSet.GetFaceSetName(), "look")
        self.assertEqual(lookFaceSet.GetPrim(), sphere)

        (valid, reason) = lookFaceSet.Validate()
        self.assertTrue(valid, reason)

        self.assertTrue(lookFaceSet.GetIsPartition())
        
        fc = lookFaceSet.GetFaceCounts()

        self.assertEqual(list(lookFaceSet.GetFaceCounts()), [4, 8, 4])
        self.assertEqual(list(lookFaceSet.GetFaceCounts(Usd.TimeCode(1))), [4, 4, 8])
        self.assertEqual(list(lookFaceSet.GetFaceCounts(Usd.TimeCode(2))), [8, 4, 4])
        self.assertEqual(list(lookFaceSet.GetFaceCounts(Usd.TimeCode(3))), [4, 8, 4])
        
        self.assertEqual(list(lookFaceSet.GetFaceIndices()), 
                    [12, 13, 14, 15, 0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7])
        self.assertEqual(list(lookFaceSet.GetFaceIndices(Usd.TimeCode(1))),
                    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])
        self.assertEqual(list(lookFaceSet.GetFaceIndices(Usd.TimeCode(2))),
                    [12, 13, 14, 15, 0, 1, 2, 3, 8, 9, 10, 11, 4, 5, 6, 7])
        self.assertEqual(list(lookFaceSet.GetFaceIndices(Usd.TimeCode(3))),
                    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])

        self.assertEqual(lookFaceSet.GetBindingTargets(), 
                    [Sdf.Path("/Sphere/Looks/initialShadingGroup"),
                     Sdf.Path("/Sphere/Looks/lambert2SG"),
                     Sdf.Path("/Sphere/Looks/lambert3SG")])

    def test_CreateFaceSet(self):
        newLook = UsdGeom.FaceSetAPI.Create(sphere, "newLook", isPartition=False)
        self.assertEqual(newLook.GetFaceSetName(), "newLook")
        self.assertEqual(newLook.GetPrim(), sphere)
        self.assertFalse(newLook.GetIsPartition())
        
        # Empty faceSet should be invalid.
        (valid, reason) = newLook.Validate()
        self.assertFalse(valid)

        self.assertTrue(newLook.SetIsPartition(True))
        self.assertTrue(newLook.GetIsPartition())
        self.assertTrue(newLook.SetIsPartition(False))
        self.assertFalse(newLook.GetIsPartition())

        self.assertTrue(newLook.SetFaceCounts([1, 2, 3]))
        self.assertTrue(newLook.SetFaceCounts([3, 2, 1], Usd.TimeCode(1.0)))
        self.assertTrue(newLook.SetFaceCounts([2, 3, 1], Usd.TimeCode(2.0)))
        self.assertTrue(newLook.SetFaceCounts([1, 3, 2], Usd.TimeCode(3.0)))

        self.assertTrue(newLook.SetFaceIndices([0, 1, 2, 3, 4, 5]))
        self.assertTrue(newLook.SetFaceIndices([0, 1, 2, 3, 4, 5],
                                          Usd.TimeCode(1.0)))
        self.assertTrue(newLook.SetFaceIndices([6, 7, 8, 9, 10, 11], 
                                          Usd.TimeCode(2.0)))
        self.assertTrue(newLook.SetFaceIndices([10, 11, 12, 13, 14, 15], 
                                          Usd.TimeCode(3.0)))

        self.assertTrue(newLook.SetBindingTargets([Sdf.Path("/Sphere/Looks/initialShadingGroup"),
                     Sdf.Path("/Sphere/Looks/lambert2SG"),
                     Sdf.Path("/Sphere/Looks/lambert3SG")]))

    def test_ValidCases(self):
        validFaceSetNames = ("newLook", "validVaryingFaceCounts")

        for faceSetName in validFaceSetNames:
            faceSet = UsdGeom.FaceSetAPI(sphere, faceSetName)
            (valid, reason) = faceSet.Validate()
            self.assertTrue(valid, "FaceSet '%s' was found to be invalid: %s" % 
                (faceSetName, reason))

    def test_ErrorCases(self):
        invalidFaceSetNames = ("badPartition", "missingIndices", 
            "invalidVaryingFaceCounts", "bindingMismatch")

        for faceSetName in invalidFaceSetNames:
            faceSet= UsdGeom.FaceSetAPI(sphere, faceSetName)
            (valid, reason) = faceSet.Validate()
            print "FaceSet named '%s' should be invalid because: %s" % \
                (faceSetName, reason)
            self.assertFalse(valid)

    def test_NumFaceSets(self):
        faceSets = UsdGeom.FaceSetAPI.GetFaceSets(sphere)
        self.assertEqual(len(faceSets), 9)

    def test_AppendFaceGroupBindingTarget(self):
        btTestFaceSet1 = UsdGeom.FaceSetAPI.Create(sphere, "bindingTargetTest1")
        btTestFaceSet1.AppendFaceGroup(faceIndices=[0], 
                                      bindingTarget=Sdf.Path())
        with self.assertRaises(RuntimeError):
            btTestFaceSet1.AppendFaceGroup(
                faceIndices=[1,2],
                bindingTarget=Sdf.Path('/Sphere/Looks/lambert2SG'))

        btTestFaceSet2 = UsdGeom.FaceSetAPI.Create(sphere, "bindingTargetTest2")
        btTestFaceSet2.AppendFaceGroup(faceIndices=[0], 
            bindingTarget=Sdf.Path('/Sphere/Looks/lambert2SG'))
        with self.assertRaises(RuntimeError):
            btTestFaceSet2.AppendFaceGroup(
                faceIndices=[1,2],
                bindingTarget=Sdf.Path())

    def test_AppendFaceGroupAnimated(self):
        animatedFaceSet = UsdGeom.FaceSetAPI(sphere, "animated")

        # Test appending at default time.
        animatedFaceSet.AppendFaceGroup(faceIndices=[0], time=Usd.TimeCode.Default())
        animatedFaceSet.AppendFaceGroup(faceIndices=[1],time=Usd.TimeCode.Default())
        faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode.Default())
        self.assertEqual(len(faceCounts), 2)

        # Test appending at the same time ordinate.
        animatedFaceSet.AppendFaceGroup(faceIndices=[2], time=Usd.TimeCode(1))
        animatedFaceSet.AppendFaceGroup(faceIndices=[3], time=Usd.TimeCode(1))
        faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode(1))
        self.assertEqual(len(faceCounts), 2)

        # Test appending at a new time ordinate.
        animatedFaceSet.AppendFaceGroup(faceIndices=[4], time=Usd.TimeCode(2))

        faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode.Default())
        self.assertEqual(len(faceCounts), 2)
        faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode(1))
        self.assertEqual(len(faceCounts), 2)
        faceCounts = animatedFaceSet.GetFaceCounts(Usd.TimeCode(2))
        self.assertEqual(len(faceCounts), 1)

if __name__ == "__main__":
    unittest.main()
