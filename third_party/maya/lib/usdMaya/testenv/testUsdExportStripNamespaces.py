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


class testUsdExportStripNamespaces(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportWithClashStripping(self):
        mayaFilePath = os.path.abspath('UsdExportStripNamespaces.ma')
        cmds.file(mayaFilePath, new=True, force=True)

        node1 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube1" )
        cmds.namespace(add="foo")
        cmds.namespace(set="foo");
        node2 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube1" )
        cmds.namespace(set=":");

        usdFilePath = os.path.abspath('UsdExportStripNamespaces_EXPORTED.usda')

        errorRegexp = "Multiple dag nodes map to the same prim path" \
            ".+|cube1 - |foo:cube1.*"
        with self.assertRaisesRegexp(RuntimeError, errorRegexp) as cm:
            cmds.usdExport(mergeTransformAndShape=True,
                           selection=False,
                           stripNamespaces=True,
                           file=usdFilePath,
                           shadingMode='none')

        with self.assertRaisesRegexp(RuntimeError,errorRegexp) as cm:
            cmds.usdExport(mergeTransformAndShape=False,
                           selection=False,
                           stripNamespaces=True,
                           file=usdFilePath,
                           shadingMode='none')

    def testExportWithStripAndMerge(self):
        mayaFilePath = os.path.abspath('UsdExportStripNamespaces.ma')
        cmds.file(mayaFilePath, new=True, force=True)

        cmds.namespace(add=":foo")
        cmds.namespace(add=":bar")

        node1 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube1" )
        cmds.namespace(set=":foo");
        node2 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube2" )
        cmds.namespace(set=":bar");
        node3 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube3" )
        cmds.namespace(set=":");

        usdFilePath = os.path.abspath('UsdExportStripNamespaces_EXPORTED.usda')

        cmds.usdExport(mergeTransformAndShape=True,
                       selection=False,
                       stripNamespaces=True,
                       file=usdFilePath,
                       shadingMode='none')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        expectedPrims = ("/cube1", "/cube2", "/cube3")

        for primPath in expectedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim.IsValid(), "Expect " + primPath)

    def testExportWithStrip(self):
        mayaFilePath = os.path.abspath('UsdExportStripNamespaces.ma')
        cmds.file(mayaFilePath, new=True, force=True)

        cmds.namespace(add=":foo")
        cmds.namespace(add=":bar")

        node1 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube1" )
        cmds.namespace(set=":foo");
        node2 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube2" )
        cmds.namespace(set=":bar");
        node3 = cmds.polyCube( sx=5, sy=5, sz=5, name="cube3" )
        cmds.namespace(set=":");

        usdFilePath = os.path.abspath('UsdExportStripNamespaces_EXPORTED.usda')

        cmds.usdExport(mergeTransformAndShape=False,
                       selection=False,
                       stripNamespaces=True,
                       file=usdFilePath,
                       shadingMode='none')

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        expectedPrims = (
            "/cube1",
            "/cube1/cube1Shape",
            "/cube2",
            "/cube2/cube2Shape",
            "/cube3",
            "/cube3/cube3Shape"
        )

        for primPath in expectedPrims:
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim.IsValid(), "Expect " + primPath)

if __name__ == '__main__':
    unittest.main(verbosity=2)
