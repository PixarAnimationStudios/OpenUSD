#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Vt, Gf, Usd, UsdGeom, UsdLod

import math
import unittest

TRANSLATE_X = 3.0

class TestUsdLodRootAPI(unittest.TestCase):

    # Run before each test.
    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()
        self.world = UsdGeom.Xform.Define(self.stage, "/World")

        self.lodRoot = UsdGeom.Xform.Define(self.stage, "/World/lodRoot")
        self.lodRootPrim = self.lodRoot.GetPrim()
        self.lodRoot.AddTranslateXOp().Set(TRANSLATE_X)

        self.lodItems = [
            UsdGeom.Sphere.Define(self.stage, "/World/lodRoot/lodItem_0"),
            UsdGeom.Cylinder.Define(self.stage, "/World/lodRoot/lodItem_1"),
            UsdGeom.Cube.Define(self.stage, "/World/lodRoot/lodItem_2"),
        ]

        visValues = ["invisible", "inherited", "invisible"]
        for i, lodItem in enumerate(self.lodItems):
            lodItem.CreateVisibilityAttr(visValues[i])

    def test_RootAPI(self):
        # Test applying the rootAPI to a prim
        self.assertFalse(self.lodRootPrim.HasAPI(UsdLod.RootAPI),
                         f"{self.lodRoot} already has UsdLod.RootAPI applied.")
        rootAPI = UsdLod.RootAPI.Apply(self.lodRootPrim)
        self.assertTrue(rootAPI,
                        f"Failed to apply UsdLod.RootAPI to {self.lodRoot}.")
        self.assertTrue(self.lodRootPrim.HasAPI(UsdLod.RootAPI),
                        f"{self.lodRoot} does not have UsdLod.RootAPI applied.")

        # Test create attributes and relationships
        lodHeuristicsRel = rootAPI.CreateLodHeuristicsRel()
        self.assertTrue(lodHeuristicsRel,
                        f"Unable to create lodHeuristics relationship on"
                        f" {rootAPI}")

        # Test get attributes and relationships
        lodHeuristicsRel2 = rootAPI.GetLodHeuristicsRel()
        self.assertEqual(lodHeuristicsRel, lodHeuristicsRel2,
                         f"GetLodHeuristicsRel() returned a different attr than"
                         f" CreateLodHeuristicsRel() on {rootAPI}.")

        lodDefaultIndexAttr = rootAPI.CreateLodDefaultIndexAttr()
        self.assertTrue(lodDefaultIndexAttr,
                        f"Unable to create lod:default:index attribute on"
                        f" {rootAPI}")

        lodDefaultIndexAttr2 = rootAPI.GetLodDefaultIndexAttr()
        self.assertEqual(lodDefaultIndexAttr, lodDefaultIndexAttr2,
                         f"GetLodDefaultIndexAttr() returned a different attr"
                         f" than CreateLodDefaultIndexAttr() on {rootAPI}.")


if __name__ == "__main__":
    unittest.main()
