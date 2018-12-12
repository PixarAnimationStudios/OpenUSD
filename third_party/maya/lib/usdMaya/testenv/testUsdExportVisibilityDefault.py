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

from pxr import Usd
from pxr import UsdGeom

from maya import cmds
from maya import standalone

class testUsdExportVisibilityDefault(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')
        cmds.file(os.path.abspath('UsdExportVisibilityDefaultTest.ma'),
                  open=True,
                  force=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testCurrentFrameVisibilityAsDefault(self):
        # Makes sure that the current frame's visibility is exported as default.
        for frame in (1, 2):
            cmds.currentTime(frame)
            cmds.usdExport(file=os.path.abspath("out_%d.usda" % frame),
                    exportVisibility=True)

        out_1 = Usd.Stage.Open('out_1.usda')
        self.assertEqual(UsdGeom.Imageable.Get(out_1, '/group').ComputeVisibility(), UsdGeom.Tokens.invisible)
        self.assertEqual(UsdGeom.Imageable.Get(out_1, '/group_inverse').ComputeVisibility(), UsdGeom.Tokens.inherited)

        out_2 = Usd.Stage.Open('out_2.usda')
        self.assertEqual(UsdGeom.Imageable.Get(out_2, '/group').ComputeVisibility(), UsdGeom.Tokens.inherited)
        self.assertEqual(UsdGeom.Imageable.Get(out_2, '/group_inverse').ComputeVisibility(), UsdGeom.Tokens.invisible)

if __name__ == '__main__':
    unittest.main(verbosity=2)

