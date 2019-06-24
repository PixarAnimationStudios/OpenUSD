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
