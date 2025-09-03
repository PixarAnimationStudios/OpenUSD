#!/pxrpythonsubst

#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Ts

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

if __name__ == "__main__":
    unittest.main()
