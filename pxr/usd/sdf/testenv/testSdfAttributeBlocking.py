#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf
import unittest

class TestSdfAttributeBlocking(unittest.TestCase):
    def test_Basic(self):
        # generate a test layer with a default attr and time sample attr
        layer = Sdf.Layer.CreateAnonymous()
        prim = Sdf.PrimSpec(layer, "Sphere", Sdf.SpecifierDef)

        # configure a default attr
        defAttr = Sdf.AttributeSpec(prim, "visibility", Sdf.ValueTypeNames.Token)  
        defAttr.default = "visible"

        # configure a time sampled attr
        sampleAttr = Sdf.AttributeSpec(prim, "xformOp:transform", Sdf.ValueTypeNames.Double)
        layer.SetTimeSample(sampleAttr.path, 101, 101.0)
        layer.SetTimeSample(sampleAttr.path, 102, 102.0)
        layer.SetTimeSample(sampleAttr.path, 103, 103.0)
        layer.SetTimeSample(sampleAttr.path, 104, 104.0)

        # Test time sample based API
        for i in range(101, 105):
            self.assertEqual(layer.QueryTimeSample(sampleAttr.path, i), i)
            layer.SetTimeSample(sampleAttr.path, i, Sdf.ValueBlock())
            self.assertEqual(layer.QueryTimeSample(sampleAttr.path, i),
                             Sdf.ValueBlock())

        #Test default value API
        self.assertEqual(defAttr.default, 'visible')
        defAttr.default = Sdf.ValueBlock()
        self.assertEqual(defAttr.default, Sdf.ValueBlock())

if __name__ == "__main__":
    unittest.main()
