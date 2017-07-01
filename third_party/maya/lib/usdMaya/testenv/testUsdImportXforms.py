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

from pxr import Gf

from maya import cmds
from maya.api import OpenMaya as OM
from maya import standalone

import os
import unittest


class testUsdImportXforms(unittest.TestCase):

    EPSILON = 1e-6

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

        usdFile = os.path.abspath('UsdImportXformsTest.usda')
        cmds.usdImport(file=usdFile, shadingMode='none')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _GetMayaTransform(self, transformName):
        selectionList = OM.MSelectionList()
        selectionList.add(transformName)
        mObj = selectionList.getDependNode(0)

        return OM.MFnTransform(mObj)

    def testImportInverseXformOpsOnly(self):
        """
        Tests that importing a USD cube mesh that has XformOps on it all tagged
        as inverse ops results in the correct transform when imported into Maya.
        """
        mayaTransform = self._GetMayaTransform('InverseOpsOnlyCube')
        transformationMatrix = mayaTransform.transformation()

        expectedTranslation = [-1.0, -2.0, -3.0]
        actualTranslation = list(
            transformationMatrix.translation(OM.MSpace.kTransform))
        self.assertTrue(
            Gf.IsClose(expectedTranslation, actualTranslation, self.EPSILON))

        expectedRotation = [0.0, 0.0, Gf.DegreesToRadians(-45.0)]
        actualRotation = list(transformationMatrix.rotation())
        self.assertTrue(
            Gf.IsClose(expectedRotation, actualRotation, self.EPSILON))

        expectedScale = [2.0, 2.0, 2.0]
        actualScale = list(transformationMatrix.scale(OM.MSpace.kTransform))
        self.assertTrue(
            Gf.IsClose(expectedScale, actualScale, self.EPSILON))


if __name__ == '__main__':
    unittest.main(verbosity=2)
