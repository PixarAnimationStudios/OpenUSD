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

from pixar import UsdGeom

# XXX: The try/except here is temporary until we change the Pixar-internal
# package name to match the external package name.
try:
    from pxr import UsdMaya
except ImportError:
    from pixar import UsdMaya


class testUsdExportMesh(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd', quiet=True)

        usdFile = os.path.abspath('Mesh.usda')
        cmds.usdImport(file=usdFile, shadingMode='none')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testImportPoly(self):
        mesh = 'PolyMeshShape'
        self.assertTrue(cmds.objExists(mesh))

        schema = UsdMaya.Adaptor(mesh).GetSchema(UsdGeom.Mesh)
        subdivisionScheme = schema.GetAttribute(
                UsdGeom.Tokens.subdivisionScheme).Get()
        self.assertEqual(subdivisionScheme, UsdGeom.Tokens.none)

        faceVaryingLinearInterpolation = schema.GetAttribute(
                UsdGeom.Tokens.faceVaryingLinearInterpolation).Get()
        self.assertIsNone(faceVaryingLinearInterpolation) # not authored

        interpolateBoundary = schema.GetAttribute(
                UsdGeom.Tokens.interpolateBoundary).Get()
        self.assertEqual(interpolateBoundary, UsdGeom.Tokens.none)

        self.assertTrue(
                cmds.attributeQuery("USD_EmitNormals", node=mesh, exists=True))

    def testImportSubdiv(self):
        mesh = 'SubdivMeshShape'
        self.assertTrue(cmds.objExists(mesh))

        schema = UsdMaya.Adaptor(mesh).GetSchema(UsdGeom.Mesh)
        subdivisionScheme = \
                schema.GetAttribute(UsdGeom.Tokens.subdivisionScheme).Get()
        self.assertIsNone(subdivisionScheme)

        faceVaryingLinearInterpolation = schema.GetAttribute(
                UsdGeom.Tokens.faceVaryingLinearInterpolation).Get()
        self.assertEqual(faceVaryingLinearInterpolation, UsdGeom.Tokens.all)

        interpolateBoundary = schema.GetAttribute(
                UsdGeom.Tokens.interpolateBoundary).Get()
        self.assertEqual(interpolateBoundary, UsdGeom.Tokens.edgeOnly)

        self.assertFalse(
                cmds.attributeQuery("USD_EmitNormals", node=mesh, exists=True))

if __name__ == '__main__':
    unittest.main(verbosity=2)
