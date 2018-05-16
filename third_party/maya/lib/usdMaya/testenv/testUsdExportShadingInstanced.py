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

        mayaFile = os.path.abspath('InstancedShading.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('InstancedShading.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
                shadingMode='displayColor', exportInstances=True)

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testInstancedGeom(self):
        """Tests that different shader bindings are correctly authored on
        instanced geometry."""
        redMat = "/World/Looks/blinn1SG"
        redPaths = ["/World/redCube", "/World/redSphere"]
        blueMat = "/World/Looks/phong1SG"
        bluePaths = [
                "/World/blueCube", "/World/blueSphere", "/World/blueSphere2"]
        instanceMasters = [
                "/InstanceSources/World_redSphere_blueSphereMultiAssignShape",
                "/InstanceSources/World_blueCube_blueCubeShape"]

        for path in redPaths:
            prim = self._stage.GetPrimAtPath(path)
            self.assertTrue(prim.IsInstance())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), redMat)
            self.assertEqual(rel.GetPrim().GetPath(), path)

        for path in bluePaths:
            prim = self._stage.GetPrimAtPath(path)
            self.assertTrue(prim.IsInstance())
            bindingAPI = UsdShade.MaterialBindingAPI(prim)
            mat, rel = bindingAPI.ComputeBoundMaterial()
            self.assertEqual(mat.GetPath(), blueMat)
            self.assertEqual(rel.GetPrim().GetPath(), path)

        for path in instanceMasters:
            prim = self._stage.GetPrimAtPath(path)
            self.assertTrue(prim)
            self.assertFalse(
                    prim.HasRelationship(UsdShade.Tokens.materialBinding))

    def testInstancedGeom_Subsets(self):
        """Tests that instanced geom with materials assigned to subsets are
        automatically de-instanced."""
        multiAssignPrim = self._stage.GetPrimAtPath(
                "/World/blueSphereMultiAssign")
        self.assertFalse(multiAssignPrim.IsInstanceable())

        subset1 = multiAssignPrim.GetChild("initialShadingGroup")
        self.assertTrue(subset1)
        mat, _ = UsdShade.MaterialBindingAPI(subset1).ComputeBoundMaterial()
        self.assertEqual(mat.GetPath(), "/World/Looks/initialShadingGroup")

        subset2 = multiAssignPrim.GetChild("blinn1SG")
        self.assertTrue(subset2)
        mat, _ = UsdShade.MaterialBindingAPI(subset2).ComputeBoundMaterial()
        self.assertEqual(mat.GetPath(), "/World/Looks/blinn1SG")

    def testUninstancedGeom(self):
        """Tests a basic case of non-instanced geometry with bindings."""
        redMat = self._stage.GetPrimAtPath("/World/Looks/blinn1SG")
        uninstancedPrim = self._stage.GetPrimAtPath("/World/notInstanced")

        self.assertFalse(uninstancedPrim.IsInstance())
        bindingAPI = UsdShade.MaterialBindingAPI(uninstancedPrim)
        mat, rel = bindingAPI.ComputeBoundMaterial()
        self.assertEqual(mat.GetPrim(), redMat)
        self.assertEqual(rel.GetPrim(), uninstancedPrim)


if __name__ == '__main__':
    unittest.main(verbosity=2)
