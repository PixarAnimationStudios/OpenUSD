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

# XXX: The try/except here is temporary until we change the Pixar-internal
# package name to match the external package name.
try:
    from pxr import UsdMaya
except ImportError:
    from pixar import UsdMaya

from pxr import Sdf

from maya import cmds
from maya import standalone

import os
import unittest


class testUsdMayaReferenceAssemblyEdits(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def setUp(self):
        cmds.file(new=True, force=True)

    @staticmethod
    def _CreateAssemblyNode(nodeName='TestAssemblyNode'):
        """
        Creates a model reference assembly node for testing.
        """
        ASSEMBLY_TYPE_NAME = 'pxrUsdReferenceAssembly'
        assetName = 'CubeModel'
        usdFile = os.path.abspath('%s.usda' % assetName)
        primPath = '/%s' % assetName

        assemblyNodeName = cmds.assembly(name=nodeName, type=ASSEMBLY_TYPE_NAME)
        cmds.setAttr('%s.filePath' % assemblyNodeName, usdFile, type='string')
        cmds.setAttr('%s.primPath' % assemblyNodeName, primPath, type='string')

        return assemblyNodeName

    def _GetAssemblyEdit(self, assemblyNodeName, editPath):
        """
        Gets the assembly edit on the USD prim at editPath for the Maya assembly
        node assemblyNodeName.
        It is assumed that there is only ever one edit per editPath, and no
        invalid edits.
        """
        (assemEdits, invalidEdits) = UsdMaya.EditUtil.GetEditsForAssembly(
            assemblyNodeName)
        self.assertEqual(len(assemEdits), 1)
        self.assertEqual(invalidEdits, [])

        self.assertIn(editPath, assemEdits)
        self.assertEqual(len(assemEdits[editPath]), 1)

        refEdit = assemEdits[editPath][0]
        return refEdit

    def _MakeAssemblyEdit(self, assemblyNodeName, editNodeName):
        """
        Creates an assembly edit on the Maya node editNodeName inside the Maya
        assembly assemblyNodeName.
        """
        editNode = 'NS_%s:%s' % (assemblyNodeName, editNodeName)
        editAttr = '%s.tx' % editNode
        cmds.setAttr(editAttr, 5.0)

        # Verify that the edit was made correctly.
        editPath = Sdf.Path('Geom/%s' % editNodeName)
        refEdit = self._GetAssemblyEdit(assemblyNodeName, editPath)

        expectedEditString = 'setAttr "NS_{nodeName}:Geom|NS_{nodeName}:Cube.translateX" 5'.format(
            nodeName=assemblyNodeName)
        self.assertEqual(refEdit.editString, expectedEditString)

    def testAssemblyEditsAfterRename(self):
        """
        Tests that assembly edits made prior to a rename are still present on
        the assembly after it has been renamed.
        """
        # Create the initial assembly and activate its 'Full' representation.
        assemblyNodeName = self._CreateAssemblyNode()
        cmds.assembly(assemblyNodeName, edit=True, active='Full')

        # Make an edit on a node inside the assembly.
        self._MakeAssemblyEdit(assemblyNodeName, 'Cube')

        # Now do the rename.
        cmds.rename(assemblyNodeName, 'FooBar')

        editPath = Sdf.Path('Geom/Cube')

        # Because the rename does not change the 'repNamespace' attribute on
        # the assembly, it should still be the same as it was before the rename.
        refEdit = self._GetAssemblyEdit('FooBar', editPath)
        expectedEditString = 'setAttr "NS_TestAssemblyNode:Geom|NS_TestAssemblyNode:Cube.translateX" 5'
        self.assertEqual(refEdit.editString, expectedEditString)

    # XXX: Maya's built-in duplicate() command does NOT copy assembly edits.
    #      Hopefully one day it will, and we can enable this test.
    # def testAssemblyEditsAfterDuplicate(self):
    #     """
    #     Tests that assembly edits made on an assembly node are preserved on the
    #     original node AND transferred to the duplicate node correctly when the
    #     original node is duplicated.
    #     """
    #     # Create the initial assembly and activate its 'Full' representation.
    #     assemblyNodeName = self._CreateAssemblyNode()
    #     cmds.assembly(assemblyNodeName, edit=True, active='Full')
    #
    #     # Make an edit on a node inside the assembly.
    #     self._MakeAssemblyEdit(assemblyNodeName, 'Cube')
    #
    #     # Now do the duplicate.
    #     cmds.duplicate(assemblyNodeName, name='DuplicateAssembly')
    #
    #     editPath = Sdf.Path('Geom/Cube')
    #
    #     # Verify the edit on the original assembly node.
    #     refEdit = self._GetAssemblyEdit(assemblyNodeName, editPath)
    #     expectedEditString = 'setAttr "NS_{nodeName}:Geom|NS_{nodeName}:Cube.translateX" 5'.format(
    #         nodeName=assemblyNodeName)
    #     self.assertEqual(refEdit.editString, expectedEditString)
    #
    #     # Verify the edit on the duplicate assembly node.
    #     refEdit = self._GetAssemblyEdit('DuplicateAssembly', editPath)
    #     expectedEditString = 'setAttr "NS_{nodeName}:Geom|NS_{nodeName}:Cube.translateX" 5'.format(
    #         nodeName='DuplicateAssembly')
    #     self.assertEqual(refEdit.editString, expectedEditString)


if __name__ == '__main__':
    unittest.main(verbosity=2)
