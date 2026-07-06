#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import division
from pxr import Sdf, Tf
import itertools, unittest

# Test the basics of SdfTimeCode which is a special typed wrapper around 
# double values.
class TestSdfLayerOffset(unittest.TestCase):
    def test_LayerOffset(self):
        # Test the multiplying of layer offsets with time codes.
        layerOffset1 = Sdf.LayerOffset(offset=3.0)
        layerOffset2 = Sdf.LayerOffset(scale=2.0)
        layerOffset3 = Sdf.LayerOffset(offset=3.0, scale=2.0)

        timeCode1 = Sdf.TimeCode(0)
        timeCode2 = Sdf.TimeCode(3.0)
        timeCode3 = Sdf.TimeCode(-2.5)

        # Sanity check that multiplying a layer offset by a time code returns
        # a time code while multiplying by a float returns a float. 
        self.assertTrue(isinstance(layerOffset1 * timeCode2, Sdf.TimeCode))
        self.assertTrue(isinstance(layerOffset1 * 3.0, float))

        self.assertEqual(layerOffset1 * timeCode1, Sdf.TimeCode(3.0))
        self.assertEqual(layerOffset1 * timeCode2, Sdf.TimeCode(6.0))
        self.assertEqual(layerOffset1 * timeCode3, Sdf.TimeCode(0.5))

        self.assertEqual(layerOffset2 * timeCode1, Sdf.TimeCode(0.0))
        self.assertEqual(layerOffset2 * timeCode2, Sdf.TimeCode(6.0))
        self.assertEqual(layerOffset2 * timeCode3, Sdf.TimeCode(-5.0))

        self.assertEqual(layerOffset3 * timeCode1, Sdf.TimeCode(3.0))
        self.assertEqual(layerOffset3 * timeCode2, Sdf.TimeCode(9.0))
        self.assertEqual(layerOffset3 * timeCode3, Sdf.TimeCode(-2.0))

if __name__ == "__main__":
    unittest.main()
