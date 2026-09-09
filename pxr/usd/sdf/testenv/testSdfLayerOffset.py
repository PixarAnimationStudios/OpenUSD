#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from __future__ import division
from pxr import Sdf, Gf, Tf
import itertools, unittest

# Test the basics of SdfTimeCode which is a special typed wrapper around
# double values.
class TestSdfLayerOffset(unittest.TestCase):
    def test_LayerOffset(self):
        # Test the multiplying of layer offsets with time codes.
        layerOffset1 = Sdf.LayerOffset(offset=3.0)
        layerOffset2 = Sdf.LayerOffset(scale=2.0)
        layerOffset3 = Sdf.LayerOffset(offset=3.0, scale=2.0)

        float1 = 0.0
        float2 = 3.0
        float3 = -2.5

        timeCode1 = Gf.TimeCode(0)
        timeCode2 = Gf.TimeCode(3.0)
        timeCode3 = Gf.TimeCode(-2.5)

        duration1 = Gf.Duration(0)
        duration2 = Gf.Duration(3.0)
        duration3 = Gf.Duration(-2.5)

        # Sanity check that multiplying a layer offset by a time code returns
        # a time code while multiplying by a float returns a float.
        self.assertTrue(isinstance(layerOffset1 * float2,    float))
        self.assertTrue(isinstance(layerOffset1 * timeCode2, Gf.TimeCode))
        self.assertTrue(isinstance(layerOffset1 * duration2, Gf.Duration))

        # layerOffset1 - offset only, no scale
        self.assertEqual(layerOffset1 * float1,    3.0)
        self.assertEqual(layerOffset1 * timeCode1, Gf.TimeCode(3.0))
        self.assertEqual(layerOffset1 * duration1, Gf.Duration(0.0))

        self.assertEqual(layerOffset1 * float2,    6.0)
        self.assertEqual(layerOffset1 * timeCode2, Gf.TimeCode(6.0))
        self.assertEqual(layerOffset1 * duration2, Gf.Duration(3.0))

        self.assertEqual(layerOffset1 * float3,    0.5)
        self.assertEqual(layerOffset1 * timeCode3, Gf.TimeCode(0.5))
        self.assertEqual(layerOffset1 * duration3, Gf.Duration(-2.5))

        # layerOffset2 - scale only, no offset
        self.assertEqual(layerOffset2 * float1,    0.0)
        self.assertEqual(layerOffset2 * timeCode1, Gf.TimeCode(0.0))
        self.assertEqual(layerOffset2 * duration1, Gf.Duration(0.0))

        self.assertEqual(layerOffset2 * float2,    6.0)
        self.assertEqual(layerOffset2 * timeCode2, Gf.TimeCode(6.0))
        self.assertEqual(layerOffset2 * duration2, Gf.Duration(6.0))

        self.assertEqual(layerOffset2 * float3,    -5.0)
        self.assertEqual(layerOffset2 * timeCode3, Gf.TimeCode(-5.0))
        self.assertEqual(layerOffset2 * duration3, Gf.Duration(-5.0))

        # layerOffset 3 - scale and offset
        self.assertEqual(layerOffset3 * float1,    3.0)
        self.assertEqual(layerOffset3 * timeCode1, Gf.TimeCode(3.0))
        self.assertEqual(layerOffset3 * duration1, Gf.Duration(0.0))

        self.assertEqual(layerOffset3 * float2,    9.0)
        self.assertEqual(layerOffset3 * timeCode2, Gf.TimeCode(9.0))
        self.assertEqual(layerOffset3 * duration2, Gf.Duration(6.0))

        self.assertEqual(layerOffset3 * float3,    -2.0)
        self.assertEqual(layerOffset3 * timeCode3, Gf.TimeCode(-2.0))
        self.assertEqual(layerOffset3 * duration3, Gf.Duration(-5.0))


if __name__ == "__main__":
    unittest.main()
