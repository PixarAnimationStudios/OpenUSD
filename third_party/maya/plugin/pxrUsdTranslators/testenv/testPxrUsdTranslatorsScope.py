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

from pxr import Usd


class testPxrUsdTranslatorsScope(unittest.TestCase):

    USD_FILE = os.path.abspath('Scopes.usda')
    USD_FILE_OUT = os.path.abspath('Scopes.reexported.usda')

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    def testImportScope(self):
        cmds.usdImport(file=self.USD_FILE, primPath='/')
        dagObjects = cmds.ls(long=True, dag=True)

        self.assertIn('|A', dagObjects)
        self.assertEqual(cmds.nodeType('|A'), 'transform')
        self.assertFalse(cmds.getAttr('|A.tx', lock=True))

        self.assertIn('|A|A_1', dagObjects)
        self.assertEqual(cmds.nodeType('|A|A_1'), 'transform')
        self.assertEqual(cmds.getAttr('|A|A_1.USD_typeName'), 'Scope')
        self.assertTrue(cmds.getAttr('|A|A_1.tx', lock=True))

        self.assertIn('|A|A_1|A_1_I', dagObjects)
        self.assertEqual(cmds.nodeType('|A|A_1|A_1_I'), 'transform')
        self.assertFalse(cmds.getAttr('|A|A_1|A_1_I.tx', lock=True))

        self.assertIn('|A|A_1|A_1_II', dagObjects)
        self.assertEqual(cmds.nodeType('|A|A_1|A_1_II'), 'transform')
        self.assertFalse(cmds.getAttr('|A|A_1|A_1_II.tx', lock=True))

        self.assertIn('|A|A_1|A_1_III', dagObjects)
        self.assertEqual(cmds.nodeType('|A|A_1|A_1_III'), 'transform')
        self.assertEqual(cmds.getAttr('|A|A_1|A_1_III.USD_typeName'), 'Scope')
        self.assertTrue(cmds.getAttr('|A|A_1|A_1_III.tx', lock=True))

        self.assertIn('|A|A_2', dagObjects)
        self.assertEqual(cmds.nodeType('|A|A_2'), 'transform')
        self.assertEqual(cmds.getAttr('|A|A_2.USD_typeName'), 'Scope')
        self.assertTrue(cmds.getAttr('|A|A_2.tx', lock=True))

        self.assertIn('|B', dagObjects)
        self.assertEqual(cmds.nodeType('|B'), 'transform')
        self.assertEqual(cmds.getAttr('|B.USD_typeName'), 'Scope')
        self.assertTrue(cmds.getAttr('|B.tx', lock=True))

        self.assertIn('|B|B_1', dagObjects)
        self.assertEqual(cmds.nodeType('|B|B_1'), 'transform')
        self.assertFalse(cmds.getAttr('|B|B_1.tx', lock=True))

    def testReexportScope(self):
        cmds.usdImport(file=self.USD_FILE, primPath='/')
        cmds.usdExport(file=self.USD_FILE_OUT)

        stage = Usd.Stage.Open(self.USD_FILE_OUT)
        self.assertTrue(stage)

        self.assertTrue(stage.GetPrimAtPath('/A'))
        self.assertEqual(stage.GetPrimAtPath('/A').GetTypeName(), 'Xform')
        self.assertTrue(stage.GetPrimAtPath('/A/A_1'))
        self.assertEqual(stage.GetPrimAtPath('/A/A_1').GetTypeName(), 'Scope')
        self.assertTrue(stage.GetPrimAtPath('/A/A_1/A_1_I'))
        self.assertEqual(stage.GetPrimAtPath('/A/A_1/A_1_I').GetTypeName(),
                'Mesh')
        self.assertTrue(stage.GetPrimAtPath('/A/A_1/A_1_II'))
        self.assertEqual(stage.GetPrimAtPath('/A/A_1/A_1_II').GetTypeName(),
                'Camera')
        self.assertTrue(stage.GetPrimAtPath('/A/A_1/A_1_III'))
        self.assertEqual(stage.GetPrimAtPath('/A/A_1/A_1_III').GetTypeName(),
                'Scope')
        self.assertTrue(stage.GetPrimAtPath('/A/A_2'))
        self.assertEqual(stage.GetPrimAtPath('/A/A_2').GetTypeName(), 'Scope')
        self.assertTrue(stage.GetPrimAtPath('/B'))
        self.assertEqual(stage.GetPrimAtPath('/B').GetTypeName(), 'Scope')
        self.assertTrue(stage.GetPrimAtPath('/B/B_1'))
        self.assertEqual(stage.GetPrimAtPath('/B/B_1').GetTypeName(), 'Xform')


if __name__ == '__main__':
    unittest.main(verbosity=2)
