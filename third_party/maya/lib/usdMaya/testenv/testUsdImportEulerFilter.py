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
from maya.api import OpenMayaAnim as OMA
from maya import standalone

import os
import unittest


class testUsdImportEulerFilter(unittest.TestCase):

    EPSILON = 1e-6

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(cls):
        # Create a new file so each test case starts with a fresh state.
        cmds.file(new=1, f=1)

    def _GetNode(self, nodeName):
        if not cmds.objExists(nodeName):
            return None

        selectionList = OM.MSelectionList()
        selectionList.add(nodeName)
        return selectionList.getDependNode(0)

    def _GetMayaTransform(self, transformName):
        node = self._GetNode(transformName)
        if node:
            return OM.MFnTransform(node)
        return None

    def _TestRotations(self, xformName, expectedRotations):
        for timeCode in range(3):
            mayaTime = OM.MTime(timeCode)
            OMA.MAnimControl.setCurrentTime(mayaTime)

            mayaTransform = self._GetMayaTransform(xformName)
            transformationMatrix = mayaTransform.transformation()
            actualRotation = [Gf.RadiansToDegrees(r) for r in transformationMatrix.rotation()]
            expectedRotation = expectedRotations[timeCode]
            self.assertTrue(
                Gf.IsClose(
                    expectedRotation,
                    actualRotation,
                    self.EPSILON),
                "Expected '{0}' but got '{1}' for xform {2} at t={3}".format(
                    expectedRotation,
                    actualRotation,
                    xformName,
                    timeCode))

    def testImportXformNoEulerFlipsNoFilter(self):
        """
        Tests that importing a USD Xform that has custom XformOps with a
        a rotation not suffering from Euler flips results in the correct rotations
        when imported into Maya when not using the Euler filter.
        """
        usdFile = os.path.abspath('UsdImportEulerFilterTest.usda')
        cmds.usdImport(
            file=usdFile,
            shadingMode='none',
            readAnimData=True,
            useEulerFilter=False)

        expectedRotations = {
            0: [0.0, 10.0, 0.0],
            1: [0.0, 20.0, 0.0],
            2: [0.0, 30.0, 0.0]}

        xformName = "NoEulerFlips"
        self._TestRotations(xformName, expectedRotations)

    def testImportXformNoEulerFlipsWithFilter(self):
        """
        Tests that importing a USD Xform that has custom XformOps with a
        a rotation not suffering from Euler flips results in the correct rotations
        when imported into Maya when using the Euler filter.
        """
        usdFile = os.path.abspath('UsdImportEulerFilterTest.usda')
        cmds.usdImport(
            file=usdFile,
            shadingMode='none',
            readAnimData=True)

        expectedRotations = {
            0: [0.0, 10.0, 0.0],
            1: [0.0, 20.0, 0.0],
            2: [0.0, 30.0, 0.0]}

        xformName = "NoEulerFlips"
        self._TestRotations(xformName, expectedRotations)

    def testImportXformEulerFlipsNoFilter(self):
        """
        Tests that importing a USD Xform that has custom XformOps with a
        a rotation suffering from Euler flips results in the correct (flipped)
        rotations when imported into Maya when not using the Euler filter.
        """
        usdFile = os.path.abspath('UsdImportEulerFilterTest.usda')
        cmds.usdImport(
            file=usdFile,
            shadingMode='none',
            readAnimData=True,
            useEulerFilter=False)

        expectedRotations = {
            0: [0.0, 80.0, 0.0],
            1: [0.0, 90.0, 0.0],
            2: [180.0, 80.0, 180.0]}

        xformName = "EulerFlips"
        self._TestRotations(xformName, expectedRotations)

    def testImportXformEulerFlipsWithFilter(self):
        """
        Tests that importing a USD Xform that has custom XformOps with a
        a rotation suffering from Euler flips results in the correct rotations
        when imported into Maya when using the Euler filter.
        """
        usdFile = os.path.abspath('UsdImportEulerFilterTest.usda')
        cmds.usdImport(
            file=usdFile,
            shadingMode='none',
            readAnimData=True)

        expectedRotations = {
            0: [0.0, 80.0, 0.0],
            1: [0.0, 90.0, 0.0],
            2: [0.0, 100.0, 0.0]}

        xformName = "EulerFlips"
        self._TestRotations(xformName, expectedRotations)


    def testImportSimpleXformEulerFlipsNoFilter(self):
        """
        Tests that importing a USD Xform that has simple XformOps with a
        a rotation suffering from Euler flips results in the correct
        rotations when imported into Maya when not using the Euler filter.
        """
        usdFile = os.path.abspath('UsdImportEulerFilterTest.usda')
        cmds.usdImport(
            file=usdFile,
            shadingMode='none',
            readAnimData=True,
            useEulerFilter=False)

        expectedRotations = {
            0: [0.0, 80.0, 0.0],
            1: [0.0, 90.0, 0.0],
            2: [0.0, 100.0, 0.0]}

        xformName = "SimpleXformEulerFlips"
        self._TestRotations(xformName, expectedRotations)

    def testImportSimpleXformEulerFlipsWithFilter(self):
        """
        Tests that importing a USD Xform that has simple XformOps with a
        a rotation suffering from Euler flips results in the correct rotations
        when imported into Maya when using the Euler filter.
        """
        usdFile = os.path.abspath('UsdImportEulerFilterTest.usda')
        cmds.usdImport(
            file=usdFile,
            shadingMode='none',
            readAnimData=True)

        expectedRotations = {
            0: [0.0, 80.0, 0.0],
            1: [0.0, 90.0, 0.0],
            2: [0.0, 100.0, 0.0]}

        xformName = "SimpleXformEulerFlips"
        self._TestRotations(xformName, expectedRotations)

if __name__ == '__main__':
    unittest.main(verbosity=2)
