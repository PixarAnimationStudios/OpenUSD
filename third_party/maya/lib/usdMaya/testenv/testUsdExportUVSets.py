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
from pxr import Gf
from pxr import Sdf
from pxr import Usd
from pxr import UsdGeom
from pxr import Vt
from pxr import Tf

from maya import cmds
from maya import standalone

try:
    from pxr import UsdMaya
except ImportError:
    from pixar import UsdMaya



class testUsdExportUVSets(unittest.TestCase):

    def _AssertUVPrimvar(self, primvar,
            expectedValues=None, expectedInterpolation=None,
            expectedIndices=None, expectedUnauthoredValuesIndex=None):
        self.assertTrue(primvar)

        if isinstance(expectedIndices, list):
            expectedIndices = Vt.IntArray(expectedIndices)
        if expectedUnauthoredValuesIndex is None:
            expectedUnauthoredValuesIndex = -1

        if not UsdMaya.WriteUtil.WriteUVAsFloat2():
            self.assertEqual(primvar.GetTypeName(), Sdf.ValueTypeNames.TexCoord2fArray)
        else: 
            self.assertEqual(primvar.GetTypeName(), Sdf.ValueTypeNames.Float2Array)

        for idx in range(len(primvar.Get())):
            self.assertEqual(primvar.Get()[idx], expectedValues[idx])
        self.assertEqual(primvar.GetIndices(), expectedIndices)
        self.assertEqual(primvar.GetUnauthoredValuesIndex(),
            expectedUnauthoredValuesIndex)
        self.assertEqual(primvar.GetInterpolation(), expectedInterpolation)


    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

        if not UsdMaya.WriteUtil.WriteUVAsFloat2():
            cmds.file(os.path.abspath('UsdExportUVSetsTest.ma'), open=True,
                       force=True)
        else:
            cmds.file(os.path.abspath('UsdExportUVSetsTest_Float.ma'), open=True,
                       force=True)

        # Make some live edits to the box with weird UVs for the
        # testExportUvVersusUvIndexFromIterator test.
        cmds.select("box.map[0:299]", r=True)
        cmds.polyEditUV(u=1.0, v=1.0)

        usdFilePath = os.path.abspath('UsdExportUVSetsTest.usda')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            shadingMode='none',
            exportColorSets=False,
            exportDisplayColor=False,
            exportUVs=True)

        cls._stage = Usd.Stage.Open(usdFilePath)

    def testStageOpens(self):
        self.assertTrue(self._stage)

    def _GetCubeUsdMesh(self, cubeName):
        cubePrimPath = '/UsdExportUVSetsTest/Geom/CubeMeshes/%s' % cubeName
        cubePrim = self._stage.GetPrimAtPath(cubePrimPath)
        self.assertTrue(cubePrim)

        usdMesh = UsdGeom.Mesh(cubePrim)
        self.assertTrue(usdMesh)

        return usdMesh

    def testExportEmptyDefaultUVSet(self):
        """
        Tests that a cube mesh with an empty default UV set (named 'map1' in
        Maya) does NOT get exported as 'st'.
        """
        usdCubeMesh = self._GetCubeUsdMesh('EmptyDefaultUVSetCube')

        primvar = usdCubeMesh.GetPrimvar('st')
        self.assertFalse(primvar)

    def testExportDefaultUVSet(self):
        """
        Tests that a cube mesh with the default values for the default UV set
        (named 'map1' in Maya) gets exported correctly as 'st'.
        """
        usdCubeMesh = self._GetCubeUsdMesh('DefaultUVSetCube')

        # These are the default UV values and indices that are exported for a
        # regular Maya polycube. If you just created a new cube and then
        # exported it to USD, these are the values and indices you would see
        # for the default UV set 'map1'. The data here has already been
        # merged/compressed.
        expectedValues = [
            Gf.Vec2f(0.375, 0),
            Gf.Vec2f(0.625, 0),
            Gf.Vec2f(0.625, 0.25),
            Gf.Vec2f(0.375, 0.25),
            Gf.Vec2f(0.625, 0.5),
            Gf.Vec2f(0.375, 0.5),
            Gf.Vec2f(0.625, 0.75),
            Gf.Vec2f(0.375, 0.75),
            Gf.Vec2f(0.625, 1),
            Gf.Vec2f(0.375, 1),
            Gf.Vec2f(0.875, 0),
            Gf.Vec2f(0.875, 0.25),
            Gf.Vec2f(0.125, 0),
            Gf.Vec2f(0.125, 0.25)
        ]
        expectedIndices = [
            0, 1, 2, 3,
            3, 2, 4, 5,
            5, 4, 6, 7,
            7, 6, 8, 9,
            1, 10, 11, 2,
            12, 0, 3, 13]

        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = usdCubeMesh.GetPrimvar('st')
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

    def testExportOneMissingFaceUVSet(self):
        """
        Tests that a cube mesh with values for all but one face in the default
        UV set (named 'map1' in Maya) gets exported correctly as 'st'.
        """
        usdCubeMesh = self._GetCubeUsdMesh('OneMissingFaceCube')

        expectedValues = [
            Gf.Vec2f(0.0, 0.0),
            Gf.Vec2f(0.375, 0),
            Gf.Vec2f(0.625, 0),
            Gf.Vec2f(0.625, 0.25),
            Gf.Vec2f(0.375, 0.25),
            Gf.Vec2f(0.625, 0.5),
            Gf.Vec2f(0.375, 0.5),
            Gf.Vec2f(0.375, 0.75),
            Gf.Vec2f(0.625, 0.75),
            Gf.Vec2f(0.625, 1),
            Gf.Vec2f(0.375, 1),
            Gf.Vec2f(0.875, 0),
            Gf.Vec2f(0.875, 0.25),
            Gf.Vec2f(0.125, 0),
            Gf.Vec2f(0.125, 0.25)
        ]
        expectedIndices = [
            1, 2, 3, 4,
            4, 3, 5, 6,
            0, 0, 0, 0,
            7, 8, 9, 10,
            2, 11, 12, 3,
            13, 1, 4, 14
        ]
        expectedUnauthoredValuesIndex = 0

        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = usdCubeMesh.GetPrimvar('st')
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices,
            expectedUnauthoredValuesIndex=expectedUnauthoredValuesIndex)

    def testExportOneAssignedFaceUVSet(self):
        """
        Tests that a cube mesh with values for only one face in the default
        UV set (named 'map1' in Maya) gets exported correctly as 'st'.
        """
        usdCubeMesh = self._GetCubeUsdMesh('OneAssignedFaceCube')

        expectedValues = [
            Gf.Vec2f(0.0, 0.0),
            Gf.Vec2f(0.375, 0.5),
            Gf.Vec2f(0.625, 0.5),
            Gf.Vec2f(0.625, 0.75),
            Gf.Vec2f(0.375, 0.75)
        ]
        expectedIndices = [
            0, 0, 0, 0,
            0, 0, 0, 0,
            1, 2, 3, 4,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0
        ]
        expectedUnauthoredValuesIndex = 0

        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = usdCubeMesh.GetPrimvar('st')
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices,
            expectedUnauthoredValuesIndex=expectedUnauthoredValuesIndex)

    def testExportCompressibleUVSets(self):
        """
        Tests that UV sets on a cube mesh that can be compressed to constant,
        uniform and vertex interpolations get exported correctly.

        Note that the actual values here don't really make sense as UV sets.
        """
        usdCubeMesh = self._GetCubeUsdMesh('CompressibleUVSetsCube')

        uvSetName = 'ConstantInterpSet'
        expectedValues = [
            Gf.Vec2f(0.25, 0.25)
        ]
        expectedIndices = [0]
        expectedInterpolation = UsdGeom.Tokens.constant

        primvar = usdCubeMesh.GetPrimvar(uvSetName)
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

        uvSetName = 'UniformInterpSet'
        expectedValues = [
            Gf.Vec2f(0.0, 0.0),
            Gf.Vec2f(0.1, 0.1),
            Gf.Vec2f(0.2, 0.2),
            Gf.Vec2f(0.3, 0.3),
            Gf.Vec2f(0.4, 0.4),
            Gf.Vec2f(0.5, 0.5)
        ]
        expectedIndices = [0, 1, 2, 3, 4, 5]
        expectedInterpolation = UsdGeom.Tokens.uniform

        primvar = usdCubeMesh.GetPrimvar(uvSetName)
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

        # The values here end up in a somewhat odd order because of how
        # MItMeshFaceVertex visits vertices.
        uvSetName = 'VertexInterpSet'
        expectedValues = [
            Gf.Vec2f(0.0, 0.0),
            Gf.Vec2f(0.1, 0.1),
            Gf.Vec2f(0.3, 0.3),
            Gf.Vec2f(0.2, 0.2),
            Gf.Vec2f(0.5, 0.5),
            Gf.Vec2f(0.4, 0.4),
            Gf.Vec2f(0.7, 0.7),
            Gf.Vec2f(0.6, 0.6)
        ]
        expectedIndices = [0, 1, 3, 2, 5, 4, 7, 6]
        expectedInterpolation = UsdGeom.Tokens.vertex

        primvar = usdCubeMesh.GetPrimvar(uvSetName)
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

    def testExportSharedFacesUVSets(self):
        """
        Tests that UV sets on a cube mesh that use the same UV ranges for
        multiple faces get exported correctly.
        """
        usdCubeMesh = self._GetCubeUsdMesh('SharedFacesCube')

        # All six faces share the same range 0.0-1.0.
        uvSetName = 'AllFacesSharedSet'
        expectedValues = [
            Gf.Vec2f(0.0, 0.0),
            Gf.Vec2f(1.0, 0.0),
            Gf.Vec2f(1.0, 1.0),
            Gf.Vec2f(0.0, 1.0)
        ]
        expectedIndices = [0, 1, 2, 3] * 6
        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = usdCubeMesh.GetPrimvar(uvSetName)
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

        # The faces alternate between ranges 0.0-0.5 and 0.5-1.0.
        uvSetName = 'PairedFacesSet'
        expectedValues = [
            Gf.Vec2f(0.0, 0.0),
            Gf.Vec2f(0.5, 0.0),
            Gf.Vec2f(0.5, 0.5),
            Gf.Vec2f(0.0, 0.5),
            Gf.Vec2f(1.0, 0.5),
            Gf.Vec2f(1.0, 1.0),
            Gf.Vec2f(0.5, 1.0)
        ]
        expectedIndices = [
            0, 1, 2, 3,
            2, 4, 5, 6] * 3
        expectedInterpolation = UsdGeom.Tokens.faceVarying

        primvar = usdCubeMesh.GetPrimvar(uvSetName)
        self._AssertUVPrimvar(primvar,
            expectedValues=expectedValues,
            expectedInterpolation=expectedInterpolation,
            expectedIndices=expectedIndices)

    def testExportUvVersusUvIndexFromIterator(self):
        """
        Tests that UVs export properly on a mesh where the UV returned by
        MItMeshFaceVertex::getUV is known to be different from that
        returned by MItMeshFaceVertex::getUVIndex and indexed into the UV array.
        (The latter should return the desired result from the outMesh and is the
        method currently used by usdMaya.)
        """
        brokenBoxMesh = UsdGeom.Mesh(self._stage.GetPrimAtPath(
                "/UsdExportUVSetsTest/Geom/BrokenUVs/box"))
        stPrimvar = brokenBoxMesh.GetPrimvar("st").ComputeFlattened()
        self.assertEqual(stPrimvar[0], Gf.Vec2f(1.0, 1.0))

if __name__ == '__main__':
    unittest.main(verbosity=2)
