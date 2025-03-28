#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
