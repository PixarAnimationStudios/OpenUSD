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

from maya import OpenMaya as OM
from maya import OpenMayaAnim as OMA


class testUsdImportNestedAssemblyAnimation(unittest.TestCase):

    SET_NAME = 'Cube_set'
    ANIM_START_TIME_CODE = 101.0
    ANIM_END_TIME_CODE = 149.0

    KEYFRAME_TIMES = [101.0, 125.0, 149.0]
    KEYFRAME_VALUES = [-10.0, 10.0, -10.0]

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd', quiet=True)

        usdFile = os.path.abspath('%s.usda' % cls.SET_NAME)
        primPath = '/%s' % cls.SET_NAME

        assemblyNode = cmds.createNode('pxrUsdReferenceAssembly',
            name=cls.SET_NAME)
        cmds.setAttr("%s.filePath" % assemblyNode, usdFile, type="string")
        cmds.setAttr("%s.primPath" % assemblyNode, primPath, type="string")

        # The set layer specifies a frame range of 101-149, so when we activate
        # the 'Full' representation and trigger an import, we expect the
        # current frame range in Maya to be expanded to include that. We set
        # it to something inside that range now so we can make sure that that
        # happens.
        OMA.MAnimControl.setAnimationStartEndTime(OM.MTime(121), OM.MTime(130))

        cmds.assembly(assemblyNode, edit=True, active='Full')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testFrameTimingAsExpected(self):
        self.assertAlmostEqual(OMA.MAnimControl.minTime().value(),
            self.ANIM_START_TIME_CODE)
        self.assertAlmostEqual(OMA.MAnimControl.maxTime().value(),
            self.ANIM_END_TIME_CODE)

    @staticmethod
    def _GetMayaDagNode(objName):
        selectionList = OM.MSelectionList()
        selectionList.add(objName)
        mObj = OM.MObject()
        selectionList.getDependNode(0, mObj)

        return OM.MFnDagNode(mObj)

    def _AssertAnimation(self, dagNodePath, expectedKeyframeTimes,
            expectedKeyframeValues):
        self.assertEqual(len(expectedKeyframeTimes), len(expectedKeyframeValues))

        dagNode = testUsdImportNestedAssemblyAnimation._GetMayaDagNode(dagNodePath)

        txPlug = dagNode.findPlug('translateX')
        animCurveFn = OMA.MFnAnimCurve(txPlug)

        expectedNumKeys = len(expectedKeyframeTimes)
        self.assertEqual(animCurveFn.numKeys(), expectedNumKeys)

        for keyNumber in xrange(expectedNumKeys):
            expectedTime = expectedKeyframeTimes[keyNumber]
            expectedValue = expectedKeyframeValues[keyNumber]

            keyTime = animCurveFn.time(keyNumber).value()
            self.assertAlmostEqual(expectedTime, keyTime)

            keyValue = animCurveFn.value(keyNumber)
            self.assertAlmostEqual(expectedValue, keyValue)

    def testImportCubeAnimatedModel(self):
        """
        Tests that importing a cube model that has animation authored directly
        in the model brings in the animation correctly when the model is
        referenced into a set.
        """
        nestedAssemblyNode = 'NS_Cube_set:AnimationDirectlyInModelCube'
        cmds.assembly(nestedAssemblyNode, edit=True, active='Full')

        cubeTransformDagNodePath = 'NS_Cube_set:NS_AnimationDirectlyInModelCube:Cube'

        self._AssertAnimation(cubeTransformDagNodePath, self.KEYFRAME_TIMES,
            self.KEYFRAME_VALUES)

    def testImportCubeAnimationLayerWithModelReference(self):
        """
        Tests that importing a cube model that is referenced by another layer
        that adds animation opinions brings in the animation correctly when the
        animation layer is referenced into a set.
        """
        nestedAssemblyNode = 'NS_Cube_set:AnimationInLayerOnReferencedCube'
        cmds.assembly(nestedAssemblyNode, edit=True, active='Full')

        cubeTransformDagNodePath = 'NS_Cube_set:NS_AnimationInLayerOnReferencedCube:Cube'

        self._AssertAnimation(cubeTransformDagNodePath, self.KEYFRAME_TIMES,
            self.KEYFRAME_VALUES)


if __name__ == '__main__':
    unittest.main(verbosity=2)
