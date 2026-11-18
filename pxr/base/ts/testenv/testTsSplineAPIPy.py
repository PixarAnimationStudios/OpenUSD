#!/pxrpythonsubst

#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Tf, Ts, Gf

import unittest

class TsTest_SplineAPI(unittest.TestCase):

    def test_SplineToVtValueFromPython(self):
        # Test that we can convert a Spline to a VtValue from Python and back to
        # TsSpline python object.
        spline = Ts.Spline()
        spline.SetCurveType(Ts.CurveTypeHermite)
        # It returns the TsSpline which then gets wrapped in a python object as
        # it gets passed back to python and stored in spline2 below.
        spline2 = Ts._TestTsSplineToVtValueFromPython(spline)
        self.assertEqual(spline, spline2)

    def test_DefaultSplineCreationFromPython(self):
        spline = Ts.Spline()  # No argument
        self.assertFalse(spline.GetValueType())
        self.assertEqual(spline.GetValueTypeName(), '')
        self.assertFalse(spline.IsTimeValued())
        self.assertEqual(spline.GetCurveType(), Ts.CurveTypeBezier)
        self.assertEqual(spline.GetPreExtrapolation().mode, Ts.ExtrapHeld);
        self.assertEqual(spline.GetPostExtrapolation().mode, Ts.ExtrapHeld);
        self.assertTrue(spline.IsEmpty())
        self.assertFalse(spline.HasValueBlocks())
        self.assertFalse(spline.HasLoops())
        self.assertFalse(spline.GetKnots())
        self.assertFalse(spline.HasRegressiveTangents());

        self.assertIsNone(spline.Eval(0))
        self.assertIsNone(spline.EvalPreValue(0))
        self.assertIsNone(spline.EvalDerivative(0))
        self.assertIsNone(spline.EvalPreDerivative(0))
        self.assertIsNone(spline.EvalHeld(0))
        self.assertIsNone(spline.EvalPreValueHeld(0))

        knot = Ts.Knot('double', time=1.0, value=1.0)
        spline.SetKnot(knot)
        self.assertEqual(spline.GetValueType(), Tf.Type.FindByName('double'))
        self.assertEqual(spline.GetValueTypeName(), 'double')
        self.assertEqual(spline.Eval(0), 1.0)
        self.assertEqual(type(spline.Eval(0)), float)

        spline = Ts.Spline(None)  # None argument
        self.assertFalse(spline.GetValueType())
        self.assertEqual(spline.GetValueTypeName(), '')
        self.assertFalse(spline.IsTimeValued())
        self.assertEqual(spline.GetCurveType(), Ts.CurveTypeBezier)
        self.assertEqual(spline.GetPreExtrapolation().mode, Ts.ExtrapHeld);
        self.assertEqual(spline.GetPostExtrapolation().mode, Ts.ExtrapHeld);
        self.assertTrue(spline.IsEmpty())
        self.assertFalse(spline.HasValueBlocks())
        self.assertFalse(spline.HasLoops())
        self.assertFalse(spline.GetKnots())
        self.assertFalse(spline.HasRegressiveTangents());

        self.assertIsNone(spline.Eval(0))
        self.assertIsNone(spline.EvalPreValue(0))
        self.assertIsNone(spline.EvalDerivative(0))
        self.assertIsNone(spline.EvalPreDerivative(0))
        self.assertIsNone(spline.EvalHeld(0))
        self.assertIsNone(spline.EvalPreValueHeld(0))

        knot = Ts.Knot('half', time=1.0, value=1.0)
        spline.SetKnot(knot)
        self.assertEqual(spline.GetValueType(), Tf.Type.FindByName('GfHalf'))
        self.assertEqual(spline.GetValueTypeName(), 'half')
        self.assertEqual(spline.Eval(0), 1.0)
        self.assertEqual(type(spline.Eval(0)), float)

    def test_NamedTypeSplineCreationFromPython(self):
        # Type names with their TfType values and the Python type
        # that will be returned by the eval methods.
        namesAndTypes = [
            # name       TfType type                       Python type
            ('double',   Tf.Type.FindByName('double'),     float),
            ('float',    Tf.Type.FindByName('float'),      float),
            ('half',     Tf.Type.FindByName('GfHalf'),     float),
            ('timecode', Tf.Type.FindByName('GfTimeCode'), Gf.TimeCode),
            ('duration', Tf.Type.FindByName('GfDuration'), Gf.Duration)]

        for i, (typeName, tfType, pythonType) in enumerate(namesAndTypes):
            spline = Ts.Spline(typeName)
            self.assertEqual(spline.GetValueType(), tfType)
            self.assertEqual(spline.GetValueTypeName(), typeName)
            self.assertEqual(spline.IsTimeValued(), (i > 2))
            self.assertEqual(spline.GetCurveType(), Ts.CurveTypeBezier)
            self.assertEqual(spline.GetPreExtrapolation().mode, Ts.ExtrapHeld);
            self.assertEqual(spline.GetPostExtrapolation().mode, Ts.ExtrapHeld);
            self.assertTrue(spline.IsEmpty())
            self.assertFalse(spline.HasValueBlocks())
            self.assertFalse(spline.HasLoops())
            self.assertFalse(spline.GetKnots())
            self.assertFalse(spline.HasRegressiveTangents());

            self.assertIsNone(spline.Eval(0))
            self.assertIsNone(spline.EvalPreValue(0))
            self.assertIsNone(spline.EvalDerivative(0))
            self.assertIsNone(spline.EvalPreDerivative(0))
            self.assertIsNone(spline.EvalHeld(0))
            self.assertIsNone(spline.EvalPreValueHeld(0))

            knot = Ts.Knot(typeName, time=1.0, value=1.0)
            spline.SetKnot(knot)
            self.assertEqual(spline.Eval(0), 1.0)
            self.assertEqual(type(spline.Eval(0)), pythonType)

    def test_TfTypeSplineCreationFromPython(self):
        # Type names with their TfType values and the Python type
        # that will be returned by the eval methods.
        namesAndTypes = [
            # name       TfType type                       Python type
            ('double',   Tf.Type.FindByName('double'),     float),
            ('float',    Tf.Type.FindByName('float'),      float),
            ('half',     Tf.Type.FindByName('GfHalf'),     float),
            ('timecode', Tf.Type.FindByName('GfTimeCode'), Gf.TimeCode),
            ('duration', Tf.Type.FindByName('GfDuration'), Gf.Duration)]

        for i, (typeName, tfType, pythonType) in enumerate(namesAndTypes):
            spline = Ts.Spline(tfType)
            self.assertEqual(spline.GetValueType(), tfType)
            self.assertEqual(spline.GetValueTypeName(), typeName)
            self.assertEqual(spline.IsTimeValued(), (i > 2))
            self.assertEqual(spline.GetCurveType(), Ts.CurveTypeBezier)
            self.assertEqual(spline.GetPreExtrapolation().mode, Ts.ExtrapHeld);
            self.assertEqual(spline.GetPostExtrapolation().mode, Ts.ExtrapHeld);
            self.assertTrue(spline.IsEmpty())
            self.assertFalse(spline.HasValueBlocks())
            self.assertFalse(spline.HasLoops())
            self.assertFalse(spline.GetKnots())
            self.assertFalse(spline.HasRegressiveTangents());

            self.assertIsNone(spline.Eval(0))
            self.assertIsNone(spline.EvalPreValue(0))
            self.assertIsNone(spline.EvalDerivative(0))
            self.assertIsNone(spline.EvalPreDerivative(0))
            self.assertIsNone(spline.EvalHeld(0))
            self.assertIsNone(spline.EvalPreValueHeld(0))

            knot = Ts.Knot(typeName, time=1.0, value=1.0)
            spline.SetKnot(knot)
            self.assertEqual(spline.Eval(0), 1.0)
            self.assertEqual(type(spline.Eval(0)), pythonType)


if __name__ == "__main__":
    unittest.main()
