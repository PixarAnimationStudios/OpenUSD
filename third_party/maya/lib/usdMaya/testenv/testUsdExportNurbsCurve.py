#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

from maya import cmds
from maya import standalone

from pxr import Gf
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt


class testUsdExportNurbsCurve(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.file(os.path.abspath('UsdExportNurbsCurveTest.ma'), open=True,
            force=True)

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExport(self):
        '''
        Export the Maya file and validate a handful of properties.
        '''
        usdFile = os.path.abspath('UsdExportNurbsCurveTest.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none')

        stage = Usd.Stage.Open(usdFile)
        
        nc = UsdGeom.NurbsCurves.Get(stage, '/curve1')
        self.assertEqual(nc.GetWidthsAttr().Get(), Vt.FloatArray([1.0]))
        self.assertEqual(nc.GetWidthsInterpolation(), UsdGeom.Tokens.constant)
        self.assertEqual(nc.GetRangesAttr().Get(), Vt.Vec2dArray([Gf.Vec2d(0, 5)]))
        self.assertEqual(nc.GetOrderAttr().Get(), Vt.IntArray([4]))
        self.assertEqual(nc.GetCurveVertexCountsAttr().Get(), Vt.IntArray([8]))


if __name__ == '__main__':
    unittest.main(verbosity=2)
