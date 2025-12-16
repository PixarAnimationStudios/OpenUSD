#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import unittest

from pxr import Usd, UsdAbc, UsdGeom


class TestUsdAbcArrayExtent(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        usdFile = 'original.usda'
        abcFile = 'converted.abc'

        UsdAbc._WriteAlembic(usdFile, abcFile)
        cls.stage = Usd.Stage.Open(abcFile)

    def test_Roundtrip(self):

        prim = self.stage.GetPrimAtPath("/Prim")
        self.assertTrue(prim)

        primvarsToTest = [
            ("arrayExtent1", False, 1),
            ("podExtent2", False, 1),
            ("arrayExtent2", True, 2),
            ("arrayExtent3", True, 3),
        ]

        pvApi = UsdGeom.PrimvarsAPI(prim)
        for name, authored, size in primvarsToTest:
            pv = pvApi.GetPrimvar(name)
            self.assertEqual(pv.HasAuthoredElementSize(), authored)
            self.assertEqual(pv.GetElementSize(), size)


if __name__ == "__main__":
    unittest.main()
