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

from pxr import UsdMaya

from maya import cmds
from maya import standalone

import unittest


class testUsdMayaBlockSceneModificationContext(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _AssertSceneIsModified(self, modified):
        isSceneModified = cmds.file(query=True, modified=True)
        self.assertEqual(isSceneModified, modified)

    def setUp(self):
        cmds.file(new=True, force=True)
        self._AssertSceneIsModified(False)

    def testPreserveSceneModified(self):
        """
        Tests that making scene modifications using a
        UsdMayaBlockSceneModificationContext on a scene that has already been
        modified correctly maintains the modification status after the context
        exits.
        """

        # Create a cube to dirty the scene.
        cmds.polyCube()
        self._AssertSceneIsModified(True)

        with UsdMaya.BlockSceneModificationContext():
            # Create a cube inside the context manager.
            cmds.polyCube()

        # The scene should still be modified.
        self._AssertSceneIsModified(True)

    def testPreserveSceneNotModified(self):
        """
        Tests that making scene modifications using a
        UsdMayaBlockSceneModificationContext on a scene that has not been
        modified correctly maintains the modification status after the context
        exits.
        """

        with UsdMaya.BlockSceneModificationContext():
            # Create a cube inside the context manager.
            cmds.polyCube()

        # The scene should NOT be modified.
        self._AssertSceneIsModified(False)
    

if __name__ == '__main__':
    unittest.main(verbosity=2)
