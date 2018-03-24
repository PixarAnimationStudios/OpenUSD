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

from pxr import Sdf
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt
from pxr import Gf

from maya import cmds
from maya import standalone


class testUsdExportDisplayColor(unittest.TestCase):

    def _AssertPrimvar(self, primvar, expectedTypeName=None,
            expectedValues=None, expectedInterpolation=None,
            expectedIndices=None, expectedUnauthoredValuesIndex=None):
        self.assertTrue(primvar)

        if expectedInterpolation is None:
            expectedInterpolation = UsdGeom.Tokens.constant
        if expectedIndices is None:
            expectedIndices = Vt.IntArray()
        elif isinstance(expectedIndices, list):
            expectedIndices = Vt.IntArray(expectedIndices)
        if expectedUnauthoredValuesIndex is None:
            expectedUnauthoredValuesIndex = -1

        # This should work for undefined primvars.
        self.assertEqual(primvar.GetIndices(), expectedIndices)
        self.assertEqual(primvar.GetUnauthoredValuesIndex(),
            expectedUnauthoredValuesIndex)

        if expectedTypeName is None:
            self.assertFalse(primvar.IsDefined())
            # No further testing for primvars that we expect not to exist.
            return

        self.assertTrue(primvar.IsDefined())
        self.assertEqual(primvar.GetTypeName(), expectedTypeName)
        self.assertEqual(primvar.GetInterpolation(), expectedInterpolation)

        if expectedValues is None:
            self.assertFalse(primvar.GetAttr().HasAuthoredValueOpinion())
            self.assertEqual(primvar.Get(), None)
        else:
            for idx in range(len(primvar.Get())):
                val1 = primvar.Get()[idx]
                val2 = expectedValues[idx]
                if isinstance(val1, Gf.Vec3f):
                    self.assertEqual(val1, val2)  # both are 3-vectors
                    continue
                self.assertAlmostEqual(val1, val2, places=5)  # default==7

    def _AssertMeshDisplayColorAndOpacity(self, mesh, expectedColors=None,
            expectedOpacities=None, expectedInterpolation=None,
            expectedIndices=None, expectedUnauthoredValuesIndex=None):
        displayColorPrimvar = mesh.GetDisplayColorPrimvar()
        self._AssertPrimvar(
            displayColorPrimvar, Sdf.ValueTypeNames.Color3fArray,
            expectedColors, expectedInterpolation,
            expectedIndices, expectedUnauthoredValuesIndex)

        displayOpacityPrimvar = mesh.GetDisplayOpacityPrimvar()
        self._AssertPrimvar(
            displayOpacityPrimvar, Sdf.ValueTypeNames.FloatArray,
            expectedOpacities, expectedInterpolation,
            expectedIndices, expectedUnauthoredValuesIndex)


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.file(os.path.abspath('UsdExportDisplayColorTest.ma'),
                       open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('UsdExportDisplayColorTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            exportDisplayColor=True)

        cls._stage = Usd.Stage.Open(usdFilePath)

    def testStageOpens(self):
        self.assertTrue(self._stage)

    def _GetCubeMesh(self, cubeName):
        cubePrimPath = '/UsdExportDisplayColorTest/Geom/CubeMeshes/%s' % cubeName
        cubePrim = self._stage.GetPrimAtPath(cubePrimPath)
        self.assertTrue(cubePrim)

        usdMesh = UsdGeom.Mesh(cubePrim)
        self.assertTrue(usdMesh)

        return usdMesh

    def testExportPolyCubeObjectLevelAssignment(self):
        """
        Tests exporting a cube where the entire object is assigned one shader.
        """
        cubeMesh = self._GetCubeMesh('ObjectLevelCube')

        expectedColors = Vt.Vec3fArray([(1.0, 1.0, 0.0)])
        expectedOpacities = Vt.FloatArray([0.4])
        self._AssertMeshDisplayColorAndOpacity(cubeMesh, expectedColors,
            expectedOpacities, UsdGeom.Tokens.constant)

    def testExportPolyCubeUniquePerFace(self):
        """
        Tests exporting a cube where each face is assigned a unique shader.
        """
        cubeMesh = self._GetCubeMesh('UniquePerFaceCube')

        expectedColors = Vt.Vec3fArray([
            (0.0, 1.0, 1.0),
            (1.0, 0.0, 0.0),
            (0.0, 1.0, 0.0),
            (0.0, 0.0, 1.0),
            (1.0, 1.0, 0.0),
            (1.0, 0.0, 1.0)])
        expectedOpacities = Vt.FloatArray([
            0.1,
            0.85,
            0.7,
            0.55,
            0.4,
            0.25])
        expectedIndices = Vt.IntArray([5, 3, 0, 1, 2, 4])
        self._AssertMeshDisplayColorAndOpacity(cubeMesh, expectedColors,
            expectedOpacities, UsdGeom.Tokens.uniform, expectedIndices)

    def testExportPolyCubeSharedFaces(self):
        """
        Tests exporting a cube where each of three pairs of faces have the
        same shader assigned.
        """
        cubeMesh = self._GetCubeMesh('SharedFacesCube')

        expectedColors = Vt.Vec3fArray([
            (1.0, 0.0, 1.0),
            (1.0, 0.0, 0.0),
            (0.0, 0.0, 1.0)])
        expectedOpacities = Vt.FloatArray([
            0.25,
            0.85,
            0.55])
        expectedIndices = Vt.IntArray([0, 2, 0, 1, 2, 1])
        self._AssertMeshDisplayColorAndOpacity(cubeMesh, expectedColors,
            expectedOpacities, UsdGeom.Tokens.uniform, expectedIndices)

    def testExportPolyCubeUnassigned(self):
        """
        Tests exporting a cube that has no shader assigned at all.
        """
        cubeMesh = self._GetCubeMesh('UnassignedCube')

        self._AssertMeshDisplayColorAndOpacity(cubeMesh)

        # If whole mesh is assigned to a "empty shading group", then it should
        # be equiv to "UnassignedCube" case above.  Currently, this means it
        # gets exported with no displayColor/Opacity authored (which is
        # unfortunately different from you have per-face assignments to the same
        # empty shading group).
        cubeMesh = self._GetCubeMesh('ShadingGroupNoShader')
        self._AssertMeshDisplayColorAndOpacity(cubeMesh)

    def testExportPolyCubeOneAssignedFace(self):
        """
        Tests exporting a cube that has no object-level shader assigned and
        only one face that has an assigned shader.
        """
        cubeMesh = self._GetCubeMesh('OneFaceCube')

        expectedColors = Vt.Vec3fArray([
            (0.0, 1.0, 0.0),
            (0.5, 0.5, 0.5)])
        expectedOpacities = Vt.FloatArray([
            0.7,
            0.0])

        expectedIndices = Vt.IntArray([0, 1, 1, 1, 1, 1])
        expectedUnauthoredValuesIndex = 1
        self._AssertMeshDisplayColorAndOpacity(cubeMesh, expectedColors,
            expectedOpacities, UsdGeom.Tokens.uniform,
            expectedIndices, expectedUnauthoredValuesIndex)

    def testExportMeshInEmptyShadingGroup(self):
        """
        Tests exporting a cube where either all of it or faces are assigned to a
        shadingGroup that has it's surface material deleted.
        """

        # If all faces are in the empty shading group, all faces get opacity=0.
        allFaces = self._GetCubeMesh('ShadingGroupNoShaderAllFaces')
        expectedColors = Vt.Vec3fArray([
            (0.5, 0.5, 0.5)])
        expectedOpacities = Vt.FloatArray([
            0.0])
        expectedIndices = Vt.IntArray([0, 0, 0, 0, 0, 0])
        expectedUnauthoredValuesIndex = 0
        self._AssertMeshDisplayColorAndOpacity(allFaces, expectedColors,
            expectedOpacities, UsdGeom.Tokens.uniform,
            expectedIndices, expectedUnauthoredValuesIndex)

        # If some are "empty shading group", but not all, then empty ones get
        # opacity 0.
        oneFace = self._GetCubeMesh('ShadingGroupNoShaderOneFace')
        expectedColors = Vt.Vec3fArray([
            (1, 0, 0),  
            (0, 0, 1), 
            (0.21763764, 0.21763764, 0.21763764),  # initialShadingGroup
            (0.5, 0.5, 0.5)])
        expectedOpacities = Vt.FloatArray([1, 1, 1, 0])
        expectedIndices = [3, 1, 2, 2, 0, 2]
        expectedUnauthoredValuesIndex = 3
        self._AssertMeshDisplayColorAndOpacity(oneFace, expectedColors,
            expectedOpacities, UsdGeom.Tokens.uniform,
            expectedIndices, expectedUnauthoredValuesIndex)

        # note, there is one test case for having a whole mesh assigned to an
        # "empty shading group" in testExportPolyCubeUnassigned.


if __name__ == '__main__':
    unittest.main(verbosity=2)
