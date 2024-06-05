#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd, UsdSkel, Gf, Vt
import unittest


class TestUsdSkelBlendShape(unittest.TestCase):

    def test_BlendShape(self):

        stage = Usd.Stage.CreateInMemory()

        shape = UsdSkel.BlendShape.Define(stage, '/shape')

        self.assertFalse(shape.GetInbetweens())
        self.assertFalse(shape.GetAuthoredInbetweens())


    def test_BlendShapeWithInbetweens(self):

        stage = Usd.Stage.CreateInMemory()
        
        shape = UsdSkel.BlendShape.Define(stage, '/shape')
        self.assertFalse(shape.GetAuthoredInbetweens())

        subShape = shape.CreateInbetween('subShape')
        self.assertTrue(subShape)
        self.assertEqual(subShape, shape.GetInbetween('subShape'))

        self.assertEqual(shape.GetInbetweens(), [subShape])

        self.assertFalse(subShape.HasAuthoredWeight())
        self.assertTrue(subShape.SetWeight(0.5))
        self.assertEqual(subShape.GetWeight(), 0.5)
        self.assertTrue(subShape.HasAuthoredWeight())

        offsets = Vt.Vec3fArray([Gf.Vec3f(1,1,1)])
        self.assertTrue(subShape.SetOffsets(offsets))
        self.assertEqual(subShape.GetOffsets(), offsets)

        self.assertFalse(subShape.GetNormalOffsetsAttr())
        self.assertFalse(subShape.GetNormalOffsets())
        
        normalOffsets = Vt.Vec3fArray([Gf.Vec3f(1,2,3)])
        self.assertTrue(subShape.SetNormalOffsets(normalOffsets))
        self.assertEqual(subShape.GetNormalOffsets(), normalOffsets)

        self.assertEqual(shape.GetInbetweens(), [subShape])


if __name__ == "__main__":
    unittest.main()
