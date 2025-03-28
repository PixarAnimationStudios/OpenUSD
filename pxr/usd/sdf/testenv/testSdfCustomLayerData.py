#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf
import unittest

class TestSdfCustomLayer(unittest.TestCase):
    # Test the customLayerData API via Sdf's Layer API
    def test_BasicUsage(self):
        filePath = 'testSdfCustomLayerData.testenv/layerAccess.sdf'
        layer = Sdf.Layer.FindOrOpen(filePath)
        self.assertTrue(layer is not None)

        expectedValue = { 'layerAccessId' : 'id',
                          'layerAccessAssetPath' : Sdf.AssetPath('/layer/access.sdf'),
                          'layerAccessRandomNumber' : 5 }
        self.assertEqual(layer.customLayerData, expectedValue)

        self.assertTrue(layer.HasCustomLayerData())
        layer.ClearCustomLayerData()
        self.assertFalse(layer.HasCustomLayerData())

        newValue = { 'newLayerAccessId' : 'newId',
                     'newLayerAccessAssetPath' : Sdf.AssetPath('/new/layer/access.sdf'),
                     'newLayerAccessRandomNumber' : 1 }
        layer.customLayerData = newValue
        self.assertEqual(layer.customLayerData, newValue)

        self.assertTrue(layer.HasCustomLayerData())
        layer.ClearCustomLayerData()
        self.assertFalse(layer.HasCustomLayerData())

if __name__ == '__main__':
    unittest.main()
