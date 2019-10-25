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
#

import os
import unittest

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom

from maya import cmds
from maya import standalone


class testPxrUsdTranslatorsStroke(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cls._testName = 'StrokeExportTest'

        mayaFile = os.path.abspath('%s.ma' % cls._testName)
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('%s.usda' % cls._testName)
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='none')

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

    def testExportStrokeNode(self):
        """
        Tests that a Maya stroke node created using Paint Effects exports
        correctly as a UsdGeomBasisCurves schema prim.
        """
        strokePrim = self._stage.GetPrimAtPath('/%s/Geom/StrokeCrystals' %
            self._testName)
        basisCurves = UsdGeom.BasisCurves(strokePrim)
        self.assertTrue(basisCurves)

        # As authored in Maya, there are expected to be 503 total curves,
        # composed of 3318 vertices.
        expectedNumCurves = 503
        expectedNumVertices = 3318

        curveVertexCounts = basisCurves.GetCurveVertexCountsAttr().Get()
        self.assertEqual(len(curveVertexCounts), expectedNumCurves)

        points = basisCurves.GetPointsAttr().Get()
        self.assertEqual(len(points), expectedNumVertices)

        widths = basisCurves.GetWidthsAttr().Get()
        self.assertEqual(len(widths), expectedNumVertices)

        displayColors = basisCurves.GetDisplayColorPrimvar().ComputeFlattened()
        self.assertEqual(len(displayColors), expectedNumVertices)

        displayOpacities = basisCurves.GetDisplayOpacityPrimvar().ComputeFlattened()
        self.assertEqual(len(displayOpacities), expectedNumVertices)


if __name__ == '__main__':
    unittest.main(verbosity=2)
