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

from maya import cmds
from maya import standalone


class testUserExportedAttributes(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        # The alembic chaser is in the pxrUsd plugin.
        cmds.loadPlugin('pxrUsd', quiet=True)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testExportAttributes(self):
        mayaFilePath = os.path.abspath('AlembicChaser.ma')
        cmds.file(mayaFilePath, open=True, force=True)

        usdFilePath = os.path.abspath('out.usda')

        # Export to USD.
        cmds.usdExport(
            file=usdFilePath,
            chaser=['alembic'],
            chaserArgs=[
                ('alembic', 'attrprefix', 'ABC_,ABC2_=ABC2_,ABC3_=,ABC4_=a:'),
            ])

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        prim = stage.GetPrimAtPath('/Foo/Plane')
        self.assertTrue(prim)

        attr = prim.GetAttribute('userProperties:test')
        self.assertTrue(attr)
        self.assertEqual(attr.Get(), 'success')

        attr2 = prim.GetAttribute('ABC2_test')
        self.assertTrue(attr2)
        self.assertEqual(attr2.Get(), 'success')

        attr3 = prim.GetAttribute('test')
        self.assertTrue(attr3)
        self.assertEqual(attr3.Get(), 'success')

        attr4 = prim.GetAttribute('a:test')
        self.assertTrue(attr4)
        self.assertEqual(attr4.Get(), 'success')

        subd = UsdGeom.Mesh.Get(stage, '/Foo/sphere_subd')
        self.assertEqual(subd.GetSubdivisionSchemeAttr().Get(), 'catmullClark')
        poly = UsdGeom.Mesh.Get(stage, '/Foo/sphere_poly')
        self.assertEqual(poly.GetSubdivisionSchemeAttr().Get(), 'none')

    def testExportedPrimvars(self):
        mayaFilePath = os.path.abspath('AlembicChaserPrimvars.ma')
        cmds.file(mayaFilePath, open=True, force=True)

        usdFilePath = os.path.abspath('out_primvars.usda')

        # Export to USD.
        cmds.usdExport(
            file=usdFilePath,
            chaser=['alembic'],
            chaserArgs=[
                ('alembic', 'attrprefix', 'my'),
                ('alembic', 'primvarprefix', 'my=awesome_'),
            ])

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)

        prim = stage.GetPrimAtPath('/AlembicChaserPrimvars/Geom/CubeTypedAttrs')
        self.assertTrue(prim)

        attr = prim.GetAttribute('userProperties:IntArrayAttr')
        self.assertTrue(attr)
        self.assertEqual(attr.Get(),
            Vt.IntArray([99, 98, 97, 96, 95, 94, 93, 92, 91, 90]))

        imageable = UsdGeom.Imageable(prim)
        self.assertTrue(imageable)

        primvar = imageable.GetPrimvar('awesome_ConstantIntPrimvar')
        self.assertTrue(primvar)
        self.assertEqual(primvar.Get(), 123)
        self.assertEqual(primvar.GetTypeName(), Sdf.ValueTypeNames.Int)
        self.assertEqual(primvar.GetInterpolation(), UsdGeom.Tokens.constant)

        primvar2 = imageable.GetPrimvar('awesome_UniformDoublePrimvar')
        self.assertTrue(primvar2)
        self.assertEqual(primvar2.Get(), 3.140)
        self.assertEqual(primvar2.GetTypeName(), Sdf.ValueTypeNames.Double)
        self.assertEqual(primvar2.GetInterpolation(), UsdGeom.Tokens.uniform)

        primvar3 = imageable.GetPrimvar('awesome_FaceVaryingIntPrimvar')
        self.assertTrue(primvar3)
        self.assertEqual(primvar3.Get(), 999)
        self.assertEqual(primvar3.GetTypeName(), Sdf.ValueTypeNames.Int)
        self.assertEqual(primvar3.GetInterpolation(), UsdGeom.Tokens.faceVarying)

        primvar4 = imageable.GetPrimvar('awesome_FloatArrayPrimvar')
        self.assertTrue(primvar4)
        self.assertEqual(primvar4.Get(),
            Vt.FloatArray([1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8]))
        self.assertEqual(primvar4.GetTypeName(), Sdf.ValueTypeNames.FloatArray)
        self.assertEqual(primvar4.GetInterpolation(), UsdGeom.Tokens.vertex)


if __name__ == '__main__':
    unittest.main(verbosity=2)
