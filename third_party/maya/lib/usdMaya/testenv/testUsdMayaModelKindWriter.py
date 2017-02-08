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
from pxr import Kind
from pxr import Usd

from maya import cmds
from maya import standalone


class testUsdMayaModelKindWriter(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

    def testExportWithKindFlag(self):
        """
        Tests exporting a Maya file with no USD_kind custom attributes
        and using the usdExport -kind flag.
        """
        cmds.file(os.path.abspath('KindTest.ma'), open=True, force=True)
        cmds.loadPlugin('pxrUsd')

        usdFilePath = os.path.abspath('KindTest.usda')
        with self.assertRaises(RuntimeError):
                          cmds.usdExport(mergeTransformAndShape=True,
                                         file=usdFilePath,
                                         kind='assembly')

        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            kind='fakeKind')
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        rootPrim = stage.GetPrimAtPath('/KindTest')
        self.assertTrue(Kind.Registry().IsA(Usd.ModelAPI(rootPrim).GetKind(),
                'fakeKind'))

    def testExportWithKindAttrAndKindFlag(self):
        """
        Tests exporting a Maya file with both USD_kind custom attributes and
        using the usdExport -kind flag; there should be an error if the USD_kind
        is not derived from the kind specified in the -kind flag.
        """
        cmds.file(os.path.abspath('KindTestUsdKindAttr.ma'), open=True, force=True)
        cmds.loadPlugin('pxrUsd')

        usdFilePath = os.path.abspath('KindTestUsdKindAttr.usda')
        with self.assertRaises(RuntimeError):
            cmds.usdExport(mergeTransformAndShape=True,
                           file=usdFilePath,
                           kind='assembly')

        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            kind='model')
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        rootPrim = stage.GetPrimAtPath('/KindTest')
        self.assertTrue(Kind.Registry().IsA(Usd.ModelAPI(rootPrim).GetKind(),
                'component'))
        rootPrim2 = stage.GetPrimAtPath('/KindTest2')
        self.assertTrue(Kind.Registry().IsA(Usd.ModelAPI(rootPrim2).GetKind(),
                'assembly'))

    def testExportWithAssemblies(self):
        """
        Tests exporting a Maya file with a root prim containing an assembly.
        """
        cmds.file(os.path.abspath('KindTestAssembly.ma'), open=True, force=True)
        cmds.loadPlugin('pxrUsd')

        usdFilePath = os.path.abspath('KindTestAssembly.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            kind='assembly')
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        # Default kind without setting kind=assembly should still be assembly.
        usdFilePath = os.path.abspath('KindTestAssembly2.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath)
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        rootPrim = stage.GetPrimAtPath('/KindTest')
        self.assertTrue(Kind.Registry().IsA(Usd.ModelAPI(rootPrim).GetKind(),
                'assembly'))

    def testExportWithAssemblyAndMesh(self):
        """
        Tests exporting a Maya file with a root prim containing an assembly
        and a mesh.
        """
        cmds.file(os.path.abspath('KindTestAssemblyAndMesh.ma'), open=True,
                force=True)
        cmds.loadPlugin('pxrUsd')

        # Should fail due to the mesh.
        usdFilePath = os.path.abspath('KindTestAssemblyAndMesh.usda')
        with self.assertRaises(RuntimeError):
            cmds.usdExport(mergeTransformAndShape=True,
                           file=usdFilePath,
                           kind='assembly')

        # Should be 'component' because of the mesh
        usdFilePath = os.path.abspath('KindTestAssemblyAndMesh.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath)
        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        rootPrim = stage.GetPrimAtPath('/KindTest')
        self.assertTrue(Kind.Registry().IsA(Usd.ModelAPI(rootPrim).GetKind(),
                'component'))


if __name__ == '__main__':
    unittest.main(verbosity=2)
