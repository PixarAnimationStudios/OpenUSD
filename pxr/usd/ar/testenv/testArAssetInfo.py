#!/pxrpythonsubst
#
# Copyright 2022 Pixar
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
