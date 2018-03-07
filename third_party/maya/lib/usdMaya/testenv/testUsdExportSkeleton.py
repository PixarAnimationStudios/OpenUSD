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

from maya import cmds
from maya import standalone
from maya.api import OpenMaya as OM

from pxr import Gf, Usd, UsdSkel, Vt


class testUsdExportSkeleton(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.file(os.path.abspath('UsdExportSkeleton.ma'), open=True,
            force=True)

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _AssertMatricesClose(self, gfm1, gfm2):
        for i in xrange(0, 4):
            for j in xrange(0, 4):
                self.assertAlmostEqual(gfm1[i][j], gfm2[i][j], places=3)

    def testSkeletonTopology(self):
        """Tests that the joint topology is correct."""
        usdFile = os.path.abspath('UsdExportSkeleton.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none')
        stage = Usd.Stage.Open(usdFile)

        skeleton = UsdSkel.Skeleton.Get(stage, '/skeleton_Hip')
        self.assertTrue(skeleton)

        joints = skeleton.GetJointsAttr().Get()
        self.assertEqual(joints, Vt.TokenArray([
            "Hip",
            "Hip/Spine",
            "Hip/Spine/Neck",
            "Hip/Spine/Neck/Head",
            "Hip/Spine/Neck/LArm",
            "Hip/Spine/Neck/LArm/LHand",
            # note: skips ExtraJoints because it's not a joint
            "Hip/Spine/Neck/LArm/LHand/ExtraJoints/ExtraJoint1",
            "Hip/Spine/Neck/LArm/LHand/ExtraJoints/ExtraJoint1/ExtraJoint2",
            "Hip/Spine/Neck/RArm",
            "Hip/Spine/Neck/RArm/RHand",
            "Hip/RLeg",
            "Hip/RLeg/RFoot",
            "Hip/LLeg",
            "Hip/LLeg/LFoot"
        ]))

    def testSkelTransformDecomposition(self):
        """
        Tests that the decomposed transform values, when recomposed, recreate
        the correct Maya transformation matrix.
        """
        usdFile = os.path.abspath('UsdExportSkeleton.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', frameRange=[1, 30])
        stage = Usd.Stage.Open(usdFile)
        anim = UsdSkel.PackedJointAnimation.Get(stage,
                '/skeleton_Hip/Animation')
        self.assertEqual(anim.GetJointsAttr().Get()[8],
                "Hip/Spine/Neck/RArm")
        animT = anim.GetTranslationsAttr()
        animR = anim.GetRotationsAttr()
        animS = anim.GetScalesAttr()

        selList = OM.MSelectionList()
        selList.add("RArm")
        rArmDagPath = selList.getDagPath(0)
        fnTransform = OM.MFnTransform(rArmDagPath)

        for i in xrange(1, 31):
            cmds.currentTime(i, edit=True)

            mayaXf = fnTransform.transformation().asMatrix()
            usdT = animT.Get(i)[8]
            usdR = animR.Get(i)[8]
            usdS = animS.Get(i)[8]
            usdXf = UsdSkel.MakeTransform(usdT, usdR, usdS)
            self._AssertMatricesClose(usdXf, Gf.Matrix4d(*mayaXf))

if __name__ == '__main__':
    unittest.main(verbosity=2)
