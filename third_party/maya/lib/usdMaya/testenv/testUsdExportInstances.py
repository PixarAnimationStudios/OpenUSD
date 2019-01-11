#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Kind
from pxr import Usd
from pxr import UsdGeom


class testUsdExportInstances(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.file(os.path.abspath('UsdExportInstancesTest.ma'), open=True,
            force=True)

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportInstances(self):
        usdFile = os.path.abspath('UsdExportInstances_instances.usda')
        cmds.usdExport(mergeTransformAndShape=True, exportInstances=True,
            shadingMode='none', file=usdFile)

        stage = Usd.Stage.Open(usdFile)

        s = UsdGeom.Scope.Get(stage, '/InstanceSources')
        self.assertTrue(s.GetPrim().IsValid())

        expectedMeshPath = [
            '/InstanceSources/pCube1_pCubeShape1/Shape',
            '/InstanceSources/pCube1_pCube2_pCubeShape2/Shape'
        ]

        for each in expectedMeshPath:
            m = UsdGeom.Mesh.Get(stage, each)
            p = m.GetPrim()
            self.assertTrue(p.IsValid())            

        expectedXForm = [
            ('/pCube1', None),
            ('/pCube1/pCubeShape1', '/InstanceSources/pCube1_pCubeShape1'),
            ('/pCube1/pCube2', '/InstanceSources/pCube1_pCube2'),
            ('/pCube1/pCube3', '/InstanceSources/pCube1_pCube3'),
            ('/pCube4', None),
            ('/pCube4/pCubeShape1', '/InstanceSources/pCube1_pCubeShape1'),
            ('/pCube4/pCube2', '/InstanceSources/pCube1_pCube2'),
            ('/pCube4/pCube3', '/InstanceSources/pCube1_pCube3'),
            ('/InstanceSources/pCube1_pCube2/pCubeShape2',
                    '/InstanceSources/pCube1_pCube2_pCubeShape2'),
            ('/InstanceSources/pCube1_pCube3/pCubeShape2',
                    '/InstanceSources/pCube1_pCube2_pCubeShape2'),
        ]

        layer = stage.GetLayerStack()[1]

        for each in expectedXForm:
            pp = each[0]
            i = each[1]
            x = UsdGeom.Xform.Get(stage, pp)
            p = x.GetPrim()
            self.assertTrue(p.IsValid())
            if i is not None:
                self.assertTrue(p.IsInstanceable())
                self.assertTrue(p.IsInstance())
                ps = layer.GetPrimAtPath(pp)

                ref = ps.referenceList.GetAddedOrExplicitItems()[0]
                self.assertEqual(ref.assetPath, "")
                self.assertEqual(ref.primPath, i)

        # Test that the InstanceSources prim is last in the layer's root prims.
        rootPrims = layer.rootPrims.keys()
        self.assertEqual(rootPrims[-1], "InstanceSources")

    def testExportInstances_ModelHierarchyValidation(self):
        """Tests that model-hierarchy validation works with instances."""
        usdFile = os.path.abspath('UsdExportInstances_kinds.usda')
        with self.assertRaises(RuntimeError):
            # Should fail because assembly |pCube1 contains gprims.
            cmds.usdExport(mergeTransformAndShape=True, exportInstances=True,
                shadingMode='none', kind='assembly', file=usdFile)

    def testExportInstances_NoKindForInstanceSources(self):
        """Tests that the -kind export flag doesn't affect InstanceSources."""
        usdFile = os.path.abspath('UsdExportInstances_instancesources.usda')
        cmds.usdExport(mergeTransformAndShape=True, exportInstances=True,
            shadingMode='none', kind='component', file=usdFile)

        stage = Usd.Stage.Open(usdFile)

        instanceSources = Usd.ModelAPI.Get(stage, '/InstanceSources')
        self.assertEqual(instanceSources.GetKind(), '')

        pCube1 = Usd.ModelAPI.Get(stage, '/pCube1')
        self.assertEqual(pCube1.GetKind(), Kind.Tokens.component)

if __name__ == '__main__':
    unittest.main(verbosity=2)
