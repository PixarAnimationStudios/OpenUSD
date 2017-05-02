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

from maya import cmds
from maya import standalone

from pxr import Usd


class testUsdExportSelection(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportWithSelection(self):
        mayaFilePath = os.path.abspath('UsdExportSelectionTest.ma')
        cmds.file(mayaFilePath, open=True, force=True)

        selection = ['GroupA', 'Cube3', 'Cube6']
        cmds.select(selection)

        usdFilePath = os.path.abspath('UsdExportSelectionTest_EXPORTED.usda')
        cmds.usdExport(mergeTransformAndShape=True, selection=True,
            file=usdFilePath, shadingMode='none')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        expectedExportedPrims = [
            '/UsdExportSelectionTest',
            '/UsdExportSelectionTest/Geom',
            '/UsdExportSelectionTest/Geom/GroupA',
            '/UsdExportSelectionTest/Geom/GroupA/Cube1',
            '/UsdExportSelectionTest/Geom/GroupA/Cube2',
            '/UsdExportSelectionTest/Geom/GroupB',
            '/UsdExportSelectionTest/Geom/GroupB/Cube3',
            '/UsdExportSelectionTest/Geom/GroupC',
            '/UsdExportSelectionTest/Geom/GroupC/GroupD',
            '/UsdExportSelectionTest/Geom/GroupC/GroupD/Cube6',
        ]

        expectedNonExportedPrims = [
            '/UsdExportSelectionTest/Geom/GroupB/Cube4',
            '/UsdExportSelectionTest/Geom/GroupC/Cube5',
            '/UsdExportSelectionTest/Geom/GroupC/GroupD/Cube7',
        ]

        for primPath in expectedExportedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim.IsValid())

        for primPath in expectedNonExportedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertFalse(prim.IsValid())


if __name__ == '__main__':
    unittest.main(verbosity=2)
