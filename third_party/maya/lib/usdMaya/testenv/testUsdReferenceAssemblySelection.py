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

from maya import OpenMaya as OM


class testUsdReferenceAssemblySelection(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testSoftSelection(self):
        # open maya file
        mayaFile = os.path.abspath('UsdReferenceAssemblySelectionTest.ma')
        cmds.file(mayaFile, open=True, force=True)

        # load all the assemblies
        self._LoadAll('Collapsed')

        # enable soft select with a medium radius
        OBJECT_MODE = 3
        cmds.softSelect(softSelectEnabled=1, softSelectFalloff=OBJECT_MODE,
            softSelectDistance=4.0)

        centerCube = 'Cube_49'
        neighborCube = 'Cube_48'

        neighborFullName = cmds.ls(neighborCube, long=True)[0]
        cmds.select(centerCube)

        # make sure more than 1 thing was selected
        softSel = self._GetSoftSelection()
        self.assertEqual(len(softSel), 9)
        self.assertTrue(self._ContainsAssembly(neighborFullName, softSel))

        # clear the selection
        cmds.select([])

        # set one of the things nearby and make it expanded
        cmds.assembly(neighborCube, edit=True, active='Expanded')

        # select thing again.  It should no longer include the thing that we've
        # made expanded.
        cmds.select(centerCube)
        softSel = self._GetSoftSelection()
        self.assertFalse(self._ContainsAssembly(neighborFullName, softSel))

    def _LoadAll(self, assemblyMode):
        for a in cmds.ls(type='pxrUsdReferenceAssembly'):
            cmds.assembly(a, edit=True, active=assemblyMode)

    def _GetSoftSelection(self):
        softSelection = OM.MRichSelection()
        OM.MGlobal.getRichSelection(softSelection)
        selection = OM.MSelectionList()
        softSelection.getSelection(selection)
        pathDag = OM.MDagPath()
        oComp = OM.MObject()

        ret = []
        for i in xrange(selection.length()):
            selection.getDagPath(i, pathDag, oComp)
            if not oComp.isNull():
                continue

            ret.append(pathDag.fullPathName())
        return ret

    # Note that the selection will actually contain the UsdProxyShape nodes,
    # not the actual assembly nodes.
    def _ContainsAssembly(self, assemblyFullName, fullNamesSelected):
        for n in fullNamesSelected:
            if n == assemblyFullName or n.startswith(assemblyFullName + '|'):
                return True
        return False


if __name__ == '__main__':
    unittest.main(verbosity=2)
