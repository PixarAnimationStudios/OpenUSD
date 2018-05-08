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

from maya import cmds
from maya import standalone

import os
import unittest
from pxr import Usd, UsdGeom

# XXX: The try/except here is temporary until we change the Pixar-internal
# package name to match the external package name.
try:
    from pxr import UsdMaya
except ImportError:
    from pixar import UsdMaya

class testUsdMayaAdaptorMetadata(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    def testImport_HiddenInstanceableKind(self):
        """
        Tests that the adaptor mechanism can import
        hidden, instanceable, and kind metadata properly.
        """
        cmds.file(new=True, force=True)
        usdFile = os.path.abspath('UsdAttrs.usda')
        cmds.usdImport(file=usdFile, shadingMode='none')

        # pCube1 and pCube2 have USD_kind.
        self.assertEqual(cmds.getAttr('pCube1.USD_kind'), 'potato')
        self.assertEqual(cmds.getAttr('pCube2.USD_kind'), 'bakedpotato')
        
        # pCube2, pCube4, and pCube5 have USD_hidden. pCube1 and pCube3 do not.
        self.assertTrue(
            cmds.attributeQuery('USD_hidden', node='pCube2', exists=True))
        self.assertTrue(
            cmds.attributeQuery('USD_hidden', node='pCube4', exists=True))
        self.assertTrue(
            cmds.attributeQuery('USD_hidden', node='pCube5', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_hidden', node='pCube1', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_hidden', node='pCube3', exists=True))

        self.assertTrue(cmds.getAttr('pCube2.USD_hidden'))
        self.assertTrue(cmds.getAttr('pCube4.USD_hidden'))
        self.assertFalse(cmds.getAttr('pCube5.USD_hidden'))
        
        # pCube3 and pCube4 have USD_instanceable.
        self.assertTrue(cmds.getAttr('pCube3.USD_instanceable'))
        self.assertTrue(cmds.getAttr('pCube4.USD_instanceable'))

    def testImport_HiddenInstanceable(self):
        """Tests import with only hidden, instanceable metadata and not kind."""
        cmds.file(new=True, force=True)
        usdFile = os.path.abspath('UsdAttrs.usda')
        cmds.usdImport(file=usdFile, shadingMode='none',
                metadata=['hidden', 'instanceable'])

        # pCube1 and pCube2 have USD_kind, but that shouldn't be imported.
        self.assertFalse(
            cmds.attributeQuery('USD_kind', node='pCube1', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_kind', node='pCube2', exists=True))
        
        # hidden should be imported.
        self.assertTrue(
            cmds.attributeQuery('USD_hidden', node='pCube2', exists=True))
        self.assertTrue(cmds.getAttr('pCube2.USD_hidden'))
        
        # instanceable should be imported.
        self.assertTrue(
            cmds.attributeQuery('USD_instanceable', node='pCube3', exists=True))
        self.assertTrue(cmds.getAttr('pCube3.USD_instanceable'))

    def testImport_TypeName(self):
        """Tests import with metadata=typeName manually specified."""
        cmds.file(new=True, force=True)
        usdFile = os.path.abspath('UsdAttrs.usda')
        cmds.usdImport(file=usdFile, shadingMode='none',
                metadata=["typeName"])

        # typeName metadata should come through.
        self.assertTrue(
            cmds.attributeQuery('USD_typeName', node='pCube1', exists=True))
        self.assertEqual(cmds.getAttr('pCube1.USD_typeName'), 'Mesh')
        self.assertTrue(
            cmds.attributeQuery('USD_typeName', node='World', exists=True))
        self.assertEqual(cmds.getAttr('World.USD_typeName'), 'Xform')

        # No USD_kind, hidden, or instanceable.
        self.assertFalse(
            cmds.attributeQuery('USD_kind', node='pCube1', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_hidden', node='pCube2', exists=True))
        self.assertFalse(
            cmds.attributeQuery('USD_instanceable', node='pCube3', exists=True))

    def testImport_GeomModelAPI(self):
        """Tests importing UsdGeomModelAPI attributes."""
        cmds.file(new=True, force=True)
        usdFile = os.path.abspath('UsdAttrs.usda')
        cmds.usdImport(file=usdFile, shadingMode='none',
                apiSchema=['GeomModelAPI'])

        worldProxy = UsdMaya.Adaptor('World')
        modelAPI = worldProxy.GetSchema(UsdGeom.ModelAPI)

        # model:cardGeometry
        self.assertTrue(
            cmds.attributeQuery('USD_ATTR_model_cardGeometry',
            node='World', exists=True))
        self.assertEqual(cmds.getAttr('World.USD_ATTR_model_cardGeometry'),
            UsdGeom.Tokens.fromTexture)
        self.assertEqual(
            modelAPI.GetAttribute(UsdGeom.Tokens.modelCardGeometry).Get(),
            UsdGeom.Tokens.fromTexture)

        # model:cardTextureXPos
        self.assertTrue(
            cmds.attributeQuery('USD_ATTR_model_cardTextureXPos',
            node='World', exists=True))
        self.assertEqual(cmds.getAttr('World.USD_ATTR_model_cardTextureXPos'),
            'right.png')
        self.assertEqual(
            modelAPI.GetAttribute(UsdGeom.Tokens.modelCardTextureXPos).Get(),
            'right.png')

    def testExport(self):
        """
        Tests that the adaptor mechanism can export
        USD_hidden, USD_instanceable, and USD_kind attributes by setting
        the correct metadata in the output USD file.
        """
        cmds.file(new=True, force=True)
        usdFile = os.path.abspath('UsdAttrs.usda')
        cmds.usdImport(file=usdFile, shadingMode='none')

        newUsdFilePath = os.path.abspath('UsdAttrsNew.usda')
        cmds.usdExport(file=newUsdFilePath, shadingMode='none')
        newUsdStage = Usd.Stage.Open(newUsdFilePath)
        
        # pCube1 and pCube2 have USD_kind.
        prim1 = newUsdStage.GetPrimAtPath('/World/pCube1')
        self.assertEqual(Usd.ModelAPI(prim1).GetKind(), 'potato')
        prim2 = newUsdStage.GetPrimAtPath('/World/pCube2')
        self.assertEqual(Usd.ModelAPI(prim2).GetKind(), 'bakedpotato')
        
        # pCube2, pCube4, and pCube5 have USD_hidden. pCube1 and pCube3 do not.
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube2').HasAuthoredHidden())
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube4').HasAuthoredHidden())
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube5').HasAuthoredHidden())
        self.assertFalse(newUsdStage.GetPrimAtPath('/World/pCube1').HasAuthoredHidden())
        self.assertFalse(newUsdStage.GetPrimAtPath('/World/pCube3').HasAuthoredHidden())

        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube2').IsHidden())
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube4').IsHidden())
        self.assertFalse(newUsdStage.GetPrimAtPath('/World/pCube5').IsHidden())
        
        # pCube3 and pCube4 have USD_instanceable.
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube3').IsInstanceable())
        self.assertTrue(newUsdStage.GetPrimAtPath('/World/pCube4').IsInstanceable())

    def testExport_GeomModelAPI(self):
        """Tests export with the GeomModelAPI."""
        cmds.file(new=True, force=True)
        usdFile = os.path.abspath('UsdAttrs.usda')
        cmds.usdImport(file=usdFile, shadingMode='none',
                apiSchema=['GeomModelAPI'])

        newUsdFilePath = os.path.abspath('UsdAttrsNew_GeomModelAPI.usda')
        cmds.usdExport(file=newUsdFilePath, shadingMode='none')
        newUsdStage = Usd.Stage.Open(newUsdFilePath)

        world = newUsdStage.GetPrimAtPath('/World')
        self.assertEqual(world.GetAppliedSchemas(), ['GeomModelAPI'])

        geomModelAPI = UsdGeom.ModelAPI(world)
        self.assertEqual(geomModelAPI.GetModelCardTextureXPosAttr().Get(),
                'right.png')

    def testExport_GeomModelAPI_MotionAPI(self):
        """Tests export with both the GeomModelAPI and MotionAPI."""
        cmds.file(new=True, force=True)
        usdFile = os.path.abspath('UsdAttrs.usda')
        cmds.usdImport(file=usdFile, shadingMode='none',
                apiSchema=['GeomModelAPI', 'MotionAPI'])

        newUsdFilePath = os.path.abspath('UsdAttrsNew_TwoAPIs.usda')
        cmds.usdExport(file=newUsdFilePath, shadingMode='none')
        newUsdStage = Usd.Stage.Open(newUsdFilePath)

        world = newUsdStage.GetPrimAtPath('/World')
        self.assertEqual(world.GetAppliedSchemas(),
                ['MotionAPI', 'GeomModelAPI'])

        geomModelAPI = UsdGeom.ModelAPI(world)
        self.assertEqual(geomModelAPI.GetModelCardTextureXPosAttr().Get(),
                'right.png')

        motionAPI = UsdGeom.MotionAPI(world)
        self.assertAlmostEqual(motionAPI.GetVelocityScaleAttr().Get(), 4.2,
                places=6)

if __name__ == '__main__':
    unittest.main(verbosity=2)
