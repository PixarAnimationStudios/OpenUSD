#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import UsdUtils, Sdf
import unittest

class TestUsdUtilsPipeline(unittest.TestCase):
    def test_Basic(self):
        lyr1 = Sdf.Layer.FindOrOpen('BottleMedicalDefaultPrim.usd')
        self.assertTrue(lyr1)
        self.assertEqual(UsdUtils.GetModelNameFromRootLayer(lyr1), 'BottleMedical')

        lyr2 = Sdf.Layer.FindOrOpen('BottleMedicalSameName.usd')
        self.assertTrue(lyr2)
        self.assertEqual(UsdUtils.GetModelNameFromRootLayer(lyr2), 'BottleMedicalSameName')

        lyr3 = Sdf.Layer.FindOrOpen('BottleMedicalRootPrim.usd')
        self.assertTrue(lyr3)
        self.assertEqual(UsdUtils.GetModelNameFromRootLayer(lyr3), 'BottleMedical')

if __name__=="__main__":
    unittest.main()
