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

from pxr import Usd
from pxr import UsdShade

from maya import cmds
from maya import standalone


class testUsdExportShadingInstanced(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        # Stage with simple (non-nested) instancing.
        mayaFile = os.path.abspath('InstancedShading.ma')
        cmds.file(mayaFile, open=True, force=True)

        usdFilePath = os.path.abspath('InstancedShading.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
                shadingMode='displayColor', exportInstances=True,
                exportCollectionBasedBindings=True,
                exportMaterialCollections=True,
                materialCollectionsPath="/World")

        cls._simpleStage = Usd.Stage.Open(usdFilePath)

        # Stage with nested instancing.
        mayaFile = os.path.abspath('NestedInstancedShading.ma')
        cmds.file(mayaFile, open=True, force=True)

        usdFilePath = os.path.abspath('NestedInstancedShading.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
                shadingMode='displayColor', exportInstances=True,
                exportCollectionBasedBindings=True,
                exportMaterialCollections=True,
                materialCollectionsPath="/World")

        cls._nestedStage = Usd.Stage.Open(usdFilePath)


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testInstancedGeom(self):
        """Tests that different shader bindings are correctly authored on
        instanced geometry."""
        worldPath = "/World" # Where collections are authored
        redMat = "/World/Materials/blinn1SG"
        redPaths = ["/World/redCube", "/World/redSphere"]
        blueMat = "/World/Materials/phong1SG"
        bluePaths = [
                "/World/blueCube", "/World/blueSphere", "/World/blueSphere2"]
        instanceMasters = [
                "/InstanceSources/World_redSphere_blueSphereMultiAssignShape",
                "/InstanceSources/World_blueCube_blueCubeShape"]

        for path in redPaths:
            prim = self._simpleStage.GetPrimAtPath(path)
            self.assertTrue(prim.IsInstance())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), redMat)
            self.assertEqual(rel.GetPrim().GetPath(), worldPath)

        for path in bluePaths:
            prim = self._simpleStage.GetPrimAtPath(path)
            self.assertTrue(prim.IsInstance())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), blueMat)
            self.assertEqual(rel.GetPrim().GetPath(), worldPath)

        for path in instanceMasters:
            prim = self._simpleStage.GetPrimAtPath(path)
            self.assertTrue(prim)
            self.assertFalse(
                    prim.HasRelationship(UsdShade.Tokens.materialBinding))

    def testInstancedGeom_Subsets(self):
        """Tests that instanced geom with materials assigned to subsets are
        automatically de-instanced."""
        multiAssignPrim = self._simpleStage.GetPrimAtPath(
                "/World/blueSphereMultiAssign")
        self.assertFalse(multiAssignPrim.IsInstanceable())

        shape = multiAssignPrim.GetChild("Shape")
        self.assertFalse(shape.IsInstance())

        subset1 = shape.GetChild("initialShadingGroup")
        self.assertTrue(subset1)
        mat, _ = UsdShade.MaterialBindingAPI(subset1).ComputeBoundMaterial()
        self.assertEqual(mat.GetPath(), "/World/Materials/initialShadingGroup")

        subset2 = shape.GetChild("blinn1SG")
        self.assertTrue(subset2)
        mat, _ = UsdShade.MaterialBindingAPI(subset2).ComputeBoundMaterial()
        self.assertEqual(mat.GetPath(), "/World/Materials/blinn1SG")

    def testUninstancedGeom(self):
        """Tests a basic case of non-instanced geometry with bindings."""
        worldPath = "/World" # Where collections are authored
        redMat = self._simpleStage.GetPrimAtPath("/World/Materials/blinn1SG")
        uninstancedPrim = self._simpleStage.GetPrimAtPath("/World/notInstanced")

        self.assertFalse(uninstancedPrim.IsInstance())
        bindingAPI = UsdShade.MaterialBindingAPI(uninstancedPrim)
        mat, rel = bindingAPI.ComputeBoundMaterial()
        self.assertEqual(mat.GetPrim(), redMat)
        self.assertEqual(rel.GetPrim().GetPath(), worldPath)

    def testNestedInstancedGeom(self):
        """Tests that different shader bindings are correctly authored on
        instanced geometry within nested instances."""
        worldPath = "/World" # Where collections are authored
        greenMat = "/World/Materials/blinn1SG"
        greenPaths = [
                "/World/SimpleInstance1/Shape",
                "/World/ComplexA/NestedA/Base1/BaseShape1",
                "/World/ComplexA/NestedB/Base1/BaseShape1",
                "/World/Extra/Base3/Shape",
                "/World/ComplexB/NestedA/Base1/BaseShape1",
                "/World/ComplexB/NestedB/Base1/BaseShape1"]
        blueMat = "/World/Materials/blinn2SG"
        bluePaths = [
                "/World/SimpleInstance2/Shape",
                "/World/ComplexA/NestedA/Base2/BaseShape1",
                "/World/ComplexA/NestedB/Base2/BaseShape1",
                "/World/ComplexB/NestedA/Base2/BaseShape1",
                "/World/ComplexB/NestedB/Base2/BaseShape1"]
        instanceMasters = [
                "/InstanceSources/World_ComplexA_NestedA_Base1_BaseShape1" +
                    "/Shape",
                "/InstanceSources/World_SimpleInstance1_SimpleInstanceShape1" +
                    "/Shape"]

        for path in greenPaths:
            prim = self._nestedStage.GetPrimAtPath(path)
            self.assertTrue(prim, msg=path)
            self.assertTrue(prim.IsInstanceProxy())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), greenMat)
            self.assertEqual(rel.GetPrim().GetPath(), worldPath)

        for path in bluePaths:
            prim = self._nestedStage.GetPrimAtPath(path)
            self.assertTrue(prim, msg=path)
            self.assertTrue(prim.IsInstanceProxy())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), blueMat)
            self.assertEqual(rel.GetPrim().GetPath(), worldPath)

        for path in instanceMasters:
            prim = self._nestedStage.GetPrimAtPath(path)
            self.assertTrue(prim)
            self.assertFalse(
                    prim.HasRelationship(UsdShade.Tokens.materialBinding))


if __name__ == '__main__':
    unittest.main(verbosity=2)
