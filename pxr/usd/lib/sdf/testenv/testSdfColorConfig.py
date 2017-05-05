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

from pxr import Sdf
import unittest

class TestSdfColorConfig(unittest.TestCase):
    # Test the color config API on SdfLayer
    def test_LayerColorConfig(self):
        filePath = 'testSdfColorConfig.testenv/colorConfig.sdf'
        layer = Sdf.Layer.FindOrOpen(filePath)
        self.assertTrue(layer is not None)

        expectedValue = Sdf.AssetPath('https://github.com/imageworks/OpenColorIO-Configs/blob/master/aces_1.0.1/config.ocio')
        self.assertEqual(layer.colorConfiguration, expectedValue)

        self.assertTrue(layer.HasColorConfiguration())
        layer.ClearColorConfiguration()
        self.assertFalse(layer.HasColorConfiguration())

        newValue = Sdf.AssetPath('https://github.com/imageworks/OpenColorIO-Configs/blob/master/aces_1.0.3/config.ocio')

        layer.colorConfiguration = newValue
        self.assertEqual(layer.colorConfiguration, newValue)

        self.assertTrue(layer.HasColorConfiguration())
        layer.ClearColorConfiguration()
        self.assertFalse(layer.HasColorConfiguration())

    def test_AttrColorSpace(self):
        filePath = 'testSdfColorConfig.testenv/colorSpace.sdf'
        layer = Sdf.Layer.FindOrOpen(filePath)
        self.assertTrue(layer is not None)

        baseColorAttr = layer.GetAttributeAtPath("/Pattern.baseColor")
        self.assertTrue(baseColorAttr)

        extraColorsAttr = layer.GetAttributeAtPath("/Pattern.extraColors")
        self.assertTrue(extraColorsAttr)

        unknownColorAttr = layer.GetAttributeAtPath("/Pattern.unknownColor")
        self.assertTrue(unknownColorAttr)

        self.assertTrue(baseColorAttr.HasColorSpace())
        self.assertTrue(extraColorsAttr.HasColorSpace())
        self.assertFalse(unknownColorAttr.HasColorSpace())

        self.assertEqual(baseColorAttr.colorSpace, 'lin_srgb')

        extraColorsAttr.ClearColorSpace()
        self.assertFalse(extraColorsAttr.HasColorSpace())

        unknownColorAttr.colorSpace = 'lin_rec709'
        self.assertTrue(unknownColorAttr.HasColorSpace())
        self.assertEqual(unknownColorAttr.colorSpace, 'lin_rec709')

if __name__ == '__main__':
    unittest.main()
