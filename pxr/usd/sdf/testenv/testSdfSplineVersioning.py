#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Gf, Ts, Sdf

import re
import unittest

class TestSdfSplineVersioning(unittest.TestCase):

    def setUp(self):
        self.usdName = './testSdfCrateSplines.usd'
        self.usdaName = self.usdName + 'a'
        self.usdcName = self.usdName + 'c'
        
        self.layer = Sdf.Layer.CreateNew('./testSdfCrateSplines.usd')
        self.prim = Sdf.CreatePrimInLayer(self.layer, '/Prim')
        self.attr = Sdf.CreatePrimAttributeInLayer(self.layer,
                                                   '/Prim.value',
                                                   Sdf.ValueTypeNames.Double)

    def _WriteFiles(self):
        self.layer.Save()
        self.layer.Export(self.usdaName)
        self.layer.Export(self.usdcName)

    def _CheckUsdaVersion(self, name):
        with open(name) as usdaFile:
            header = usdaFile.readline()

        match = re.match(r'#usda\s+([0-9.]+)\s*$', header)
        self.assertIsNotNone(
            match,
            f"Failed to find usda version in {name}. Header line: {header}")

        return match.group(1)

    def _CheckVersions(self):
        self._WriteFiles()

        usdVersion = Sdf.CrateInfo.Open(self.usdName).GetFileVersion()
        usdaVersion = self._CheckUsdaVersion(self.usdaName)
        usdcVersion = Sdf.CrateInfo.Open(self.usdcName).GetFileVersion()

        return (usdVersion, usdaVersion, usdcVersion)
        
    def test_Versioning(self):
        baseVersions = self._CheckVersions()
        self.assertEqual(baseVersions, ('0.8.0', '1.0', '0.8.0'))

        # Now add a spline.
        spline = Ts.Spline()
        spline.SetKnot(Ts.Knot(time=1.0, value=1.0))

        self.attr.SetSpline(spline)

        splineVersions = self._CheckVersions()
        self.assertEqual(splineVersions, ('0.12.0', '1.0', '0.12.0'))

        # Verify that clearing the spline resets the exported (but not saved)
        # versions.
        self.attr.ClearSpline()
        
        clearVersions = self._CheckVersions()
        self.assertEqual(clearVersions, ('0.12.0', '1.0', '0.8.0'))

        # Now add a tangent algorithm to the spline.
        spline.SetKnot(Ts.Knot(time=0.0, value=0.0, nextInterp=Ts.InterpCurve,
                               postTanAlgorithm=Ts.TangentAlgorithmAutoEase))
        self.attr.SetSpline(spline)

        algorithmVersions = self._CheckVersions()
        self.assertEqual(algorithmVersions, ('0.13.0', '1.1', '0.13.0'))

        # Add loopBoundaryTime to looping extrapolation
        extrap = Ts.Extrapolation(Ts.ExtrapLoopRepeat)
        extrap.loopBoundaryTime = 1.0
        spline.SetPreExtrapolation(extrap)
        self.attr.SetSpline(spline)

        loopBoundaryVersions = self._CheckVersions()
        self.assertEqual(loopBoundaryVersions, ('0.15.0', '1.3', '0.15.0'))

        # Reset the attr's spline
        self.attr.ClearSpline()

        clearVersions = self._CheckVersions()
        self.assertEqual(clearVersions, ('0.15.0', '1.0', '0.8.0'))

        # Create a spline with GfTimeCode as the value type
        spline = Ts.Spline("timecode")
        self.attr2 = Sdf.CreatePrimAttributeInLayer(self.layer,
                                                    '/Prim.timeValue',
                                                    Sdf.ValueTypeNames.TimeCode)
        self.attr2.SetSpline(spline)
        timeCodeVersions = self._CheckVersions()
        self.assertEqual(timeCodeVersions, ('0.15.0', '1.0', '0.15.0'))

        self.attr2.ClearSpline()
        clearVersions = self._CheckVersions()
        self.assertEqual(clearVersions, ('0.15.0', '1.0', '0.8.0'))

        # Create a GfDuration attribute
        self.attr = Sdf.CreatePrimAttributeInLayer(self.layer,
                                                   '/Prim.durationValue',
                                                   Sdf.ValueTypeNames.Duration)
        self.attr.default = Gf.Duration(1.0)
        durationVersions = self._CheckVersions()
        self.assertEqual(durationVersions, ('0.16.0', '1.4', '0.16.0'))


if __name__ == '__main__':
    unittest.main()
