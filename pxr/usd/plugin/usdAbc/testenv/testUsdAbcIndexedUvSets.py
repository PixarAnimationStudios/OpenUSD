#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Usd, UsdGeom
import unittest

class TestUsdAbcIndexedUvSets(unittest.TestCase):
    def test_ExtraUvSetsAreIndexedPrimvars(self):
        # All uv sets should be indexed texCoord2f primvars with int indices
        stage = Usd.Stage.Open('multipleUvSets.abc')
        mesh = UsdGeom.Mesh(stage.GetPrimAtPath('/mesh'))
        self.assertTrue(mesh)
        primvars = UsdGeom.PrimvarsAPI(mesh)
        texCoordArray = Sdf.ValueTypeNames.TexCoord2fArray
        intArray = Sdf.ValueTypeNames.IntArray

        for name in ('st', 'map2'):
            primvar = primvars.GetPrimvar(name)
            self.assertTrue(primvar, name)
            self.assertTrue(primvar.IsIndexed(), name)
            self.assertEqual(primvar.GetTypeName(), texCoordArray, name)
            indices = primvar.GetIndicesAttr()
            self.assertEqual(indices.GetTypeName(), intArray, name)

if __name__ == "__main__":
    unittest.main()
