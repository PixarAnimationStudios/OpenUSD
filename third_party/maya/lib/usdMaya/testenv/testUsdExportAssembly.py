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

from pxr import Gf, Usd, UsdGeom, Vt

from maya import standalone
from maya import cmds


class testUsdExportAssembly(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        filepath = os.path.abspath('AssemblyTest.ma')
        cmds.file(filepath, open=True, force=True)

        usdFilePath = os.path.abspath('AssemblyTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True,
                       file=usdFilePath,
                       shadingMode='none')

        cls._stage = Usd.Stage.Open(usdFilePath)

    def testStageOpens(self):
        self.assertTrue(self._stage)

    def testBadCollapsedAssembly(self):
        """
        Tests that a collapsed assembly with Maya nodes underneath should
        skip all those Maya nodes during export.
        """
        badAssembly = self._stage.GetPrimAtPath('/BadCollapsedAssembly')
        children = badAssembly.GetAllChildren()
        self.assertEqual(len(children), 1) # 1 child via reference.

        phantomPrim = self._stage.GetPrimAtPath(
                "/BadCollapsedAssembly/PhantomCube")
        self.assertFalse(phantomPrim)

    def testGoodCollapsedAssembly(self):
        """
        Tests that a collapsed assembly without Maya nodes is exported
        normally.
        """
        goodAssembly = self._stage.GetPrimAtPath('/GoodCollapsedAssembly')
        children = goodAssembly.GetAllChildren()
        self.assertEqual(len(children), 1) # 1 child via reference.

        self.assertEqual(children[0].GetPath(),
                "/GoodCollapsedAssembly/DummyChild")

    def testAssemblyWithClasses(self):
        """
        Tests that classes were exported.
        """
        assembly = self._stage.GetPrimAtPath('/AssemblyWithClasses')
        self.assertTrue(assembly.HasAuthoredInherits())

    def testXformOpsIdentity(self):
        assembly = self._stage.GetPrimAtPath('/XformOpAssembly_Identity')
        xformable = UsdGeom.Xformable(assembly)
        self.assertFalse(xformable.GetXformOpOrderAttr().Get())
        self.assertEqual(xformable.GetLocalTransformation(), Gf.Matrix4d(1.0))

    def testXformOpsNonIdentity(self):
        assembly = self._stage.GetPrimAtPath('/XformOpAssembly_NonIdentity')
        xformable = UsdGeom.Xformable(assembly)
        self.assertEqual(xformable.GetXformOpOrderAttr().Get(),
                Vt.TokenArray(["xformOp:translate"]))
        self.assertEqual(xformable.GetLocalTransformation(),
                Gf.Matrix4d().SetTranslate(Gf.Vec3d(7.0, 8.0, 9.0)))

    def testXformOpsResetStack(self):
        assembly = self._stage.GetPrimAtPath(
                '/Group1/XformOpAssembly_NoInherit')
        xformable = UsdGeom.Xformable(assembly)
        self.assertTrue(xformable.GetResetXformStack())
        self.assertEqual(xformable.GetLocalTransformation(), Gf.Matrix4d(1.0))

if __name__ == '__main__':
    unittest.main(verbosity=2)
