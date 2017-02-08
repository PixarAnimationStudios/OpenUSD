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
from maya.api import OpenMaya as OM
from maya import standalone

from pxr import Gf


class testUsdImportUVSets(unittest.TestCase):

    def _AssertUVSet(self, mesh, uvSetName, expectedValues, expectedNumValues=None):
        """
        Verifies that the UV values for the uv set named uvSetName on the
        MFnMesh mesh match the values in expectedValues. expectedValues should
        be a dictionary mapping mesh-level face vertex indices to UV values.
        A face vertex index missing from the dictionary means that that face
        vertex should NOT have an assigned UV value on the Maya mesh.
        If multiple face vertices map to the same UV value and the number of UV
        values in the UV set is smaller than the number of assignments, pass in
        the expected number of UV values in the UV set as expectedNumValues.
        """
        if expectedNumValues is None:
            expectedNumValues = len(expectedValues)
        actualNumValues = mesh.numUVs(uvSetName)
        self.assertEqual(expectedNumValues, actualNumValues)

        itMeshFV = OM.MItMeshFaceVertex(mesh.object())
        itMeshFV.reset()
        fvi = 0
        while not itMeshFV.isDone():
            expectedUV = expectedValues.get(fvi)
            if expectedUV is None:
                self.assertFalse(itMeshFV.hasUVs(uvSetName))
            else:
                self.assertTrue(itMeshFV.hasUVs(uvSetName))

                actualUV = itMeshFV.getUV(uvSetName)
                self.assertAlmostEqual(expectedUV, actualUV)

            itMeshFV.next()
            fvi += 1

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

        usdFile = os.path.abspath('UsdImportUVSetsTest.usda')
        cmds.usdImport(file=usdFile, shadingMode='none')

    def _GetMayaMesh(self, meshName):
        selectionList = OM.MSelectionList()
        selectionList.add(meshName)
        mObj = selectionList.getDependNode(0)

        return OM.MFnMesh(mObj)

    def testImportNoUVSets(self):
        """
        Tests that importing a USD cube mesh with no UV set primvars results in
        the default 'map1' UV set on the Maya mesh being empty.
        """
        mayaCubeMesh = self._GetMayaMesh('NoUVSetsCubeShape')
        expectedValues = {}
        self._AssertUVSet(mayaCubeMesh, 'map1', expectedValues)

    def testImportDefaultUVSet(self):
        """
        Tests that a USD cube mesh with the Maya default values for the default
        UV set (named 'st' in USD) gets imported correctly as 'map1'.
        """
        mayaCubeMesh = self._GetMayaMesh('DefaultUVSetCubeShape')

        # These are the default UV values for a regular Maya polycube.
        expectedValues = {
            0: Gf.Vec2f(0.375, 0.0),
            1: Gf.Vec2f(0.625, 0.0),
            2: Gf.Vec2f(0.625, 0.25),
            3: Gf.Vec2f(0.375, 0.25),
            4: Gf.Vec2f(0.375, 0.25),
            5: Gf.Vec2f(0.625, 0.25),
            6: Gf.Vec2f(0.625, 0.5),
            7: Gf.Vec2f(0.375, 0.5),
            8: Gf.Vec2f(0.375, 0.5),
            9: Gf.Vec2f(0.625, 0.5),
            10: Gf.Vec2f(0.625, 0.75),
            11: Gf.Vec2f(0.375, 0.75),
            12: Gf.Vec2f(0.375, 0.75),
            13: Gf.Vec2f(0.625, 0.75),
            14: Gf.Vec2f(0.625, 1.0),
            15: Gf.Vec2f(0.375, 1.0),
            16: Gf.Vec2f(0.625, 0.0),
            17: Gf.Vec2f(0.875, 0.0),
            18: Gf.Vec2f(0.875, 0.25),
            19: Gf.Vec2f(0.625, 0.25),
            20: Gf.Vec2f(0.125, 0.0),
            21: Gf.Vec2f(0.375, 0.0),
            22: Gf.Vec2f(0.375, 0.25),
            23: Gf.Vec2f(0.125, 0.25)
        }

        self._AssertUVSet(mayaCubeMesh, 'map1', expectedValues)

    def testImportOneMissingFaceUVSet(self):
        """
        Tests that a USD cube mesh with values for all but one face in the
        default UV set (named 'st' in USD) gets imported correctly as 'map1'.
        """
        mayaCubeMesh = self._GetMayaMesh('OneMissingFaceCubeShape')

        expectedValues = {
            0: Gf.Vec2f(0.375, 0),
            1: Gf.Vec2f(0.625, 0),
            2: Gf.Vec2f(0.625, 0.25),
            3: Gf.Vec2f(0.375, 0.25),
            4: Gf.Vec2f(0.375, 0.25),
            5: Gf.Vec2f(0.625, 0.25),
            6: Gf.Vec2f(0.625, 0.5),
            7: Gf.Vec2f(0.375, 0.5),
            12: Gf.Vec2f(0.375, 0.75),
            13: Gf.Vec2f(0.625, 0.75),
            14: Gf.Vec2f(0.625, 1),
            15: Gf.Vec2f(0.375, 1),
            16: Gf.Vec2f(0.625, 0),
            17: Gf.Vec2f(0.875, 0),
            18: Gf.Vec2f(0.875, 0.25),
            19: Gf.Vec2f(0.625, 0.25),
            20: Gf.Vec2f(0.125, 0),
            21: Gf.Vec2f(0.375, 0),
            22: Gf.Vec2f(0.375, 0.25),
            23: Gf.Vec2f(0.125, 0.25)
        }

        self._AssertUVSet(mayaCubeMesh, 'map1', expectedValues)

    def testImportOneAssignedFaceUVSet(self):
        """
        Tests that a USD cube mesh with values for only one face in the default
        UV set (named 'st' in USD) gets imported correctly as 'map1'.
        """
        mayaCubeMesh = self._GetMayaMesh('OneAssignedFaceCubeShape')

        expectedValues = {
            8: Gf.Vec2f(0.375, 0.5),
            9: Gf.Vec2f(0.625, 0.5),
            10: Gf.Vec2f(0.625, 0.75),
            11: Gf.Vec2f(0.375, 0.75)
        }

        self._AssertUVSet(mayaCubeMesh, 'map1', expectedValues)

    def testImportCompressibleUVSets(self):
        """
        Tests that UV sets on a USD cube mesh that were compressed to constant,
        uniform, and vertex interpolations are imported correctly.

        Note that the actual values here don't really make sense as UV sets.
        """
        mayaCubeMesh = self._GetMayaMesh('CompressibleUVSetsCubeShape')

        # ALL face vertices should have the same value.
        uvSetName = 'ConstantInterpSet'
        expectedValues = {}
        for i in xrange(24):
            expectedValues[i] = Gf.Vec2f(0.25, 0.25)
        self._AssertUVSet(mayaCubeMesh, uvSetName, expectedValues,
            expectedNumValues=1)

        # All face vertices within the same face should have the same value.
        uvSetName = 'UniformInterpSet'
        expectedValues = {}
        for i in xrange(0, 4):
            expectedValues[i] = Gf.Vec2f(0.0, 0.0)
        for i in xrange(4, 8):
            expectedValues[i] = Gf.Vec2f(0.1, 0.1)
        for i in xrange(8, 12):
            expectedValues[i] = Gf.Vec2f(0.2, 0.2)
        for i in xrange(12, 16):
            expectedValues[i] = Gf.Vec2f(0.3, 0.3)
        for i in xrange(16, 20):
            expectedValues[i] = Gf.Vec2f(0.4, 0.4)
        for i in xrange(20, 24):
            expectedValues[i] = Gf.Vec2f(0.5, 0.5)
        self._AssertUVSet(mayaCubeMesh, uvSetName, expectedValues,
            expectedNumValues=6)

        # All face vertices on the same mesh vertex (indices 0-7 for a cube)
        # should have the same value.
        uvSetName = 'VertexInterpSet'
        expectedValues = {
            0 : Gf.Vec2f(0.0, 0.0),
            1 : Gf.Vec2f(0.1, 0.1),
            2 : Gf.Vec2f(0.3, 0.3),
            3 : Gf.Vec2f(0.2, 0.2),
            4 : Gf.Vec2f(0.2, 0.2),
            5 : Gf.Vec2f(0.3, 0.3),
            6 : Gf.Vec2f(0.5, 0.5),
            7 : Gf.Vec2f(0.4, 0.4),
            8 : Gf.Vec2f(0.4, 0.4),
            9 : Gf.Vec2f(0.5, 0.5),
            10 : Gf.Vec2f(0.7, 0.7),
            11 : Gf.Vec2f(0.6, 0.6),
            12 : Gf.Vec2f(0.6, 0.6),
            13 : Gf.Vec2f(0.7, 0.7),
            14 : Gf.Vec2f(0.1, 0.1),
            15 : Gf.Vec2f(0.0, 0.0),
            16 : Gf.Vec2f(0.1, 0.1),
            17 : Gf.Vec2f(0.7, 0.7),
            18 : Gf.Vec2f(0.5, 0.5),
            19 : Gf.Vec2f(0.3, 0.3),
            20 : Gf.Vec2f(0.6, 0.6),
            21 : Gf.Vec2f(0.0, 0.0),
            22 : Gf.Vec2f(0.2, 0.2),
            23 : Gf.Vec2f(0.4, 0.4)
        }
        self._AssertUVSet(mayaCubeMesh, uvSetName, expectedValues,
            expectedNumValues=8)

    def testImportSharedFacesUVSets(self):
        """
        Tests that UV sets on a USD cube mesh that use the same UV ranges for
        multiple faces are imported correctly.
        """
        mayaCubeMesh = self._GetMayaMesh('SharedFacesCubeShape')

        # All six faces share the same range 0.0-1.0.
        uvSetName = 'AllFacesSharedSet'
        expectedValues = {}
        for i in xrange(0, 24, 4):
            expectedValues[i] = Gf.Vec2f(0.0, 0.0)
        for i in xrange(1, 24, 4):
            expectedValues[i] = Gf.Vec2f(1.0, 0.0)
        for i in xrange(2, 24, 4):
            expectedValues[i] = Gf.Vec2f(1.0, 1.0)
        for i in xrange(3, 24, 4):
            expectedValues[i] = Gf.Vec2f(0.0, 1.0)
        self._AssertUVSet(mayaCubeMesh, uvSetName, expectedValues)

        # The faces alternate between ranges 0.0-0.5 and 0.5-1.0.
        uvSetName = 'PairedFacesSet'
        expectedValues = {}
        for i in xrange(0, 24, 8):
            expectedValues[i] = Gf.Vec2f(0.0, 0.0)
        for i in xrange(1, 24, 8):
            expectedValues[i] = Gf.Vec2f(0.5, 0.0)
        for i in xrange(2, 24, 8):
            expectedValues[i] = Gf.Vec2f(0.5, 0.5)
        for i in xrange(3, 24, 8):
            expectedValues[i] = Gf.Vec2f(0.0, 0.5)
        for i in xrange(4, 24, 8):
            expectedValues[i] = Gf.Vec2f(0.5, 0.5)
        for i in xrange(5, 24, 8):
            expectedValues[i] = Gf.Vec2f(1.0, 0.5)
        for i in xrange(6, 24, 8):
            expectedValues[i] = Gf.Vec2f(1.0, 1.0)
        for i in xrange(7, 24, 8):
            expectedValues[i] = Gf.Vec2f(0.5, 1.0)
        self._AssertUVSet(mayaCubeMesh, uvSetName, expectedValues)
    
    def testImportUVSetForMeshWithCreases(self):
        """
        Tests that importing a mesh with creases doesn't crash when importing
        UVs and that the UV set is imported properly.
        """

        # We need to load this mesh from a separate USD file because importing
        # it caused a crash (that this test verifies should no longer happen).
        usdFile = os.path.abspath('UsdImportUVSetsTestWithCreases.usda')

        # We also need to load it using the Maya file import command because
        # going through the usdImport command works fine but using the file
        # translator caused a crash.
        cmds.file(usdFile, i=True)

        mayaCubeMesh = self._GetMayaMesh('CreasedCubeShape')

        # These are the default UV values for a regular Maya polycube.
        expectedValues = {
            0: Gf.Vec2f(0.375, 0.0),
            1: Gf.Vec2f(0.625, 0.0),
            2: Gf.Vec2f(0.625, 0.25),
            3: Gf.Vec2f(0.375, 0.25),
            4: Gf.Vec2f(0.375, 0.25),
            5: Gf.Vec2f(0.625, 0.25),
            6: Gf.Vec2f(0.625, 0.5),
            7: Gf.Vec2f(0.375, 0.5),
            8: Gf.Vec2f(0.375, 0.5),
            9: Gf.Vec2f(0.625, 0.5),
            10: Gf.Vec2f(0.625, 0.75),
            11: Gf.Vec2f(0.375, 0.75),
            12: Gf.Vec2f(0.375, 0.75),
            13: Gf.Vec2f(0.625, 0.75),
            14: Gf.Vec2f(0.625, 1.0),
            15: Gf.Vec2f(0.375, 1.0),
            16: Gf.Vec2f(0.625, 0.0),
            17: Gf.Vec2f(0.875, 0.0),
            18: Gf.Vec2f(0.875, 0.25),
            19: Gf.Vec2f(0.625, 0.25),
            20: Gf.Vec2f(0.125, 0.0),
            21: Gf.Vec2f(0.375, 0.0),
            22: Gf.Vec2f(0.375, 0.25),
            23: Gf.Vec2f(0.125, 0.25)
        }

        self._AssertUVSet(mayaCubeMesh, 'map1', expectedValues)

if __name__ == '__main__':
    unittest.main(verbosity=2)
