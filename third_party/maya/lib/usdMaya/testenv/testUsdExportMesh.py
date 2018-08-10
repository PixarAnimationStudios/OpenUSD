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

from pxr import Gf, Sdf, Usd, UsdGeom, UsdSkel, Vt


class testUsdExportMesh(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.file(os.path.abspath('UsdExportMeshTest.ma'), open=True,
            force=True)

        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def _AssertVec3fArrayAlmostEqual(self, arr1, arr2):
        self.assertEqual(len(arr1), len(arr2))
        for i in xrange(len(arr1)):
            for j in xrange(3):
                self.assertAlmostEqual(arr1[i][j], arr2[i][j], places=3)

    def testExportAsCatmullClark(self):
        usdFile = os.path.abspath('UsdExportMesh_catmullClark.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', defaultMeshScheme='catmullClark')

        stage = Usd.Stage.Open(usdFile)

        # This has subdivision scheme 'none',
        # face-varying linear interpolation 'all'.
        # The interpolation attribute doesn't matter since it's not a subdiv,
        # so we don't write it out even though it's set on the node.
        m = UsdGeom.Mesh.Get(stage, '/UsdExportMeshTest/poly')
        self.assertEqual(m.GetSubdivisionSchemeAttr().Get(), UsdGeom.Tokens.none)
        self.assertTrue(m.GetSubdivisionSchemeAttr().IsAuthored())
        self.assertFalse(m.GetInterpolateBoundaryAttr().IsAuthored())
        self.assertFalse(m.GetFaceVaryingLinearInterpolationAttr().IsAuthored())
        self.assertTrue(len(m.GetNormalsAttr().Get()) > 0)

        m = UsdGeom.Mesh.Get(stage, '/UsdExportMeshTest/polyNoNormals')
        self.assertEqual(m.GetSubdivisionSchemeAttr().Get(), UsdGeom.Tokens.none)
        self.assertTrue(m.GetSubdivisionSchemeAttr().IsAuthored())
        self.assertFalse(m.GetInterpolateBoundaryAttr().IsAuthored())
        self.assertFalse(m.GetFaceVaryingLinearInterpolationAttr().IsAuthored())
        self.assertTrue(not m.GetNormalsAttr().Get())

        # We author subdivision scheme sparsely, so if the subd scheme is
        # catmullClark or unspecified (falls back to
        # defaultMeshScheme=catmullClark), then it shouldn't be authored.
        # Note that this code is interesting because both
        # USD_interpolateBoundary and USD_ATTR_interpolateBoundary are set;
        # the latter should win when both are present.
        m = UsdGeom.Mesh.Get(stage, '/UsdExportMeshTest/subdiv')
        self.assertEqual(m.GetSubdivisionSchemeAttr().Get(), UsdGeom.Tokens.catmullClark)
        self.assertFalse(m.GetSubdivisionSchemeAttr().IsAuthored())
        self.assertEqual(m.GetInterpolateBoundaryAttr().Get(), UsdGeom.Tokens.edgeAndCorner)
        self.assertTrue(m.GetInterpolateBoundaryAttr().IsAuthored())
        self.assertEqual(m.GetFaceVaryingLinearInterpolationAttr().Get(), UsdGeom.Tokens.cornersPlus1)
        self.assertTrue(m.GetFaceVaryingLinearInterpolationAttr().IsAuthored())
        self.assertTrue(not m.GetNormalsAttr().Get())

        m = UsdGeom.Mesh.Get(stage, '/UsdExportMeshTest/unspecified')
        self.assertEqual(m.GetSubdivisionSchemeAttr().Get(), UsdGeom.Tokens.catmullClark)
        self.assertFalse(m.GetSubdivisionSchemeAttr().IsAuthored())
        self.assertFalse(m.GetInterpolateBoundaryAttr().IsAuthored())
        self.assertFalse(m.GetFaceVaryingLinearInterpolationAttr().IsAuthored())
        self.assertTrue(not m.GetNormalsAttr().Get())

    def testExportAsPoly(self):
        usdFile = os.path.abspath('UsdExportMesh_none.usda')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFile,
            shadingMode='none', defaultMeshScheme='none')

        stage = Usd.Stage.Open(usdFile)

        m = UsdGeom.Mesh.Get(stage, '/UsdExportMeshTest/unspecified')
        self.assertEqual(m.GetSubdivisionSchemeAttr().Get(), UsdGeom.Tokens.none)
        self.assertTrue(m.GetSubdivisionSchemeAttr().IsAuthored())
        self.assertFalse(m.GetInterpolateBoundaryAttr().IsAuthored())
        self.assertFalse(m.GetFaceVaryingLinearInterpolationAttr().IsAuthored())
        self.assertTrue(len(m.GetNormalsAttr().Get()) > 0)

        # Explicit catmullClark meshes should still export as catmullClark.
        m = UsdGeom.Mesh.Get(stage, '/UsdExportMeshTest/subdiv')
        self.assertEqual(m.GetSubdivisionSchemeAttr().Get(), UsdGeom.Tokens.catmullClark)
        self.assertFalse(m.GetSubdivisionSchemeAttr().IsAuthored())
        self.assertEqual(m.GetInterpolateBoundaryAttr().Get(), UsdGeom.Tokens.edgeAndCorner)
        self.assertTrue(m.GetInterpolateBoundaryAttr().IsAuthored())
        self.assertEqual(m.GetFaceVaryingLinearInterpolationAttr().Get(), UsdGeom.Tokens.cornersPlus1)
        self.assertTrue(m.GetFaceVaryingLinearInterpolationAttr().IsAuthored())
        self.assertTrue(not m.GetNormalsAttr().Get())

        # XXX: For some reason, when the mesh export used the getNormal()
        # method on MItMeshFaceVertex, we would sometimes get incorrect normal
        # values. Instead, we had to get all of the normals off of the MFnMesh
        # and then use the iterator's normalId() method to do a lookup into the
        # normals.
        # This test ensures that we're getting correct normals. The mesh should
        # only have normals in the x or z direction.

        m = UsdGeom.Mesh.Get(stage, '/UsdExportMeshTest/TestNormalsMesh')
        normals = m.GetNormalsAttr().Get()
        self.assertTrue(normals)
        for n in normals:
            # we don't expect the normals to be pointed in the y-axis at all.
            self.assertAlmostEqual(n[1], 0.0, delta=1e-4)

            # make sure the other 2 values aren't both 0.
            self.assertNotAlmostEqual(abs(n[0]) + abs(n[2]), 0.0, delta=1e-4)


if __name__ == '__main__':
    unittest.main(verbosity=2)
