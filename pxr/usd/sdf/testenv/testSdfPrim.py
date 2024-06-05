#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import print_function

import sys, unittest
from pxr import Sdf, Tf

class TestSdfPrim(unittest.TestCase):
    def test_CreatePrimInLayer(self):
        layer = Sdf.Layer.CreateAnonymous()

        self.assertTrue(Sdf.CreatePrimInLayer(layer, 'foo'))
        self.assertTrue(Sdf.CreatePrimInLayer(layer, 'foo/bar'))
        self.assertTrue(Sdf.CreatePrimInLayer(layer, 'foo/bar/baz'))
        self.assertTrue(layer.GetPrimAtPath('/foo'))
        self.assertTrue(layer.GetPrimAtPath('/foo/bar'))
        self.assertTrue(layer.GetPrimAtPath('/foo/bar/baz'))
        self.assertTrue(Sdf.CreatePrimInLayer(layer, '/boo'))
        self.assertTrue(Sdf.CreatePrimInLayer(layer, '/boo/bar'))
        self.assertTrue(Sdf.CreatePrimInLayer(layer, '/boo/bar/baz'))
        self.assertTrue(layer.GetPrimAtPath('/boo'))
        self.assertTrue(layer.GetPrimAtPath('/boo/bar'))
        self.assertTrue(layer.GetPrimAtPath('/boo/bar/baz'))
        self.assertEqual(Sdf.CreatePrimInLayer(layer, '.'),
                         layer.GetPrimAtPath('/'))
        with self.assertRaises(Tf.ErrorException):
            Sdf.CreatePrimInLayer(layer, '..')
        with self.assertRaises(Tf.ErrorException):
            Sdf.CreatePrimInLayer(layer, '../..')

        self.assertTrue(Sdf.JustCreatePrimInLayer(layer, 'goo'))
        self.assertTrue(Sdf.JustCreatePrimInLayer(layer, 'goo/bar'))
        self.assertTrue(Sdf.JustCreatePrimInLayer(layer, 'goo/bar/baz'))
        self.assertTrue(layer.GetPrimAtPath('/goo'))
        self.assertTrue(layer.GetPrimAtPath('/goo/bar'))
        self.assertTrue(layer.GetPrimAtPath('/goo/bar/baz'))
        self.assertTrue(Sdf.JustCreatePrimInLayer(layer, '/zoo'))
        self.assertTrue(Sdf.JustCreatePrimInLayer(layer, '/zoo/bar'))
        self.assertTrue(Sdf.JustCreatePrimInLayer(layer, '/zoo/bar/baz'))
        self.assertTrue(layer.GetPrimAtPath('/zoo'))
        self.assertTrue(layer.GetPrimAtPath('/zoo/bar'))
        self.assertTrue(layer.GetPrimAtPath('/zoo/bar/baz'))
        self.assertTrue(Sdf.JustCreatePrimInLayer(layer, '.'))
        with self.assertRaises(Tf.ErrorException):
            Sdf.JustCreatePrimInLayer(layer, '..')
        with self.assertRaises(Tf.ErrorException):
            Sdf.JustCreatePrimInLayer(layer, '../..')

    def test_NameChildrenInsert(self):
        import copy, random

        layer = Sdf.Layer.CreateAnonymous("test")
        rootPrim = Sdf.PrimSpec(layer, 'Root', Sdf.SpecifierDef, 'Scope')
        groundTruthList = []
        prevGroundTruthList = []

        for i in range(1000):
            primName = 'geom{0}'.format(i)
            insertIndex = random.randint(-100, 100)

            primSpec = Sdf.PrimSpec(layer, primName, Sdf.SpecifierDef, 'Scope')

            rootPrim.nameChildren.insert(insertIndex, primSpec)
            groundTruthList.insert(insertIndex, primName)
            nameChildrenList = [x.name for x in rootPrim.nameChildren.values()]

            if nameChildrenList == groundTruthList:
                prevGroundTruthList = copy.deepcopy(groundTruthList)
            else:
                print("FAILED with primName {0} and insertIndex {1}".format(
                    primName, insertIndex))
                print("groundTruthList is {0}".format(groundTruthList))
                print("            we got {0}".format(nameChildrenList))
                print("     previous list {0}".format(prevGroundTruthList))
                self.fail("Prim insertion test failed")

    def test_InertSpecRemoval(self):
        layer = Sdf.Layer.CreateAnonymous()

        # Create a prim hierarchy with only empty overs.
        Sdf.CreatePrimInLayer(layer, "/InertSubtree/Is/Inert")
        del layer.GetPrimAtPath("/").nameChildren["InertSubtree"]

        # Create a variant set with only empty variants.
        Sdf.CreatePrimInLayer(layer, "/InertVariants{v=a}")
        Sdf.CreatePrimInLayer(layer, "/InertVariants{v=b}")
        del layer.GetPrimAtPath("/InertVariants").variantSets["v"]

if __name__ == "__main__":
    unittest.main()

