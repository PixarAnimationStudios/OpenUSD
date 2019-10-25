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

from pxr import Usd, UsdGeom


class testUsdExportConnected(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportConnectedPlug(self):
        # tests issue #656

        # here, visibility was connected to another plug that was not animated.
        cmds.file(os.path.abspath('visibility.ma'), open=True, force=True)

        usdFile = os.path.abspath('visibility.usda')
        cmds.usdExport(file=usdFile, exportVisibility=True, shadingMode='none')

        stage = Usd.Stage.Open(usdFile)

        p = stage.GetPrimAtPath('/driven')
        self.assertEqual(UsdGeom.Imageable(p).GetVisibilityAttr().Get(), 'invisible')

        p = stage.GetPrimAtPath('/invised')
        self.assertEqual(UsdGeom.Imageable(p).GetVisibilityAttr().Get(), 'invisible')

if __name__ == '__main__':
    unittest.main(verbosity=2)
