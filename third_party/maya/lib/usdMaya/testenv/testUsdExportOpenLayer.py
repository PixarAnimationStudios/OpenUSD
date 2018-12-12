#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the Apache License, Version 2.0 (the 'Apache License')
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
# distributed on an 'AS IS' BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

import os
import unittest

from pxr import Usd

from maya import cmds
from maya import standalone


class testUsdExportOpenLayer(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportToDiskLayer(self):
        """
        Tests that exporting to an on-disk layer that is open elsewhere in the
        process still works.
        """
        cmds.file(new=True, force=True)
        cmds.polyCube(name='TestCube')

        filePath = os.path.abspath("testStage.usda")
        stage = Usd.Stage.CreateNew(filePath)
        stage.Save()
        self.assertFalse(stage.GetPrimAtPath('/TestCube').IsValid())

        cmds.usdExport(
                file=filePath,
                mergeTransformAndShape=True,
                shadingMode='none')
        self.assertTrue(stage.GetPrimAtPath('/TestCube').IsValid())

        cmds.rename('TestCube', 'TestThing')
        cmds.usdExport(
                file=filePath,
                mergeTransformAndShape=True,
                shadingMode='none')
        self.assertFalse(stage.GetPrimAtPath('/TestCube').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/TestThing').IsValid())

    def testExportToAnonymousLayer(self):
        """
        Tests exporting to an existing anonymous layer. In normal (non-append)
        mode, this should completely overwrite the contents of the anonymous
        layer.
        """
        cmds.file(new=True, force=True)
        cmds.polyCube(name='TestCube')

        stage = Usd.Stage.CreateInMemory()
        self.assertFalse(stage.GetPrimAtPath('/TestCube').IsValid())

        cmds.usdExport(
                file=stage.GetRootLayer().identifier,
                mergeTransformAndShape=True,
                shadingMode='none')
        self.assertTrue(stage.GetPrimAtPath('/TestCube').IsValid())

        cmds.rename('TestCube', 'TestThing')
        cmds.usdExport(
                file=stage.GetRootLayer().identifier,
                mergeTransformAndShape=True,
                shadingMode='none')
        self.assertFalse(stage.GetPrimAtPath('/TestCube').IsValid())
        self.assertTrue(stage.GetPrimAtPath('/TestThing').IsValid())


if __name__ == '__main__':
    unittest.main(verbosity=2)
