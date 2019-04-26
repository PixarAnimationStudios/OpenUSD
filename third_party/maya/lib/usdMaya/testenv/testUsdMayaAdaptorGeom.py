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

from maya import cmds
from maya import standalone

import os
import unittest
from pxr import Usd, UsdGeom


class testUsdMayaAdaptorGeom(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

        usdFile = os.path.abspath('UsdAttrs.usda')
        cmds.usdImport(file=usdFile, shadingMode='none')

    def testImportImageable(self):
        """
        Tests that UsdGeomImageable.purpose is properly imported.
        """
        # Testing for the different purpose attributes
        self.assertEqual(cmds.getAttr('pCube1.USD_ATTR_purpose'), 'default')
        self.assertEqual(cmds.getAttr('pCube2.USD_ATTR_purpose'), 'render')
        self.assertEqual(cmds.getAttr('pCube3.USD_ATTR_purpose'), 'proxy')

        # pCube4 does not have a purpose attribute
        self.assertFalse(cmds.objExists('pCube4.USD_ATTR_purpose'))
        self.assertFalse(cmds.objExists('pCube4.USD_purpose')) # alias
    
    def testExportImageable(self):
        """
        Test that UsdGeomImageable.purpose is properly exported.
        """
        newUsdFilePath = os.path.abspath('UsdAttrsNew.usda')
        cmds.usdExport(file=newUsdFilePath, shadingMode='none')
        newUsdStage = Usd.Stage.Open(newUsdFilePath)
        
        # Testing the exported purpose attributes
        geom1 = UsdGeom.Imageable(newUsdStage.GetPrimAtPath('/World/pCube1'))
        self.assertEqual(geom1.GetPurposeAttr().Get(), 'default')
        geom2 = UsdGeom.Imageable(newUsdStage.GetPrimAtPath('/World/pCube2'))
        self.assertEqual(geom2.GetPurposeAttr().Get(), 'render')
        geom3 = UsdGeom.Imageable(newUsdStage.GetPrimAtPath('/World/pCube3'))
        self.assertEqual(geom3.GetPurposeAttr().Get(), 'proxy')

        # Testing that there is no authored attribute
        geom4 = UsdGeom.Imageable(newUsdStage.GetPrimAtPath('/World/pCube4'))
        self.assertFalse(geom4.GetPurposeAttr().HasAuthoredValue())

if __name__ == '__main__':
    unittest.main(verbosity=2)
