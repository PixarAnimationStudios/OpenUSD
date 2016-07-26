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
from pxr import Gf
import unittest

class TestGfGamma(unittest.TestCase):

    def test_DifferentVecs(self):
        '''Test gamma converting vecs of different types and sizes'''
        self.assertEqual(Gf.ApplyGamma(Gf.Vec3f(1,2,3),2.0), Gf.Vec3f(1,4,9))
        self.assertEqual(Gf.ApplyGamma(Gf.Vec3d(1,2,3),2.0), Gf.Vec3d(1,4,9))
        self.assertEqual(Gf.ApplyGamma(Gf.Vec4f(1,2,3,4),2.0), Gf.Vec4f(1,4,9,4))
        self.assertEqual(Gf.ApplyGamma(Gf.Vec4d(1,2,3,4),2.0), Gf.Vec4d(1,4,9,4))

    def test_DisplayGamma(self):
        # Check that the display gamma functions work too
        self.assertTrue(Gf.ApplyGamma(Gf.Vec3f(.5,.5,.5),2.2) ==
                Gf.ConvertDisplayToLinear(Gf.Vec3f(.5,.5,.5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec3d(.5,.5,.5),2.2) ==
                Gf.ConvertDisplayToLinear(Gf.Vec3d(.5,.5,.5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4f(.5,.5,.5,.8),2.2) ==
                Gf.ConvertDisplayToLinear(Gf.Vec4f(.5,.5,.5,.8)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4d(.5,.5,.5,.8),2.2) ==
                Gf.ConvertDisplayToLinear(Gf.Vec4d(.5,.5,.5,.8)))

        self.assertTrue(Gf.ApplyGamma(Gf.Vec3f(.5,.5,.5),1.0/2.2) ==
                Gf.ConvertLinearToDisplay(Gf.Vec3f(.5,.5,.5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec3d(.5,.5,.5),1.0/2.2) ==
                Gf.ConvertLinearToDisplay(Gf.Vec3d(.5,.5,.5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4f(.5,.5,.5,.8),1.0/2.2) ==
                Gf.ConvertLinearToDisplay(Gf.Vec4f(.5,.5,.5,.8)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4d(.5,.5,.5,.8),1.0/2.2) ==
                Gf.ConvertLinearToDisplay(Gf.Vec4d(.5,.5,.5,.8)))

if __name__ == '__main__':
    unittest.main()
