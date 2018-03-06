#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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


class testUsdExportLocator(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        mayaFile = os.path.abspath('SingleLocator.ma')
        cmds.file(mayaFile, open=True, force=True)

        cmds.loadPlugin('pxrUsd')

        cls._usdFilePathMerged = os.path.abspath(
            'SingleLocator_MERGED.usda')
        cmds.usdExport(file=cls._usdFilePathMerged,
            mergeTransformAndShape=True)

        cls._usdFilePathUnmerged = os.path.abspath(
            'SingleLocator_UNMERGED.usda')
        cmds.usdExport(file=cls._usdFilePathUnmerged,
            mergeTransformAndShape=False)

        cls._usdFilePathMergedRanged = os.path.abspath(
            'SingleLocator_MERGED_RANGED.usda')
        cmds.usdExport(file=cls._usdFilePathMergedRanged,
            mergeTransformAndShape=True, frameRange=(1, 1))

        cls._usdFilePathUnmergedRanged = os.path.abspath(
            'SingleLocator_UNMERGED_RANGED.usda')
        cmds.usdExport(file=cls._usdFilePathUnmergedRanged,
            mergeTransformAndShape=False, frameRange=(1, 1))

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _ValidateXformPrim(self, stage, xformPrimPath):
        xformPrim = stage.GetPrimAtPath(xformPrimPath)
        self.assertTrue(xformPrim)

        xformSchema = UsdGeom.Xform(xformPrim)
        self.assertTrue(xformSchema)

        return xformSchema

    def _ValidateLocatorPrim(self, stage, locatorPrimPath):
        xformSchema = self._ValidateXformPrim(stage, locatorPrimPath)

        xformOps = xformSchema.GetOrderedXformOps()
        self.assertEqual(len(xformOps), 1)

        translateOp = xformOps[0]

        self.assertEqual(translateOp.GetOpName(), 'xformOp:translate')
        self.assertEqual(translateOp.GetOpType(), UsdGeom.XformOp.TypeTranslate)

        expectedTranslation = Gf.Vec3d(1.0, 2.0, 3.0)
        self.assertTrue(Gf.IsClose(translateOp.Get(), expectedTranslation, 1e-6))

    def testExportSingleLocatorMerged(self):
        """
        Tests that exporting a single Maya locator works correctly when
        exporting with merge transforms and shapes enabled.
        """
        stage = Usd.Stage.Open(self._usdFilePathMerged)
        self.assertTrue(stage)

        self._ValidateLocatorPrim(stage, '/SingleLocator')

    def testExportSingleLocatorUnmerged(self):
        """
        Tests that exporting a single Maya locator works correctly when
        exporting with merge transforms and shapes disabled.
        """
        stage = Usd.Stage.Open(self._usdFilePathUnmerged)
        self.assertTrue(stage)

        self._ValidateLocatorPrim(stage, '/SingleLocator')

        self._ValidateXformPrim(stage, '/SingleLocator/SingleLocatorShape')

    def testExportSingleLocatorMergedRanged(self):
        """
        Tests that exporting a single Maya locator works correctly when
        exporting with merge transforms and shapes enabled and with a (trivial)
        frame range specified.
        """
        stage = Usd.Stage.Open(self._usdFilePathMergedRanged)
        self.assertTrue(stage)

        self._ValidateLocatorPrim(stage, '/SingleLocator')

    def testExportSingleLocatorUnmergedRanged(self):
        """
        Tests that exporting a single Maya locator works correctly when
        exporting with merge transforms and shapes disabled and with a (trivial)
        frame range specified.
        """
        stage = Usd.Stage.Open(self._usdFilePathUnmergedRanged)
        self.assertTrue(stage)

        self._ValidateLocatorPrim(stage, '/SingleLocator')

        self._ValidateXformPrim(stage, '/SingleLocator/SingleLocatorShape')


if __name__ == '__main__':
    unittest.main(verbosity=2)
