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

import sys, math
import unittest
from pxr import Gf

class TestGfBBox3d(unittest.TestCase):

    def test_Constructors(self):
        # no arg
        self.assertIsInstance( Gf.BBox3d(), Gf.BBox3d )

        # copy constructor
        self.assertIsInstance( Gf.BBox3d( Gf.BBox3d() ), Gf.BBox3d )

        # construct from range.
        self.assertIsInstance( Gf.BBox3d( Gf.Range3d() ), Gf.BBox3d )

        # construct from range and matrix.
        self.assertIsInstance( 
                Gf.BBox3d( Gf.Range3d(), Gf.Matrix4d() ), Gf.BBox3d )

    def test_Operators(self):
        # equality
        r1 = Gf.Range3d( Gf.Vec3d( 0, 0, 0 ), Gf.Vec3d( 1, 1, 1 ) )
        b1 = Gf.BBox3d( r1 )
        b2 = Gf.BBox3d( r1 )
        self.assertTrue(b1 == b2)

        # inequality
        r1 = Gf.Range3d( Gf.Vec3d( 0, 0, 0 ), Gf.Vec3d( 1, 1, 1 ) )
        r2 = Gf.Range3d( Gf.Vec3d( 1, 1, 1 ), Gf.Vec3d( 2, 2, 2 ) )
        b1 = Gf.BBox3d( r1 )
        b2 = Gf.BBox3d( r2 )
        self.assertTrue(b1 != b2)

        # string (just checks that it's not empty..)
        self.assertTrue(len(str(Gf.BBox3d())))

    def test_MethodsAndProperties(self):
        # set
        m = Gf.Matrix4d()
        r = Gf.Range3d()
        b = Gf.BBox3d()
        self.assertEqual(b.Set( r, m ), Gf.BBox3d( r, m ))

        # matrix prop
        b = Gf.BBox3d()
        b.matrix = Gf.Matrix4d(5)
        self.assertEqual(b.matrix, Gf.Matrix4d(5))

        # box property
        b = Gf.BBox3d()
        self.assertTrue(b.box.IsEmpty())
        b.box = Gf.Range3d( Gf.Vec3d(1,2,3), Gf.Vec3d(4,5,6) )
        self.assertEqual(b.box, Gf.Range3d( Gf.Vec3d(1,2,3), Gf.Vec3d(4,5,6) ))
        self.assertFalse(b.box.IsEmpty())

        # GetInverseGf.Matrix
        b = Gf.BBox3d()
        b.matrix = Gf.Matrix4d(5)
        m = Gf.Matrix4d(5).GetInverse()
        self.assertEqual(b.GetInverseMatrix(), Gf.Matrix4d(5).GetInverse())

        # hasZeroAreaPrimitives
        b = Gf.BBox3d()
        b.hasZeroAreaPrimitives = True
        b.hasZeroAreaPrimitives = not b.hasZeroAreaPrimitives
        self.assertFalse(b.hasZeroAreaPrimitives)

        # GetVolume
        b = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( 0, 0, 0 ), Gf.Vec3d( 2, 2, 2 ) ) )
        self.assertEqual(b.GetVolume(), 8)
        b = Gf.BBox3d(Gf.Range3d(Gf.Vec3d(1,1,1),Gf.Vec3d()))
        self.assertEqual(b.GetVolume(), 0)

        # Transform
        m1 = Gf.Matrix4d(1).SetRotate(Gf.Rotation(Gf.Vec3d.XAxis(), 30))
        m2 = Gf.Matrix4d(1).SetRotate(Gf.Rotation(Gf.Vec3d.YAxis(), 60))
        b = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( 0, 0, 0 ), Gf.Vec3d( 1, 1, 1 ) ), m1 )
        b.Transform(m2)
        self.assertEqual(b.matrix, (m1 * m2))

        # ComputeAlignedRange
        m = Gf.Matrix4d(1).SetRotate(Gf.Rotation(Gf.Vec3d.XAxis(), 30))
        b = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( 0, 0, 0 ), Gf.Vec3d( 1, 1, 1 ) ), m )
        r1 = b.ComputeAlignedRange()
        b = Gf.BBox3d( r1 )
        r2 = b.ComputeAlignedRange()
        self.assertEqual(r1, r2)
        m = Gf.Matrix4d().SetScale(3)
        b = Gf.BBox3d(Gf.Range3d(Gf.Vec3d(-1, -1, -1), Gf.Vec3d(1, 1, 1)), m)
        r = b.ComputeAlignedRange()
        self.assertEqual(r.GetSize(), Gf.Vec3d(6.0, 6.0, 6.0))
        m = Gf.Matrix4d(1).SetRotate(Gf.Rotation(Gf.Vec3d.XAxis(), 30))
        b = Gf.BBox3d(Gf.Range3d(Gf.Vec3d(-1, -1, -1), Gf.Vec3d(1, 1, 1)), m)
        angle = math.pi / 6 # 30 degrees
        width = 2 * (math.cos(angle) + math.sin(angle))
        size = Gf.Vec3d(2.0, width, width)
        r = b.ComputeAlignedRange()
        self.assertFalse((r.GetSize() - size).GetLength() > 0.000001)
        b = Gf.BBox3d()
        r = b.ComputeAlignedRange()
        self.assertTrue(r.IsEmpty())
        
        # static combine method
        m1 = Gf.Matrix4d(1).SetRotate(Gf.Rotation(Gf.Vec3d.XAxis(), 30))
        m2 = Gf.Matrix4d(1).SetRotate(Gf.Rotation(Gf.Vec3d.YAxis(), 60))
        b1 = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( 0, 0, 0 ), Gf.Vec3d( 1, 1, 1 ) ), m1 )
        b2 = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( 1, 1, 1 ), Gf.Vec3d( 2, 2, 2 ) ), m2 )
        b3 = Gf.BBox3d.Combine( b1, b2 )
        b4 = Gf.BBox3d.Combine( b2, b1 )
        self.assertEqual(b3, b4)

        # centroid
        b1 = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( 0, 0, 0 ), Gf.Vec3d( 1, 1, 1 ) ) )

        m2 = Gf.Matrix4d(1).SetRotate(Gf.Rotation(Gf.Vec3d.YAxis(), 60))
        b2 = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( -1, -1, -1 ), Gf.Vec3d( 1, 1, 1 ) ), m2)
        self.assertEqual(b1.ComputeCentroid(), Gf.Vec3d( .5, .5, .5 ))
        self.assertEqual(b2.ComputeCentroid(), Gf.Vec3d( 0, 0, 0 ))
        
        b3 = Gf.BBox3d()
        self.assertEqual(b3.ComputeCentroid(), Gf.Vec3d( 0, 0, 0 ))

        # other cases to hit code coverage.
        m1 = Gf.Matrix4d(Gf.Vec4d(1,0,0,0))
        m2 = Gf.Matrix4d(Gf.Vec4d(0,1,0,0))
        b1 = Gf.BBox3d(Gf.Range3d(Gf.Vec3d(1,1,1), Gf.Vec3d()), m1)
        b2 = Gf.BBox3d(Gf.Range3d(Gf.Vec3d(1,1,1), Gf.Vec3d(2,2,2)), m2)
        b3 = Gf.BBox3d.Combine(b1, b2)
        self.assertEqual(b3, b2)
        b3 = Gf.BBox3d.Combine(b2, b1)
        self.assertEqual(b3, b2)

        m1 = Gf.Matrix4d(Gf.Vec4d(1,0,0,0))
        m2 = Gf.Matrix4d(Gf.Vec4d(0,1,0,0))
        m3 = Gf.Matrix4d(1).SetRotate(Gf.Rotation(Gf.Vec3d.YAxis(), 60))
        b1 = Gf.BBox3d(Gf.Range3d(Gf.Vec3d(0,0,0), Gf.Vec3d(0,1,1)), m1)
        b2 = Gf.BBox3d(Gf.Range3d(Gf.Vec3d(0,0,0), Gf.Vec3d(1,1,0)), m2)
        b3 = Gf.BBox3d(Gf.Range3d(Gf.Vec3d(1,1,1), Gf.Vec3d(2,2,2)), m3)
        b4 = Gf.BBox3d.Combine(b1,b2)
        b4 = Gf.BBox3d.Combine(b1,b2)
        b4 = Gf.BBox3d.Combine(b2,b3)
        b4 = Gf.BBox3d.Combine(b3,b2)

        self.assertEqual(b4, eval(repr(b4)))

if __name__ == '__main__':
    unittest.main()

