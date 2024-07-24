#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

# pylint: disable=range-builtin-not-iterating

from __future__ import print_function

from pxr import Usd, UsdGeom, Vt, Sdf
import unittest

class testUsdGeomSubset(unittest.TestCase):
    def _ValidateFamily(self, geom, elementType, familyName, 
                        expectedIsValid, expectedReasons=""):
        (valid, reason) = UsdGeom.Subset.ValidateFamily(
            geom, elementType, familyName=familyName)
        if expectedIsValid:
            self.assertTrue(valid, "Subset family '%s' was found to be "
                "invalid: %s" % (familyName, reason))
            self.assertEqual(len(reason), 0)
        else:
            print("Subset family '%s' should be invalid because: %s" % \
                (familyName, reason))
            self.assertFalse(valid)
            self.assertTrue(len(reason) > 0)
            for expectedReason in expectedReasons:
                self.assertTrue(expectedReason in reason)

    def _TestSubsetValidity(self, geom, varyingGeom, nullGeom, elementType):
        prefix = elementType + "_"

        validFamilies = ['validPartition', 
                         'validNonOverlapping', 'validUnrestricted',
                         'emptyIndicesSomeTimes']
        for familyName in validFamilies:
            self._ValidateFamily(geom, elementType, prefix+familyName, True)

        if elementType == UsdGeom.Tokens.edge:
            invalidFamilies = [ 
                ('invalidIndices',  ["does not exist on the parent prim",
                                     "Indices attribute has an odd number of elements",
                                     "Found one or more indices that are less than 0"]), 
                ('badPartition1',   ["does not match the element count",
                                     "does not exist on the parent prim"]), 
                ('badPartition2',   ["does not match the element count"]),
                ('badPartition3',   ["Found duplicate edge"]), 
                ('invalidNonOverlapping',   ["Found duplicate edge"]),
                ('invalidUnrestricted',     ["does not exist on the parent prim",
                                             "Found one or more indices that are less than 0"]), 
                ('onlyNegativeIndices',     ["Found one or more indices that are less than 0",
                                             "does not exist on the parent prim"]),
                ('emptyIndicesAtAllTimes',  ["No indices in family at any time"])]
        else:
            invalidFamilies = [
                ('invalidIndices',  ["Found one or more indices that are greater than the element count",
                                     "Found one or more indices that are less than 0"]), 
                ('badPartition1',   ["does not match the element count",
                                     "Found one or more indices that are greater than the element count"]), 
                ('badPartition2',   ["does not match the element count"]),
                ('badPartition3',   ["Found duplicate index"]), 
                ('invalidNonOverlapping',   ["Found duplicate index"]),
                ('invalidUnrestricted',     ["Found one or more indices that are greater than the element count",
                                             "Found one or more indices that are less than 0"]), 
                ('onlyNegativeIndices',     ["Found one or more indices that are less than 0"]),
                ('emptyIndicesAtAllTimes',  ["No indices in family at any time"])]

        for familyName, reasons in invalidFamilies:
            self._ValidateFamily(geom, elementType, prefix+familyName, False, reasons)

        validFamilies = ['validPartition']
        for familyName in validFamilies:
            self._ValidateFamily(varyingGeom, elementType, prefix+familyName, True)

        invalidFamilies = [('invalidNoDefaultTimeElements', ["has no elements"])]
        for familyName, reasons in invalidFamilies:
            self._ValidateFamily(varyingGeom, elementType, prefix+familyName, False, reasons)

        invalidFamilies = [('emptyIndicesAtAllTimes', ["No indices in family at any time"]), 
                           ('invalidPartition', ["Unable to determine element count at earliest time"])]
        for familyName, reasons in invalidFamilies:
            self._ValidateFamily(nullGeom, elementType, prefix+familyName, False, reasons)

    def _TestSubsetRetrieval(self, geom, elementType, familyName):
        prefix = elementType + "_"

        materialBindSubsets = UsdGeom.Subset.GetGeomSubsets(geom, 
            elementType=elementType,
            familyName=prefix+familyName)
        self.assertEqual(len(materialBindSubsets), 3)
        
        self.assertEqual(UsdGeom.Tokens.partition, 
            UsdGeom.Subset.GetFamilyType(geom, prefix+familyName))

        self._ValidateFamily(geom, elementType, prefix+familyName, True)

    def test_SubsetRetrievalAndValidity(self):
        testFile = "Sphere.usda"
        stage = Usd.Stage.Open(testFile)
        sphere = stage.GetPrimAtPath("/Sphere/pSphere1")
        geom = UsdGeom.Imageable(sphere)
        self.assertTrue(geom)

        varyingMesh = stage.GetPrimAtPath("/Sphere/VaryingMesh")
        varyingGeom = UsdGeom.Imageable(varyingMesh)
        self.assertTrue(varyingGeom)

        nullMesh = stage.GetPrimAtPath("/Sphere/NullMesh")
        nullGeom = UsdGeom.Imageable(nullMesh)
        self.assertTrue(nullGeom)

        self._TestSubsetRetrieval(geom, UsdGeom.Tokens.face, "materialBind")
        self._TestSubsetValidity(geom, varyingGeom, nullGeom, UsdGeom.Tokens.face)

        self._TestSubsetRetrieval(geom, UsdGeom.Tokens.point, "physicsAttachment")
        self._TestSubsetValidity(geom, varyingGeom, nullGeom, UsdGeom.Tokens.point)

        self._TestSubsetRetrieval(geom, UsdGeom.Tokens.edge, "physicsAttachment")
        self._TestSubsetValidity(geom, varyingGeom, nullGeom, UsdGeom.Tokens.edge)

        sphere = stage.GetPrimAtPath("/Sphere/TetMesh")
        geom = UsdGeom.Imageable(sphere)
        self.assertTrue(geom)

        varyingMesh = stage.GetPrimAtPath("/Sphere/VaryingTetMesh")
        varyingGeom = UsdGeom.Imageable(varyingMesh)
        self.assertTrue(varyingGeom)

        nullMesh = stage.GetPrimAtPath("/Sphere/NullTetMesh")
        nullGeom = UsdGeom.Imageable(nullMesh)
        self.assertTrue(nullGeom)

        self._TestSubsetRetrieval(geom, UsdGeom.Tokens.tetrahedron, "materialBind")
        self._TestSubsetValidity(geom, varyingGeom, nullGeom, UsdGeom.Tokens.tetrahedron)

        self._TestSubsetRetrieval(geom, UsdGeom.Tokens.face, "materialBind")
        self._TestSubsetValidity(geom, varyingGeom, nullGeom, UsdGeom.Tokens.face)


    def test_GetUnassignedIndicesForEdges(self):
        testFile = "Sphere.usda"
        stage = Usd.Stage.Open(testFile)
        sphere = stage.GetPrimAtPath("/Sphere/SimpleEdges")
        geom = UsdGeom.Imageable(sphere)
        self.assertTrue(geom)

        newSubset = UsdGeom.Subset.CreateGeomSubset(geom, 'testEdge', 
            UsdGeom.Tokens.edge, indices=Vt.IntArray())
        newSubset.GetFamilyNameAttr().Set('testEdgeFamily')

        # Indices are empty when unassigned.
        self.assertEqual(newSubset.GetIndicesAttr().Get(), Vt.IntArray())
        self.assertEqual(UsdGeom.Subset.GetUnassignedIndices(geom, 
                         UsdGeom.Tokens.edge, "testEdgeFamily"), 
                         Vt.IntArray([0, 1, 0, 3, 0, 4, 1, 2, 1, 5, 2, 3, 4, 5]))

        # Some indices are assigned
        indices = [0, 1, 5, 4]
        newSubset.GetIndicesAttr().Set(indices)
        self.assertEqual(list(newSubset.GetIndicesAttr().Get()), indices)
        self.assertEqual(UsdGeom.Subset.GetUnassignedIndices(geom, 
                         UsdGeom.Tokens.edge, "testEdgeFamily"), 
                         Vt.IntArray([0, 3, 0, 4, 1, 2, 1, 5, 2, 3]))

        # All indices are assigned
        indices = [0, 1, 0, 3, 0, 4, 1, 2, 1, 5, 2, 3, 4, 5]
        newSubset.GetIndicesAttr().Set(indices)
        self.assertEqual(list(newSubset.GetIndicesAttr().Get()), indices)
        self.assertEqual(UsdGeom.Subset.GetUnassignedIndices(geom, 
                         UsdGeom.Tokens.edge, "testEdgeFamily"), 
                         Vt.IntArray())

        # Confirm GetUnassignedIndices still works with invalid indices
        invalidIndices = [0, 1, 0, 3, 0, 4, 1, 2, 2, 3, 4, 5, 7, -1]
        newSubset = UsdGeom.Subset.CreateGeomSubset(geom, 'testEdge', 
            UsdGeom.Tokens.edge, indices=invalidIndices)
        newSubset.GetFamilyNameAttr().Set('testEdgeFamily')
        self.assertEqual(list(newSubset.GetIndicesAttr().Get()), invalidIndices)
        self.assertEqual(UsdGeom.Subset.GetUnassignedIndices(geom, 
                         UsdGeom.Tokens.edge, "testEdgeFamily"), 
                         Vt.IntArray([1, 5]))


    def test_CreateGeomSubset(self):
        testFile = "Sphere.usda"
        stage = Usd.Stage.Open(testFile)
        sphere = stage.GetPrimAtPath("/Sphere/pSphere1")
        geom = UsdGeom.Imageable(sphere)
        self.assertTrue(geom)

        newSubset = UsdGeom.Subset.CreateGeomSubset(geom, 'testSubset', 
            UsdGeom.Tokens.face, indices=Vt.IntArray())

        # Check elementType
        self.assertEqual(newSubset.GetElementTypeAttr().Get(), 
                         UsdGeom.Tokens.face)

        # Check familyName
        self.assertEqual(newSubset.GetFamilyNameAttr().Get(), '')
        newSubset.GetFamilyNameAttr().Set('testFamily')
        self.assertEqual(newSubset.GetFamilyNameAttr().Get(), 'testFamily')

        # Indices are empty when unassigned.
        self.assertEqual(newSubset.GetIndicesAttr().Get(), Vt.IntArray())
        self.assertEqual(UsdGeom.Subset.GetUnassignedIndices(geom, 
                         UsdGeom.Tokens.face, "testFamily"), 
                         Vt.IntArray(range(0,16)))
        indices = [1, 2, 3, 4, 5]
        newSubset.GetIndicesAttr().Set(indices)
        self.assertEqual(list(newSubset.GetIndicesAttr().Get()), indices)

        # By default, a family of subsets is not tagged as a partition.
        self.assertEqual(UsdGeom.Subset.GetFamilyType(geom, 'testFamily'), 
                         UsdGeom.Tokens.unrestricted)

        # Ensure that there's only one subset belonging to 'testFamily'.
        testSubsets = UsdGeom.Subset.GetGeomSubsets(geom, UsdGeom.Tokens.face,
                                                     'testFamily')
        self.assertEqual(len(testSubsets), 1)
        
        # Calling CreateGeomSubset with the same subsetName will just update 
        # info on the existing subset. 
        newIndices = Vt.IntArray([0, 1, 2])
        newerSubset = UsdGeom.Subset.CreateGeomSubset(geom, "testSubset", 
            UsdGeom.Tokens.face, newIndices, familyName='testFamily', 
            familyType=UsdGeom.Tokens.partition)
        self.assertEqual(newerSubset.GetPrim(), newSubset.GetPrim())

        testSubsets = UsdGeom.Subset.GetGeomSubsets(geom, UsdGeom.Tokens.face,
                                                    'testFamily')
        # Count is still one as no new subset was created by the above call. 
        self.assertEqual(len(testSubsets), 1)

        isTaggedAsPartition = (UsdGeom.Tokens.partition ==
            UsdGeom.Subset.GetFamilyType(geom, 'testFamily'))
        self.assertTrue(isTaggedAsPartition)
        self.assertEqual(UsdGeom.Subset.GetFamilyType(geom, 'testFamily'), 
                         UsdGeom.Tokens.partition)

        self._ValidateFamily(geom, 
                UsdGeom.Tokens.face, 'testFamily', False, ["does not match the element count"])

        unassignedIndices = UsdGeom.Subset.GetUnassignedIndices(geom,
                                    UsdGeom.Tokens.face, 'testFamily')
        self.assertEqual(unassignedIndices, Vt.IntArray(range(3, 16)))

        # Confirm GetUnassignedIndices still works with invalid indices 16-19
        anotherSubset = UsdGeom.Subset.CreateUniqueGeomSubset(geom, "testSubset", 
            UsdGeom.Tokens.face, Vt.IntArray(range(3, 20)), familyName='testFamily', 
            familyType=UsdGeom.Tokens.partition)

        # CreateUniqueGeomSubset will create a new subset always!
        self.assertEqual(anotherSubset.GetPrim().GetName(), "testSubset_1")
        self.assertNotEqual(anotherSubset.GetPrim().GetName(), 
                            newSubset.GetPrim().GetName())
        self.assertEqual(Vt.IntArray(range(3, 20)), 
                         anotherSubset.GetIndicesAttr().Get())
    
        # Verify that GetAssignedIndices still works if the element count
        # is less than the number of assigned indices (as per bug USD-5599)
        self.assertEqual(UsdGeom.Subset.GetUnassignedIndices(geom,
                         UsdGeom.Tokens.face, 'testFamily'), 
                         Vt.IntArray([]))

        testSubsets = UsdGeom.Subset.GetGeomSubsets(geom, UsdGeom.Tokens.face,
                                                    familyName='testFamily')
        # Count is now two after the call to CreateUniqueGeomSubset.
        self.assertEqual(len(testSubsets), 2)

        # Update anotherSubset to contain valid indices 
        anotherSubset = UsdGeom.Subset.CreateGeomSubset(geom, "testSubset_1", 
            UsdGeom.Tokens.face, unassignedIndices, familyName='testFamily', 
            familyType=UsdGeom.Tokens.partition)
        self.assertEqual(len(testSubsets), 2)

        self._ValidateFamily(geom, UsdGeom.Tokens.face, 'testFamily', True)
        
        # Check total count.
        allGeomSubsets = UsdGeom.Subset.GetAllGeomSubsets(
                UsdGeom.Imageable(sphere))
        self.assertEqual(len(allGeomSubsets), 68)

        # Check that invalid negative indices are ignored when getting 
        # unassigned indices.
        invalidIndices = Vt.IntArray([-3, -2, 0, 1, 2])
        invalidSubset = UsdGeom.Subset.CreateUniqueGeomSubset(geom, "testSubset", 
            UsdGeom.Tokens.face, invalidIndices, familyName='testInvalid', 
            familyType=UsdGeom.Tokens.partition)
        invalidSubset.GetIndicesAttr().Set(invalidIndices)
        self.assertTrue(invalidSubset)
        self.assertEqual(UsdGeom.Subset.GetUnassignedIndices(geom,
            UsdGeom.Tokens.face, 'testInvalid'), Vt.IntArray(range(3, 16)))

    # Test gathering of prim's geom subsets when prim's parent tree includes 
    # a not-defined parent prim e.g. PointInstancer with Prototype prim using
    # specifier "over"
    def test_PointInstancer(self):
        testFile = "PointInstancer.usda"
        stage = Usd.Stage.Open(testFile)
        sphere = stage.GetPrimAtPath(
            "/Sphere/PointInstancers/Prototypes/pSphere1")
        geom = UsdGeom.Imageable(sphere)
        self.assertTrue(geom)
    
        materialBindSubsets = UsdGeom.Subset.GetGeomSubsets(geom, 
            elementType=UsdGeom.Tokens.face,
            familyName='materialBind')
        self.assertEqual(len(materialBindSubsets), 3)
        
        self.assertEqual(UsdGeom.Tokens.partition, 
            UsdGeom.Subset.GetFamilyType(geom, 'materialBind'))

        self._ValidateFamily(geom, 
            UsdGeom.Tokens.face, 'materialBind', True)

if __name__ == "__main__":
    unittest.main()
