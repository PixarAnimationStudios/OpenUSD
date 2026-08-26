#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import division

import unittest
from pxr import Gf, Vt


def _TC(*vals):
    return Vt.TimeCodeArray([Gf.TimeCode(v) for v in vals])


def _D(*vals):
    return Vt.DurationArray([Gf.Duration(v) for v in vals])


class TestVtTimeCodeArray(unittest.TestCase):

    # ---- Homogeneous TimeCode array ops ---------------------------------

    def test_TimeCodeMinusTimeCode(self):
        # TimeCode - TimeCode -> Duration
        r = _TC(1, 3) - _TC(1, 1)
        self.assertIs(type(r), Vt.DurationArray)
        self.assertEqual(r, _D(0, 2))

    def test_TimeCodePlusTimeCode(self):
        # TimeCode + TimeCode -> TimeCode
        r = _TC(1, 3) + _TC(1, 3)
        self.assertIs(type(r), Vt.TimeCodeArray)
        self.assertEqual(r, _TC(2, 6))

    def test_TimeCodeDivideTimeCode(self):
        # TimeCode / TimeCode -> double
        r = _TC(2, 6) / _TC(1, 3)
        self.assertIs(type(r), Vt.DoubleArray)
        self.assertEqual(r, Vt.DoubleArray([2.0, 2.0]))

    def test_TimeCodeUnaryNeg(self):
        r = -_TC(1, 3)
        self.assertIs(type(r), Vt.TimeCodeArray)
        self.assertEqual(r, _TC(-1, -3))

    # ---- Homogeneous Duration array ops ---------------------------------

    def test_DurationPlusDuration(self):
        r = _D(2, 4) + _D(1, 1)
        self.assertIs(type(r), Vt.DurationArray)
        self.assertEqual(r, _D(3, 5))

    def test_DurationMinusDuration(self):
        r = _D(2, 4) - _D(1, 1)
        self.assertIs(type(r), Vt.DurationArray)
        self.assertEqual(r, _D(1, 3))

    def test_DurationDivideDuration(self):
        # Duration / Duration -> double
        r = _D(2, 6) / _D(1, 3)
        self.assertIs(type(r), Vt.DoubleArray)
        self.assertEqual(r, Vt.DoubleArray([2.0, 2.0]))

    def test_DurationUnaryNeg(self):
        r = -_D(2, 4)
        self.assertIs(type(r), Vt.DurationArray)
        self.assertEqual(r, _D(-2, -4))

    # ---- Scalar-by-double ops -------------------------------------------

    def test_TimeCodeScaleByDouble(self):
        self.assertEqual(_TC(1, 3) * 2.0, _TC(2, 6))
        self.assertEqual(2.0 * _TC(1, 3), _TC(2, 6))
        self.assertEqual(_TC(2, 6) / 2.0, _TC(1, 3))
        self.assertIs(type(_TC(1, 3) * 2.0), Vt.TimeCodeArray)
        self.assertIs(type(2.0 * _TC(1, 3)), Vt.TimeCodeArray)
        self.assertIs(type(_TC(2, 6) / 2.0), Vt.TimeCodeArray)

    def test_DurationScaleByDouble(self):
        self.assertEqual(_D(1, 3) * 2.0, _D(2, 6))
        self.assertEqual(2.0 * _D(1, 3), _D(2, 6))
        self.assertEqual(_D(2, 6) / 2.0, _D(1, 3))
        self.assertIs(type(_D(1, 3) * 2.0), Vt.DurationArray)
        self.assertIs(type(2.0 * _D(1, 3)), Vt.DurationArray)
        self.assertIs(type(_D(2, 6) / 2.0), Vt.DurationArray)

    # ---- Cross-type array-array ops -------------------------------------

    def test_TimeCodePlusDurationArray(self):
        r = _TC(1, 3) + _D(2, 4)
        self.assertIs(type(r), Vt.TimeCodeArray)
        self.assertEqual(r, _TC(3, 7))

    def test_TimeCodeMinusDurationArray(self):
        r = _TC(1, 3) - _D(2, 4)
        self.assertIs(type(r), Vt.TimeCodeArray)
        self.assertEqual(r, _TC(-1, -1))

    def test_DurationPlusTimeCodeArray(self):
        r = _D(2, 4) + _TC(1, 3)
        self.assertIs(type(r), Vt.TimeCodeArray)
        self.assertEqual(r, _TC(3, 7))

    def test_DurationMinusTimeCodeArray(self):
        # Duration - TimeCode -> TimeCode
        r = _D(2, 4) - _TC(1, 3)
        self.assertIs(type(r), Vt.TimeCodeArray)
        self.assertEqual(r, _TC(1, 1))

    # ---- Cross-type scalar ops ------------------------------------------

    def test_TimeCodeArrayDurationScalar(self):
        self.assertEqual(_TC(1, 3) + Gf.Duration(10), _TC(11, 13))
        self.assertEqual(Gf.Duration(10) + _TC(1, 3), _TC(11, 13))
        self.assertEqual(_TC(1, 3) - Gf.Duration(10), _TC(-9, -7))
        # Duration - TimeCode -> TimeCode (the reflected form)
        self.assertEqual(Gf.Duration(10) - _TC(1, 3), _TC(9, 7))
        self.assertIs(type(_TC(1, 3) + Gf.Duration(10)), Vt.TimeCodeArray)
        self.assertIs(type(Gf.Duration(10) - _TC(1, 3)), Vt.TimeCodeArray)

    def test_DurationArrayTimeCodeScalar(self):
        self.assertEqual(_D(2, 4) + Gf.TimeCode(10), _TC(12, 14))
        self.assertEqual(Gf.TimeCode(10) + _D(2, 4), _TC(12, 14))
        # Duration - TimeCode -> TimeCode
        self.assertEqual(_D(2, 4) - Gf.TimeCode(10), _TC(-8, -6))
        # TimeCode - Duration -> TimeCode (the reflected form)
        self.assertEqual(Gf.TimeCode(10) - _D(2, 4), _TC(8, 6))
        self.assertIs(type(_D(2, 4) + Gf.TimeCode(10)), Vt.TimeCodeArray)
        self.assertIs(type(Gf.TimeCode(10) - _D(2, 4)), Vt.TimeCodeArray)

    # ---- List / tuple operands ------------------------------------------

    def test_ListOperand(self):
        r = _TC(1, 3) + [Gf.TimeCode(1), Gf.TimeCode(1)]
        self.assertIs(type(r), Vt.TimeCodeArray)
        self.assertEqual(r, _TC(2, 4))

        r = _D(2, 4) + (Gf.Duration(1), Gf.Duration(1))
        self.assertIs(type(r), Vt.DurationArray)
        self.assertEqual(r, _D(3, 5))

    # ---- Empty-array promotion ------------------------------------------

    def test_EmptyPromotion(self):
        # An empty operand is promoted to zeros of the other's length.
        self.assertEqual(Vt.TimeCodeArray() - _TC(1, 3), _D(-1, -3))
        self.assertEqual(_TC(1, 3) + Vt.TimeCodeArray(), _TC(1, 3))

    # ---- Operations that must NOT exist ---------------------------------

    def test_InvalidOps(self):
        with self.assertRaises(TypeError):
            _TC(1, 3) * _TC(1, 3)
        with self.assertRaises(TypeError):
            _D(1, 3) * _D(1, 3)
        with self.assertRaises(TypeError):
            _TC(1, 3) % _TC(1, 3)
        with self.assertRaises(TypeError):
            _D(1, 3) % _D(1, 3)

    # ---- VtValue round-trips --------------------------------------------

    def test_VtValueRoundTrip(self):
        # GfDuration and its array survive a VtValue round-trip, exercising
        # the value-type registration added for GfDuration.
        self.assertEqual(Vt._test_Ident(Gf.Duration(2)), Gf.Duration(2))
        self.assertEqual(Vt._test_Ident(_D(2, 4)), _D(2, 4))
        self.assertEqual(Vt._test_Ident(Gf.TimeCode(2)), Gf.TimeCode(2))
        self.assertEqual(Vt._test_Ident(_TC(2, 4)), _TC(2, 4))


if __name__ == '__main__':
    unittest.main()
