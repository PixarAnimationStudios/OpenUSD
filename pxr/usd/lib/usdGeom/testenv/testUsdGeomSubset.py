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

class testUsdGeomSubset(unittest.TestCase):
    def test_SubsetRetrievalAndValidity(self):
        geom = UsdGeom.Imageable(sphere)
        self.assertTrue(geom)
    
        materialBindSubsets = UsdGeom.Subset.GetGeomSubsets(geom, 
            elementType=UsdGeom.Tokens.face,
            familyName='materialBind')
        self.assertEqual(len(materialBindSubsets), 3)
        
        self.assertTrue(UsdGeom.Subset.GetFamilyIsPartition(geom, 
                                                            'materialBind'))

        (valid, reason) = UsdGeom.Subset.ValidatePartition(materialBindSubsets, 
                elementCount=16)

        (valid, reason) = UsdGeom.Subset.ValidateFamily(geom, 
                UsdGeom.Tokens.face, familyName='materialBind')
        self.assertTrue(valid)
        

        validFamilies = ['materialBind', 'validPartition']
        for familyName in validFamilies:
            (valid, reason) = UsdGeom.Subset.ValidateFamily(geom, 
                UsdGeom.Tokens.face, familyName=familyName)
            self.assertTrue(valid, "FaceSubset family '%s' was found to be "
                "invalid: %s" % (familyName, reason))
            self.assertEqual(len(reason), 0)

        invalidFamilies = ['invalidIndices', 'badPartition1', 'badPartition2', 
                           'badPartition3']
        for familyName in invalidFamilies:
            (valid, reason) = UsdGeom.Subset.ValidateFamily(geom, 
                UsdGeom.Tokens.face, familyName=familyName)
            print "Face-subset family '%s' should be invalid because: %s" % \
                (familyName, reason)
            self.assertFalse(valid)
            self.assertTrue(len(reason) > 0)

    def test_CreateGeomSubset(self):
        geom = UsdGeom.Imageable(sphere)
        self.assertTrue(geom)

        newSubset = UsdGeom.Subset.CreateGeomSubset(geom, 'testSubset', 
            UsdGeom.Tokens.face, indices=Vt.IntArray())

        # Indices are empty when unassigned.
        self.assertEqual(newSubset.GetIndicesAttr().Get(), Vt.IntArray())
        indices = [1, 2, 3, 4, 5]
        newSubset.GetIndicesAttr().Set(indices)
        self.assertEqual(list(newSubset.GetIndicesAttr().Get()), indices)

        # Check elementType
        self.assertEqual(newSubset.GetElementTypeAttr().Get(), 
                         UsdGeom.Tokens.face)

        # Check familyName
        self.assertEqual(newSubset.GetFamilyNameAttr().Get(), '')
        newSubset.GetFamilyNameAttr().Set('testFamily')
        self.assertEqual(newSubset.GetFamilyNameAttr().Get(), 'testFamily')

        # By default, a family of subsets is not tagged as a partition.
        self.assertEqual(UsdGeom.Subset.GetFamilyType(geom, 'testFamily'), 
                         UsdGeom.Subset.FamilyType.NonPartition)

        # Ensure that there's only one subset belonging to 'testFamily'.
        testSubsets = UsdGeom.Subset.GetGeomSubsets(geom, UsdGeom.Tokens.face,
                                                     'testFamily')
        self.assertEqual(len(testSubsets), 1)
        
        # Calling CreateGeomSubset with the same subsetName will just update 
        # info on the existing subset. 
        newIndices = Vt.IntArray([0, 1, 2])
        newerSubset = UsdGeom.Subset.CreateGeomSubset(geom, "testSubset", 
            UsdGeom.Tokens.face, newIndices, familyName='testFamily', 
            familyType=UsdGeom.Subset.FamilyType.Partition)
        self.assertEqual(newerSubset.GetPrim(), newSubset.GetPrim())

        testSubsets = UsdGeom.Subset.GetGeomSubsets(geom, UsdGeom.Tokens.face,
                                                    'testFamily')
        # Count is still one as no new subset was created by the above call. 
        self.assertEqual(len(testSubsets), 1)

        isTaggedAsPartition = UsdGeom.Subset.GetFamilyIsPartition(geom, 
                                                                  'testFamily')
        self.assertTrue(isTaggedAsPartition)
        self.assertEqual(UsdGeom.Subset.GetFamilyType(geom, 'testFamily'), 
                         UsdGeom.Subset.FamilyType.Partition)

        (valid, reason) = UsdGeom.Subset.ValidatePartition(testSubsets, 
                                                           elementCount=16)
        self.assertFalse(valid)
        
        # CreateUniqueGeomSubset will create a new subset always!
        unassignedIndices = UsdGeom.Subset.GetUnassignedIndices(testSubsets, 16)
        anotherSubset = UsdGeom.Subset.CreateUniqueGeomSubset(geom, "testSubset", 
            UsdGeom.Tokens.face, unassignedIndices, familyName='testFamily', 
            familyType=UsdGeom.Subset.FamilyType.Partition)

        self.assertNotEqual(anotherSubset.GetPrim().GetName(), 
                            newSubset.GetPrim().GetName())
        self.assertEqual(unassignedIndices, 
                         anotherSubset.GetIndicesAttr().Get())
        testSubsets = UsdGeom.Subset.GetGeomSubsets(geom, UsdGeom.Tokens.face,
                                                    familyName='testFamily')
        # Count is now two after the call to CreateUniqueGeomSubset.
        self.assertEqual(len(testSubsets), 2)

        (valid, reason) = UsdGeom.Subset.ValidatePartition(testSubsets, 
                                                           elementCount=16)
        self.assertTrue(valid)
        
        # Check total count.
        allGeomSubsets = UsdGeom.Subset.GetAllGeomSubsets(
                UsdGeom.Imageable(sphere))
        self.assertEqual(len(allGeomSubsets), 13)

if __name__ == "__main__":
    unittest.main()
