#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Usd

from maya import cmds
from maya import standalone


class testUsdExportOverImport(unittest.TestCase):

    USD_FILE = os.path.abspath('CubeModel.usda')

    BEFORE_PRIM_NAME = 'Cube'
    BEFORE_PRIM_PATH = '/CubeModel/Geom/%s' % BEFORE_PRIM_NAME
    AFTER_PRIM_NAME = 'NewCube'
    AFTER_PRIM_PATH = '/CubeModel/Geom/%s' % AFTER_PRIM_NAME

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _ValidateUsdBeforeExport(self):
        usdStage = Usd.Stage.Open(self.USD_FILE)
        self.assertTrue(usdStage)

        cubePrim = usdStage.GetPrimAtPath(self.BEFORE_PRIM_PATH)
        self.assertTrue(cubePrim)

        invalidPrim = usdStage.GetPrimAtPath(self.AFTER_PRIM_PATH)
        self.assertFalse(invalidPrim)

    def _ModifyMayaScene(self):
        cmds.rename(self.BEFORE_PRIM_NAME, self.AFTER_PRIM_NAME)

    def _ValidateUsdAfterExport(self):
        usdStage = Usd.Stage.Open(self.USD_FILE)
        self.assertTrue(usdStage)

        invalidPrim = usdStage.GetPrimAtPath(self.BEFORE_PRIM_PATH)
        self.assertFalse(invalidPrim)

        cubePrim = usdStage.GetPrimAtPath(self.AFTER_PRIM_PATH)
        self.assertTrue(cubePrim)

    def testImportModifyAndExportCubeModel(self):
        """
        Tests that re-exporting over a previously imported USD file works, and
        that changes are reflected in the new version of the file.
        """
        self._ValidateUsdBeforeExport()

        cmds.usdImport(file=self.USD_FILE, shadingMode='none')

        self._ModifyMayaScene()

        cmds.usdExport(mergeTransformAndShape=True, file=self.USD_FILE,
            shadingMode='none')

        self._ValidateUsdAfterExport()


if __name__ == '__main__':
    unittest.main(verbosity=2)
