#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Gf, Tf
import itertools, unittest

# Test the basics of GfDuration which is a special typed wrapper around
# double values.
class TestGfDuration(unittest.TestCase):
    def test_ReprAndConversion(self):
        # Verify that the default time code is 0.
        self.assertEqual(Gf.Duration(), Gf.Duration(0))

        duration1 = Gf.Duration(0)
        duration2 = Gf.Duration(3.0)
        duration3 = Gf.Duration(-2.5)

        self.assertEqual(repr(duration1), 'Gf.Duration(0)')
        self.assertEqual(eval(repr(duration1)), duration1)
        self.assertEqual(repr(duration2), 'Gf.Duration(3)')
        self.assertEqual(eval(repr(duration2)), duration2)
        self.assertEqual(repr(duration3), 'Gf.Duration(-2.5)')
        self.assertEqual(eval(repr(duration3)), duration3)

        self.assertEqual(str(duration1), '0')
        self.assertEqual(str(duration2), '3')
        self.assertEqual(str(duration3), '-2.5')

        # Converts to float
        self.assertEqual(float(duration1), 0)
        self.assertEqual(float(duration2), 3)
        self.assertEqual(float(duration3), -2.5)

        # GetValue
        self.assertEqual(duration1.GetValue(), 0)
        self.assertEqual(duration2.GetValue(), 3)
        self.assertEqual(duration3.GetValue(), -2.5)

        # bool conversion
        self.assertFalse(duration1)
        self.assertTrue(duration2)
        self.assertTrue(duration3)

    def test_Comparison(self):
        # Test the existence of comparison operators ==, !=, <, <=, >, >=.
        # We test all operator permutations of Gf.Duration and float:
        #   Gf.Duration <op> Gf.Duration
        #   Gf.Duration <op> float
        #   float <op> Gf.Duration
        duration1 = Gf.Duration(0)
        duration2 = Gf.Duration(3.0)
        duration3 = Gf.Duration(-2.5)

        self.assertTrue(duration2 == Gf.Duration(3))
        self.assertTrue(duration2 == 3)
        self.assertTrue(3 == duration2)

        self.assertTrue(duration3 != Gf.Duration(3))
        self.assertTrue(duration3 != 3)
        self.assertTrue(3 != duration3)

        self.assertFalse(duration1 < duration1)
        self.assertTrue(duration1 < duration2)
        self.assertFalse(duration1 < duration3)

        self.assertFalse(duration1 < 0)
        self.assertTrue(duration1 < 3)
        self.assertFalse(duration1 < -2.5)

        self.assertFalse(0 < duration1 )
        self.assertFalse(3 < duration1)
        self.assertTrue(-2.5 < duration1)

        self.assertTrue(duration1 <= duration1)
        self.assertTrue(duration1 <= duration2)
        self.assertFalse(duration1 <= duration3)

        self.assertTrue(duration1 <= 0)
        self.assertTrue(duration1 <= 3)
        self.assertFalse(duration1 <= -2.5)

        self.assertTrue(0 <= duration1)
        self.assertFalse(3 <= duration1)
        self.assertTrue(-2.5 <= duration1)

        self.assertFalse(duration1 > duration1)
        self.assertFalse(duration1 > duration2)
        self.assertTrue(duration1 > duration3)

        self.assertFalse(duration1 > 0)
        self.assertFalse(duration1 > 3)
        self.assertTrue(duration1 > -2.5)

        self.assertFalse(0 > duration1 )
        self.assertTrue(3 > duration1)
        self.assertFalse(-2.5 > duration1)

        self.assertTrue(duration1 >= duration1)
        self.assertFalse(duration1 >= duration2)
        self.assertTrue(duration1 >= duration3)

        self.assertTrue(duration1 >= 0)
        self.assertFalse(duration1 >= 3)
        self.assertTrue(duration1 >= -2.5)

        self.assertTrue(0 >= duration1)
        self.assertTrue(3 >= duration1)
        self.assertFalse(-2.5 >= duration1)

    def test_Arithmetic(self):
        # Test the existence of the basic aritmetic operators +, -, *, /
        # We test all operator permutations of Gf.Duration and float:
        #   Gf.Duration <op> Gf.Duration
        #   Gf.Duration <op> float
        #   float <op> Gf.Duration
        duration1 = Gf.Duration(0)
        duration2 = Gf.Duration(3.0)
        duration3 = Gf.Duration(-2.5)

        self.assertEqual(duration2 + duration3, Gf.Duration(0.5))
        self.assertEqual(duration3 + duration2, Gf.Duration(0.5))
        self.assertEqual(duration2 + 5.0, Gf.Duration(8.0))
        self.assertEqual(5.0 + duration2, Gf.Duration(8.0))

        self.assertEqual(duration2 - duration3, Gf.Duration(5.5))
        self.assertEqual(duration3 - duration2, Gf.Duration(-5.5))
        self.assertEqual(duration2 - 5.0, Gf.Duration(-2.0))
        self.assertEqual(5.0 - duration2, Gf.Duration(2.0))

        with self.assertRaises(TypeError,
                               msg = "duration * duration should not be allowed."):
            _ = duration2 * duration3
        self.assertEqual(duration2 * 5.0, Gf.Duration(15.0))
        self.assertEqual(5.0 * duration2, Gf.Duration(15.0))

        self.assertEqual(duration2 / Gf.Duration(2), float(1.5))
        self.assertEqual(Gf.Duration(6.0) / duration2, float(2.0))
        self.assertEqual(duration2 / 5.0, Gf.Duration(0.6))
        with self.assertRaises(TypeError,
                               msg = "float / duration should not be allowed."):
            _ = 6.0 / duration2


if __name__ == "__main__":
    unittest.main()
