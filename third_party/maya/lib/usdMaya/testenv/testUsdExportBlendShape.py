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


class testUsdExportBlendShape(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportBlendshapesIsOff(self):
        """tests that blend shapes are not created when exportBlendShapes
        is not set.
        """
        cmds.file(os.path.abspath('UsdExportBlendShape.ma'),
                  open=True, force=True)

        usdFile = os.path.abspath('UsdDontExportBlendShape.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none')

        stage = Usd.Stage.Open(usdFile)

        baseMesh = UsdGeom.Mesh.Get(stage, '/root/box')
        self.assertTrue(baseMesh)

        target = UsdSkel.BlendShape.Get(stage, '/root/box/bs_boxShape')
        self.assertFalse(target)

    def testSingleBlendShape(self):
        """Tests that the blend shape is created as a child of the base mesh."""

        cmds.file(os.path.abspath('UsdExportBlendShape.ma'),
                  open=True, force=True)

        usdFile = os.path.abspath('UsdExportBlendShape.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', exportBlendShapes='auto')
        stage = Usd.Stage.Open(usdFile)

        baseMesh = UsdGeom.Mesh.Get(stage, '/root/box')
        self.assertTrue(baseMesh)

        target = UsdSkel.BlendShape.Get(stage, '/root/box/bs_box1Shape')
        self.assertTrue(target)

        bindingAPI = UsdSkel.BindingAPI(baseMesh.GetPrim())
        self.assertTrue(bindingAPI)

        blendShapeNames = bindingAPI.GetBlendShapesAttr().Get()
        self.assertEqual(blendShapeNames, Vt.TokenArray([
            "bs_box1"
        ]))

        blendShapeTargets = bindingAPI.GetBlendShapeTargetsRel().GetTargets()
        self.assertEqual(blendShapeTargets, [
            "/root/box/bs_box1Shape"
        ])


    def testMultipleBlendShapes(self):
        """Tests that multiple blend shapes are in the base mesh. Custom
        names have been given to the blend shape targets."""

        cmds.file(os.path.abspath('UsdExportBlendShapes2.ma'),
                  open=True, force=True)

        usdFile = os.path.abspath('UsdExportBlendShapes2.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', exportBlendShapes='auto')
        stage = Usd.Stage.Open(usdFile)

        baseMesh = UsdGeom.Mesh.Get(stage, '/root/box')
        self.assertTrue(baseMesh)

        target1 = UsdSkel.BlendShape.Get(stage, '/root/box/bs_box1Shape')
        self.assertTrue(target1)

        target2 = UsdSkel.BlendShape.Get(stage, '/root/box/bs_box2Shape')
        self.assertTrue(target2)

        bindingAPI = UsdSkel.BindingAPI(baseMesh.GetPrim())
        self.assertTrue(bindingAPI)

        blendShapeNames = bindingAPI.GetBlendShapesAttr().Get()
        self.assertEqual(blendShapeNames, Vt.TokenArray([
            "bigger",
            "wider",
            "bigger2"
        ]))

        blendShapeTargets = bindingAPI.GetBlendShapeTargetsRel().GetTargets()
        self.assertEqual(blendShapeTargets, [
            "/root/box/bs_box1Shape",
            "/root/box/bs_box2Shape",
            "/root/box/bigger2"
        ])

        # check inbetweens
        bigger2 = UsdSkel.BlendShape.Get(stage, '/root/box/bigger2')
        self.assertTrue(bigger2)
        self.assertTrue(bigger2.HasInbetween("bigger2_0_5"))
        self.assertTrue(bigger2.HasInbetween("bigger2_1_5"))

    def testCorrectBaseShape(self):
        """Tests that the actual base shape is used even if weights are
        set on the blend shape deformer."""

        cmds.file(os.path.abspath('UsdExportBlendShapes2.ma'),
                  open=True, force=True)

        # set the "wider" weight to 1
        cmds.setAttr("blendShape1.bigger", 1);

        usdFile = os.path.abspath('UsdExportCorrectBaseShape.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', exportBlendShapes='auto')
        stage = Usd.Stage.Open(usdFile)

        baseMesh = UsdGeom.Mesh.Get(stage, '/root/box')
        self.assertTrue(baseMesh)

        # Ensure that the resulting base shape is the actual base shape
        # ignoring the set weight on the blendshape. Just checking one
        # value here for simplicity.
        points = baseMesh.GetPointsAttr().Get()
        self.assertTrue(points)
        self.assertEqual(26, len(points))
        self.assertAlmostEqual(-0.5, points[0][1])

        # TODO: tests for animation

    def testWithCluster(self):
        """Tests that the blendshapes are properly created when attached to
        skin clusters."""

        cmds.file(os.path.abspath('UsdExportDeformingBlendShape.ma'),
                  open=True, force=True)

        usdFile = os.path.abspath('UsdExportDeformingBlendShape.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
                       shadingMode='none', exportBlendShapes='auto')
        stage = Usd.Stage.Open(usdFile)

        baseMesh = UsdGeom.Mesh.Get(stage, '/root/arm_r')
        self.assertTrue(baseMesh)

        target1 = UsdSkel.BlendShape.Get(stage, '/root/arm_r/bs_arm_r1Shape')
        self.assertTrue(target1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
