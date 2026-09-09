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

    def test_SetChildrenReparentNestedValue(self):
        # Reparenting a child up one level and removing its (old) parent
        # should succeed.
        layer = Sdf.Layer.CreateAnonymous("test.usda")
        layer.ImportFromString('''#usda 1.0
            def Prim "X" {
                def Prim "Y" {
                }
            }
            ''')

        layer.rootPrims[:] = [layer.GetPrimAtPath("/X/Y")]

        # /X/Y has been reparented to /Y and /X has been removed.
        self.assertEqual([p.name for p in layer.rootPrims], ["Y"])
        self.assertTrue(layer.GetPrimAtPath("/Y"))
        self.assertFalse(layer.GetPrimAtPath("/X"))
        self.assertFalse(layer.GetPrimAtPath("/X/Y"))

        # A deeper nesting should behave the same way.
        layer2 = Sdf.Layer.CreateAnonymous("test2.usda")
        layer2.ImportFromString('''#usda 1.0
            def Prim "A" {
                def Prim "B" {
                    def Prim "C" {
                    }
                }
            }
            ''')
        layer2.rootPrims[:] = [layer2.GetPrimAtPath("/A/B/C")]
        self.assertEqual([p.name for p in layer2.rootPrims], ["C"])
        self.assertTrue(layer2.GetPrimAtPath("/C"))
        self.assertFalse(layer2.GetPrimAtPath("/A"))

        # Retaining the intermediate parent still works: both the parent and
        # its (reparented) child end up as siblings.
        layer3 = Sdf.Layer.CreateAnonymous("test3.usda")
        layer3.ImportFromString('''#usda 1.0
            def Prim "X" {
                def Prim "Y" {
                }
            }
            ''')
        layer3.rootPrims[:] = [layer3.GetPrimAtPath("/X"),
                               layer3.GetPrimAtPath("/X/Y")]
        self.assertEqual(sorted(p.name for p in layer3.rootPrims), ["X", "Y"])
        self.assertTrue(layer3.GetPrimAtPath("/X"))
        self.assertTrue(layer3.GetPrimAtPath("/Y"))
        self.assertFalse(layer3.GetPrimAtPath("/X/Y"))

        # Reparenting to not-the-root should work too.
        layer4 = Sdf.Layer.CreateAnonymous("test2.usda")
        layer4.ImportFromString('''#usda 1.0
            def Prim "A" {
                def Prim "B" {
                    def Prim "C" {
                    }
                }
            }
            ''')
        a = layer4.GetPrimAtPath("/A")
        a.nameChildren[:] = [layer4.GetPrimAtPath("/A/B/C")]
        self.assertEqual([p.name for p in a.nameChildren], ["C"])
        self.assertTrue(layer4.GetPrimAtPath("/A"))
        self.assertTrue(layer4.GetPrimAtPath("/A/C"))
        self.assertFalse(layer4.GetPrimAtPath("/A/B"))

    def test_InertSpecRemoval(self):
        layer = Sdf.Layer.CreateAnonymous()

        # Create a prim hierarchy with only empty overs.
        Sdf.CreatePrimInLayer(layer, "/InertSubtree/Is/Inert")
        del layer.GetPrimAtPath("/").nameChildren["InertSubtree"]

        # Create a variant set with only empty variants.
        Sdf.CreatePrimInLayer(layer, "/InertVariants{v=a}")
        Sdf.CreatePrimInLayer(layer, "/InertVariants{v=b}")
        del layer.GetPrimAtPath("/InertVariants").variantSets["v"]

    def test_Clips(self):
        layer = Sdf.Layer.CreateAnonymous()
        prim = Sdf.CreatePrimInLayer(layer, "/HasClips")

        # `clipSets`` behavior
        self.assertFalse(prim.hasClipSets)
        prim.clipSetsList.Append("clip1")
        self.assertTrue(prim.hasClipSets)
        self.assertTrue(prim.clipSetsList.appendedItems == ["clip1"])
        prim.clipSetsList.ClearEdits()
        self.assertFalse(prim.hasClipSets)
    
        # `clips`` behavior
        self.assertTrue(len(prim.clips) == 0)
        prim.clips = {"a": 1}
        self.assertTrue(prim.clips["a"] == 1)

if __name__ == "__main__":
    unittest.main()

