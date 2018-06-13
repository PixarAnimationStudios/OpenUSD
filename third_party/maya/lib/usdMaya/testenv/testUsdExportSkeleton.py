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

from pxr import Gf, Sdf, Usd, UsdGeom, UsdSkel, Vt


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

    def testSkeletonTopology(self):
        """Tests that the joint topology is correct."""
        usdFile = os.path.abspath('UsdExportSkeleton.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', exportSkels='auto')
        stage = Usd.Stage.Open(usdFile)

        skeleton = UsdSkel.Skeleton.Get(stage, '/SkelChar/Hips/Skeleton')
        self.assertTrue(skeleton)

        joints = skeleton.GetJointsAttr().Get()
        self.assertEqual(joints, Vt.TokenArray([
            "Hips",
            "Hips/Torso",
            "Hips/Torso/Chest",
            "Hips/Torso/Chest/UpChest",
            "Hips/Torso/Chest/UpChest/Neck",
            "Hips/Torso/Chest/UpChest/Neck/Head",
            "Hips/Torso/Chest/UpChest/Neck/Head/LEye",
            "Hips/Torso/Chest/UpChest/Neck/Head/REye",
            "Hips/Torso/Chest/UpChest/LShldr",
            "Hips/Torso/Chest/UpChest/LShldr/LArm",
            "Hips/Torso/Chest/UpChest/LShldr/LArm/LElbow",
            "Hips/Torso/Chest/UpChest/LShldr/LArm/LElbow/LHand",
            "Hips/Torso/Chest/UpChest/RShldr",
            "Hips/Torso/Chest/UpChest/RShldr/RArm",
            "Hips/Torso/Chest/UpChest/RShldr/RArm/RElbow",
            "Hips/Torso/Chest/UpChest/RShldr/RArm/RElbow/RHand",
            # note: skips ExtraJoints because it's not a joint.
            "Hips/Torso/Chest/UpChest/RShldr/RArm/RElbow/RHand/ExtraJoints/RHandPropAttach",
            "Hips/LLeg",
            "Hips/LLeg/LKnee",
            "Hips/LLeg/LKnee/LFoot",
            "Hips/LLeg/LKnee/LFoot/LToes",
            "Hips/RLeg",
            "Hips/RLeg/RKnee",
            "Hips/RLeg/RKnee/RFoot",
            "Hips/RLeg/RKnee/RFoot/RToes"
        ]))

    def testSkelTransforms(self):
        """
        Tests that the computed joint transforms in USD, when tarnsformed into
        world space, match the world space transforms of the Maya joints.
        """

        # frameRange = [1, 30]
        frameRange = [1, 3]

        usdFile = os.path.abspath('UsdExportSkeleton.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', frameRange=frameRange,
                       exportSkels='auto')
        stage = Usd.Stage.Open(usdFile)

        root = UsdSkel.Root.Get(stage, '/SkelChar')
        self.assertTrue(root)

        skelCache = UsdSkel.Cache()
        skelCache.Populate(root)

        skelQuery = skelCache.GetSkelQuery(stage.GetPrimAtPath('/SkelChar/Hips'))
        self.assertTrue(skelQuery)

        xfCache = UsdGeom.XformCache()

        for frame in xrange(*frameRange):
            cmds.currentTime(frame, edit=True)
            xfCache.SetTime(frame)

            skelLocalToWorld = \
                skelQuery.ComputeLocalToWorldTransform(xfCache)

            usdJointXforms = skelQuery.ComputeJointSkelTransforms(frame)

            for joint,usdJointXf in zip(skelQuery.GetJointOrder(),
                                        usdJointXforms):

                usdJointWorldXf = usdJointXf * skelLocalToWorld
                
                selList = OM.MSelectionList()
                selList.add(Sdf.Path(joint).name)

                dagPath = selList.getDagPath(0)
                mayaJointWorldXf = Gf.Matrix4d(*dagPath.inclusiveMatrix())

                self.assertTrue(Gf.IsClose(mayaJointWorldXf,
                                           usdJointWorldXf, 1e-5))


if __name__ == '__main__':
    unittest.main(verbosity=2)
