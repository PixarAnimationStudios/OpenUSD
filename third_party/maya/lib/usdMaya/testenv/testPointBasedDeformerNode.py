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

from maya import OpenMaya as OM
from maya import OpenMayaAnim as OMA
from maya import cmds
from maya import standalone


class testPointBasedDeformerNode(unittest.TestCase):

    START_TIMECODE = 1.0
    MID_TIMECODE = 13.0
    END_TIMECODE = 24.0

    EPSILON = 1e-3

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.loadPlugin('pxrUsd')

        cls._deformingCubeUsdFilePath = os.path.abspath('DeformingCube.usda')
        cls._deformingCubePrimPath = '/DeformingCube/Geom/Cube'

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def _ValidateControlPoint(self, nodeName, cpId, expectedPosition):
        cpX = cmds.getAttr('%s.controlPoints[%d].xValue' % (nodeName, cpId))
        cpY = cmds.getAttr('%s.controlPoints[%d].yValue' % (nodeName, cpId))
        cpZ = cmds.getAttr('%s.controlPoints[%d].zValue' % (nodeName, cpId))
        cpPosition = Gf.Vec3d(cpX, cpY, cpZ)

        self.assertTrue(Gf.IsClose(cpPosition, expectedPosition, self.EPSILON))

    def testCubeWithDeformer(self):
        """
        Tests that a native Maya mesh is deformed correctly by a point based
        deformer node.
        """
        OMA.MAnimControl.setAnimationStartEndTime(
            OM.MTime(self.START_TIMECODE), OM.MTime(self.END_TIMECODE))

        # Create the cube that will be affected by the deformer.
        testCube = cmds.polyCube(depth=1.0, height=1.0, width=1.0)[0]

        # Validate the top layer of control points of the unaffected cube.
        self._ValidateControlPoint(testCube, 0, Gf.Vec3d(-0.5, -0.5, 0.5))
        self._ValidateControlPoint(testCube, 1, Gf.Vec3d(0.5, -0.5, 0.5))
        self._ValidateControlPoint(testCube, 2, Gf.Vec3d(-0.5, 0.5, 0.5))
        self._ValidateControlPoint(testCube, 3, Gf.Vec3d(0.5, 0.5, 0.5))

        # Create the USD stage node.
        stageNode = cmds.createNode('pxrUsdStageNode')
        cmds.setAttr('%s.filePath' % stageNode, self._deformingCubeUsdFilePath,
            type='string')

        # Select the cube so that it has the deformer applied to it
        # automatically when the deformer is created.
        cmds.select(testCube, replace=True)

        # Create the deformer and setup its attributes and connections.
        deformerNode = cmds.deformer(type='pxrUsdPointBasedDeformerNode')[0]
        cmds.setAttr('%s.primPath' % deformerNode, self._deformingCubePrimPath,
            type='string')
        cmds.connectAttr('%s.outUsdStage' % stageNode,
            '%s.inUsdStage' % deformerNode)
        cmds.connectAttr('time1.outTime', '%s.time' % deformerNode)

        # The Maya cube should now be driven by the USD cube, which is twice
        # the size.
        self._ValidateControlPoint(testCube, 0, Gf.Vec3d(-1.0, -1.0, 1.0))
        self._ValidateControlPoint(testCube, 1, Gf.Vec3d(1.0, -1.0, 1.0))
        self._ValidateControlPoint(testCube, 2, Gf.Vec3d(-1.0, 1.0, 1.0))
        self._ValidateControlPoint(testCube, 3, Gf.Vec3d(1.0, 1.0, 1.0))

        # The animated deformation on the cube should twist the top of it in
        # the middle of the frame range.
        cmds.currentTime(self.MID_TIMECODE)

        self._ValidateControlPoint(testCube, 0, Gf.Vec3d(0.0, -1.0, 1.0))
        self._ValidateControlPoint(testCube, 1, Gf.Vec3d(1.0, 0.0, 1.0))
        self._ValidateControlPoint(testCube, 2, Gf.Vec3d(-1.0, 0.0, 1.0))
        self._ValidateControlPoint(testCube, 3, Gf.Vec3d(0.0, 1.0, 1.0))


if __name__ == '__main__':
    unittest.main(verbosity=2)
