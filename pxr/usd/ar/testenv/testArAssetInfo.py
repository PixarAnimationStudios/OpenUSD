#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import unittest

from pxr import Ar

class TestArAssetInfo(unittest.TestCase):
    def test_Basic(self):
        assetInfo = Ar.AssetInfo()
        self.assertEqual(assetInfo.version, "")
        self.assertEqual(assetInfo.assetName, "")
        self.assertEqual(assetInfo.resolverInfo, None)

        assetInfo.version = "1"
        self.assertEqual(assetInfo.version, "1")        

        assetInfo.assetName = "Buzz"
        self.assertEqual(assetInfo.assetName, "Buzz")        

        assetInfo.resolverInfo = 1
        self.assertEqual(assetInfo.resolverInfo, 1)

        assetInfo.resolverInfo = "Buzz"
        self.assertEqual(assetInfo.resolverInfo, "Buzz")

    def test_Equality(self):
        assetInfo1 = Ar.AssetInfo()
        assetInfo2 = Ar.AssetInfo()
        self.assertTrue(assetInfo1 == assetInfo2)
        self.assertFalse(assetInfo1 != assetInfo2)

        assetInfo2.version = "1"
        self.assertFalse(assetInfo1 == assetInfo2)
        self.assertTrue(assetInfo1 != assetInfo2)

    def test_Hash(self):
        assetInfo1 = Ar.AssetInfo()
        assetInfo2 = Ar.AssetInfo()

        assetInfo3 = Ar.AssetInfo()
        assetInfo3.assetName = "Buzz"

        assetInfo4 = Ar.AssetInfo()
        assetInfo4.assetName = "Buzz"

        self.assertEqual(
            set([assetInfo1, assetInfo2, assetInfo3, assetInfo4]), 
            set([assetInfo1, assetInfo3]))

if __name__ == '__main__':
    unittest.main()
