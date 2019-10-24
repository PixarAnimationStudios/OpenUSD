#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from pxr import Sdf, Tf
import itertools, unittest

# Test the basics of SdfTimeCode which is a special typed wrapper around 
# double values.
class TestSdfTimeCode(unittest.TestCase):
    def test_ReprAndConversion(self):
        # Verify that the default time code is 0.
        self.assertEqual(Sdf.TimeCode(), Sdf.TimeCode(0))

        timeCode1 = Sdf.TimeCode(0)
        timeCode2 = Sdf.TimeCode(3.0)
        timeCode3 = Sdf.TimeCode(-2.5)

        self.assertEqual(repr(timeCode1), 'Sdf.TimeCode(0)')
        self.assertEqual(eval(repr(timeCode1)), timeCode1)
        self.assertEqual(repr(timeCode2), 'Sdf.TimeCode(3)')
        self.assertEqual(eval(repr(timeCode2)), timeCode2)
        self.assertEqual(repr(timeCode3), 'Sdf.TimeCode(-2.5)')
        self.assertEqual(eval(repr(timeCode3)), timeCode3)

        self.assertEqual(str(timeCode1), '0')
        self.assertEqual(str(timeCode2), '3')
        self.assertEqual(str(timeCode3), '-2.5')

        # Converts to float
        self.assertEqual(float(timeCode1), 0)
        self.assertEqual(float(timeCode2), 3)
        self.assertEqual(float(timeCode3), -2.5)

        # GetValue
        self.assertEqual(timeCode1.GetValue(), 0)
        self.assertEqual(timeCode2.GetValue(), 3)
        self.assertEqual(timeCode3.GetValue(), -2.5)

        # bool conversion
        self.assertFalse(timeCode1)
        self.assertTrue(timeCode2)
        self.assertTrue(timeCode3)

    def test_Comparison(self):
        # Test the existence of comparison operators ==, !=, <, <=, >, >=.
        # We test all operator permutations of Sdf.TimeCode and float:
        #   Sdf.TimeCode <op> Sdf.TimeCode
        #   Sdf.TimeCode <op> float
        #   float <op> Sdf.TimeCode
        timeCode1 = Sdf.TimeCode(0)
        timeCode2 = Sdf.TimeCode(3.0)
        timeCode3 = Sdf.TimeCode(-2.5)

        self.assertTrue(timeCode2 == Sdf.TimeCode(3))
        self.assertTrue(timeCode2 == 3)
        self.assertTrue(3 == timeCode2)

        self.assertTrue(timeCode3 != Sdf.TimeCode(3))
        self.assertTrue(timeCode3 != 3)
        self.assertTrue(3 != timeCode3)

        self.assertFalse(timeCode1 < timeCode1)
        self.assertTrue(timeCode1 < timeCode2)
        self.assertFalse(timeCode1 < timeCode3)

        self.assertFalse(timeCode1 < 0)
        self.assertTrue(timeCode1 < 3)
        self.assertFalse(timeCode1 < -2.5)

        self.assertFalse(0 < timeCode1 )
        self.assertFalse(3 < timeCode1)
        self.assertTrue(-2.5 < timeCode1)

        self.assertTrue(timeCode1 <= timeCode1)
        self.assertTrue(timeCode1 <= timeCode2)
        self.assertFalse(timeCode1 <= timeCode3)

        self.assertTrue(timeCode1 <= 0)
        self.assertTrue(timeCode1 <= 3)
        self.assertFalse(timeCode1 <= -2.5)

        self.assertTrue(0 <= timeCode1)
        self.assertFalse(3 <= timeCode1)
        self.assertTrue(-2.5 <= timeCode1)

        self.assertFalse(timeCode1 > timeCode1)
        self.assertFalse(timeCode1 > timeCode2)
        self.assertTrue(timeCode1 > timeCode3)

        self.assertFalse(timeCode1 > 0)
        self.assertFalse(timeCode1 > 3)
        self.assertTrue(timeCode1 > -2.5)

        self.assertFalse(0 > timeCode1 )
        self.assertTrue(3 > timeCode1)
        self.assertFalse(-2.5 > timeCode1)

        self.assertTrue(timeCode1 >= timeCode1)
        self.assertFalse(timeCode1 >= timeCode2)
        self.assertTrue(timeCode1 >= timeCode3)

        self.assertTrue(timeCode1 >= 0)
        self.assertFalse(timeCode1 >= 3)
        self.assertTrue(timeCode1 >= -2.5)

        self.assertTrue(0 >= timeCode1)
        self.assertTrue(3 >= timeCode1)
        self.assertFalse(-2.5 >= timeCode1)

    def test_Arithmetic(self):
        # Test the existence of the basic aritmetic operators +, -, *, /
        # We test all operator permutations of Sdf.TimeCode and float:
        #   Sdf.TimeCode <op> Sdf.TimeCode
        #   Sdf.TimeCode <op> float
        #   float <op> Sdf.TimeCode
        timeCode1 = Sdf.TimeCode(0)
        timeCode2 = Sdf.TimeCode(3.0)
        timeCode3 = Sdf.TimeCode(-2.5)

        self.assertEqual(timeCode2 + timeCode3, Sdf.TimeCode(0.5))
        self.assertEqual(timeCode3 + timeCode2, Sdf.TimeCode(0.5))
        self.assertEqual(timeCode2 + 5.0, Sdf.TimeCode(8.0))
        self.assertEqual(5.0 + timeCode2, Sdf.TimeCode(8.0))

        self.assertEqual(timeCode2 - timeCode3, Sdf.TimeCode(5.5))
        self.assertEqual(timeCode3 - timeCode2, Sdf.TimeCode(-5.5))
        self.assertEqual(timeCode2 - 5.0, Sdf.TimeCode(-2.0))
        self.assertEqual(5.0 - timeCode2, Sdf.TimeCode(2.0))

        self.assertEqual(timeCode2 * timeCode3, Sdf.TimeCode(-7.5))
        self.assertEqual(timeCode3 * timeCode2, Sdf.TimeCode(-7.5))
        self.assertEqual(timeCode2 * 5.0, Sdf.TimeCode(15.0))
        self.assertEqual(5.0 * timeCode2, Sdf.TimeCode(15.0))

        self.assertEqual(timeCode2 / Sdf.TimeCode(2), Sdf.TimeCode(1.5))
        self.assertEqual(Sdf.TimeCode(6.0) / timeCode2, Sdf.TimeCode(2.0))
        self.assertEqual(timeCode2 / 5.0, Sdf.TimeCode(0.6))
        self.assertEqual(6.0 / timeCode2, Sdf.TimeCode(2.0))

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
        self.assertTrue(isinstance(layerOffset1 * Sdf.TimeCode(), Sdf.TimeCode))
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
