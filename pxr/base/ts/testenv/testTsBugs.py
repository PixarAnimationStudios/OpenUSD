#!/pxrpythonsubst

#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Gf, Ts

import unittest

class TestBugs(unittest.TestCase):
    def testUSD_11046(self):
        """
        An uninitialized read was leading to the possibility that we could read
        past the end of a vector producing random values in the last sample
        point returned by TsSpline::Sample
        """
        spline = Ts.Spline('float')
        spline.SetKnot(Ts.Knot('float', time=  1, value=0.000,
                               nextInterp=Ts.InterpCurve))
        spline.SetKnot(Ts.Knot('float', time=200, value=0.001))

        samples = spline.Sample(Gf.Interval(0, 192), 1, 1, 0.5)

        print(spline)
        print(samples.polylines)

        polyline = samples.polylines[0]

        self.assertTrue(Gf.IsClose(polyline[2],
                                   Gf.Vec2d(192.0, 0.0009597990405628804),
                                   1.0e-10))

if __name__ == '__main__':
    # 'buffer' means that all stdout will be captured and swallowed, unless
    # there is an error, in which case the stdout of the erroring case will be
    # printed on stderr along with the test results.  Suppressing the output of
    # passing cases makes it easier to find the output of failing ones.
    unittest.main(testRunner = unittest.TextTestRunner(buffer = True))
