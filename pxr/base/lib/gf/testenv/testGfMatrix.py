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
from pxr import Gf, Tf

try:
    import numpy
    hasNumpy = True
except ImportError:
    print 'numpy not available, skipping buffer protocol tests'
    hasNumpy = False

def makeValue( Value, vals ):
    if Value == float:
        return Value(vals[0])
    else:
        v = Value()
        for i in range(v.dimension):
            v[i] = vals[i]
    return v

class TestGfMatrix(unittest.TestCase):

    def test_Basic(self):
        Matrices = [(Gf.Matrix2d, Gf.Vec2d),
                    (Gf.Matrix2f, Gf.Vec2f),
                    (Gf.Matrix3d, Gf.Vec3d),
                    (Gf.Matrix3f, Gf.Vec3f),
                    (Gf.Matrix4d, Gf.Vec4d),
                    (Gf.Matrix4f, Gf.Vec4f)]

        for (Matrix, Vec) in Matrices:
            # constructors
            self.assertIsInstance(Matrix(), Matrix)
            self.assertIsInstance(Matrix(1), Matrix)
            self.assertIsInstance(Matrix(Vec()), Matrix)

            # python default constructor produces identity.
            self.assertEqual(Matrix(), Matrix(1))

            if hasNumpy:
                # Verify population of numpy arrays.
                emptyNumpyArray = numpy.empty((1,
                                            Matrix.dimension[0],
                                            Matrix.dimension[1]),
                                            dtype='float32')
                emptyNumpyArray[0] = Matrix(1)

                if Matrix.dimension == (2, 2):
                    self.assertIsInstance(Matrix(1,2,3,4), Matrix)
                    self.assertEqual(Matrix().Set(1,2,3,4), Matrix(1,2,3,4))
                    array = numpy.array(Matrix(1,2,3,4))
                    self.assertEqual(array.shape, (2,2))
                    # XXX: Currently cannot round-trip Gf.Matrix*f through
                    #      numpy.array
                    if Matrix != Gf.Matrix2f:
                        self.assertEqual(Matrix(array), Matrix(1,2,3,4))
                elif Matrix.dimension == (3, 3):
                    self.assertIsInstance(Matrix(1,2,3,4,5,6,7,8,9), Matrix)
                    self.assertEqual(Matrix().Set(1,2,3,4,5,6,7,8,9),
                        Matrix(1,2,3,4,5,6,7,8,9))
                    array = numpy.array(Matrix(1,2,3,4,5,6,7,8,9))
                    self.assertEqual(array.shape, (3,3))
                    # XXX: Currently cannot round-trip Gf.Matrix*f through
                    #      numpy.array
                    if Matrix != Gf.Matrix3f:
                        self.assertEqual(Matrix(array), Matrix(1,2,3,4,5,6,7,8,9))
                elif Matrix.dimension == (4, 4):
                    self.assertIsInstance(Matrix(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16),
                                    Matrix)
                    self.assertEqual(Matrix().Set(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16), 
                        Matrix(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16))
                    array = numpy.array(Matrix(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16))
                    self.assertEqual(array.shape, (4,4))
                    # XXX: Currently cannot round-trip Gf.Matrix*f through
                    #      numpy.array
                    if Matrix != Gf.Matrix4f:
                        self.assertEqual(Matrix(array), 
                            Matrix(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16))
                else:
                    self.fail()


            self.assertEqual(Matrix().SetIdentity(), Matrix(1))

            self.assertEqual(Matrix().SetZero(), Matrix(0))

            self.assertEqual(Matrix().SetDiagonal(0), Matrix().SetZero())
            self.assertEqual(Matrix().SetDiagonal(1), Matrix().SetIdentity())

    def test_Comparisons(self):
        Matrices = [(Gf.Matrix2d, Gf.Matrix2f),
                    (Gf.Matrix3d, Gf.Matrix3f),
                    (Gf.Matrix4d, Gf.Matrix4f)]
        
        for (Matrix, Matrixf) in Matrices:
            # Test comparison of Matrix and Matrixf
            #
            size = Matrix.dimension[0] * Matrix.dimension[1]
            contents = range(1, size + 1)
            md = Matrix(*contents)
            mf = Matrixf(*contents)
            self.assertEqual(md, mf)

            contents.reverse()
            md.Set(*contents)
            mf.Set(*contents)
            self.assertEqual(md, mf)

            # Convert to double precision floating point values
            contents = [1.0 / x for x in contents]
            mf.Set(*contents)
            md.Set(*contents)
            # These should *NOT* be equal due to roundoff errors in the floats.
            self.assertNotEqual(md, mf)

    def test_Other(self):
        Matrices = [(Gf.Matrix2d, Gf.Vec2d),
                    (Gf.Matrix2f, Gf.Vec2f),
                    (Gf.Matrix3d, Gf.Vec3d),
                    (Gf.Matrix3f, Gf.Vec3f),
                    (Gf.Matrix4d, Gf.Vec4d),
                    (Gf.Matrix4f, Gf.Vec4f)]

        for (Matrix, Vec) in Matrices:
            v = Vec()
            for i in range(v.dimension):
                v[i] = i
            m1 = Matrix().SetDiagonal(v)
            m2 = Matrix(0)
            for i in range(m2.dimension[0]):
                m2[i,i] = i
            self.assertEqual(m1, m2)

            v = Vec()
            for i in range(v.dimension):
                v[i] = 10
            self.assertEqual(Matrix().SetDiagonal(v), Matrix().SetDiagonal(10))

            self.assertEqual(type(Matrix()[0]), Vec)

            m = Matrix()
            m[0] = makeValue(Vec, (3,1,4,1))
            self.assertEqual(m[0], makeValue(Vec, (3,1,4,1)))

            m = Matrix()
            m[-1] = makeValue(Vec, (3,1,4,1))
            self.assertEqual(m[-1], makeValue(Vec, (3,1,4,1)))

            m = Matrix()
            m[0,0] = 1; m[1,0] = 2; m[0,1] = 3; m[1,1] = 4
            self.assertTrue(m[0,0] == 1 and m[1,0] == 2 and m[0,1] == 3 and m[1,1] == 4)
            
            m = Matrix()
            m[-1,-1] = 1; m[-2,-1] = 2; m[-1,-2] = 3; m[-2,-2] = 4
            self.assertTrue(m[-1,-1] == 1 and m[-2,-1] == 2 and m[-1,-2] == 3 and m[-2,-2] == 4)
            
            m = Matrix()
            for i in range(m.dimension[0]):
                for j in range(m.dimension[1]):
                    m[i,j] = i * m.dimension[1] + j
            m = m.GetTranspose()
            for i in range(m.dimension[0]):
                for j in range(m.dimension[1]):
                    self.assertEqual(m[j,i], i * m.dimension[1] + j)

            self.assertEqual(Matrix(1).GetInverse(), Matrix(1))
            self.assertEqual(Matrix(4).GetInverse() * Matrix(4), Matrix(1))
            self.assertNotEqual(Matrix(0).GetInverse(), Matrix(0))

            self.assertEqual(Matrix(3).GetDeterminant(), 3 ** Matrix.dimension[0])

            self.assertEqual(len(Matrix()), Matrix.dimension[0])
            
            # Test GetRow, GetRow3, GetColumn
            m = Matrix(1)
            for i in range(m.dimension[0]):
                for j in range(m.dimension[1]):
                    m[i,j] = i * m.dimension[1] + j
            for i in range(m.dimension[0]):
                j0 = i * m.dimension[1]
                self.assertEqual(m.GetRow(i), makeValue(Vec, tuple(range(j0,j0+m.dimension[1]))))
                if (Matrix == Gf.Matrix4d):
                    self.assertEqual(m.GetRow3(i), Gf.Vec3d(j0, j0+1, j0+2))

            for j in range(m.dimension[1]):
                self.assertEqual(m.GetColumn(j),
                    makeValue(Vec, tuple(j+x*m.dimension[0] \
                                            for x in range(m.dimension[0])) ))

            # Test SetRow, SetRow3, SetColumn
            m = Matrix(1)
            for i in range(m.dimension[0]):
                j0 = i * m.dimension[1]
                v = makeValue(Vec, tuple(range(j0,j0+m.dimension[1])))
                m.SetRow(i, v)
                self.assertEqual(v, m.GetRow(i))

            m = Matrix(1)
            if (Matrix == Gf.Matrix4d):
                for i in range(m.dimension[0]):
                    j0 = i * m.dimension[1]
                    v = Gf.Vec3d(j0, j0+1, j0+2)
                    m.SetRow3(i, v)
                    self.assertEqual(v, m.GetRow3(i))

            m = Matrix(1)
            for j in range(m.dimension[0]):
                v = makeValue(Vec, tuple(j+x*m.dimension[0] for x in range(m.dimension[0])) )
                m.SetColumn(i, v)
                self.assertEqual(v, m.GetColumn(i))
                    
            m = Matrix(4)
            m *= Matrix(1./4)
            self.assertEqual(m, Matrix(1))
            m = Matrix(4)
            self.assertEqual(m * Matrix(1./4), Matrix(1))

            self.assertEqual(Matrix(4) * 2, Matrix(8))
            self.assertEqual(2 * Matrix(4), Matrix(8))
            m = Matrix(4)
            m *= 2
            self.assertEqual(m, Matrix(8))

            m = Matrix(3)
            m += Matrix(2)
            self.assertEqual(m, Matrix(5))
            m = Matrix(3)
            m -= Matrix(2)
            self.assertEqual(m, Matrix(1))

            self.assertEqual(Matrix(2) + Matrix(3), Matrix(5))
            self.assertEqual(Matrix(4) - Matrix(4), Matrix(0))

            self.assertEqual(-Matrix(-1), Matrix(1))

            self.assertEqual(Matrix(3) / Matrix(2), Matrix(3) * Matrix(2).GetInverse())

            self.assertEqual(Matrix(2) * makeValue(Vec, (3,1,4,1)), makeValue(Vec, (6,2,8,2)))
            self.assertEqual(makeValue(Vec, (3,1,4,1)) * Matrix(2), makeValue(Vec, (6,2,8,2)))

            Vecf = { Gf.Vec2d: Gf.Vec2f,
                     Gf.Vec3d: Gf.Vec3f,
                     Gf.Vec4d: Gf.Vec4f }.get(Vec)
            if Vecf is not None:
                self.assertEqual(Matrix(2) * makeValue(Vecf, (3,1,4,1)), makeValue(Vecf, (6,2,8,2)))
                self.assertEqual(makeValue(Vecf, (3,1,4,1)) * Matrix(2), makeValue(Vecf, (6,2,8,2)))

            self.assertTrue(2 in Matrix(2) and not 4 in Matrix(2))

            m = Matrix(1)
            try:
                m[m.dimension[0]+1] = Vec()
            except:
                pass
            else:
                self.fail()

            m = Matrix(1)
            try:
                m[m.dimension[0]+1, m.dimension[1]+1] = 10
            except:
                pass
            else:
                self.fail()

            m = Matrix(1)
            try:
                m[1,1,1,1,1,1,1,1,1,1,1,1] = 3
                self.fail() 
            except:
                pass

            try:
                x = m[1,1,1,1,1,1,1,1,1,1,1,1]
                self.fail()
            except:
                pass

            m = Matrix(1)
            try:
                m['foo'] = 3
            except:
                pass
            else:
                self.fail()

            self.assertEqual(m, eval(repr(m)))

            self.assertTrue(len(str(Matrix())))

    def test_Matrix3Transforms(self):
        Matrices = [(Gf.Matrix3d, Gf.Vec3d),
                    (Gf.Matrix3f, Gf.Vec3f)]

        for (Matrix, Vec) in Matrices:
            def _VerifyOrthonormVecs(mat):
                v0 = Vec(mat[0][0], mat[0][1], mat[0][2])
                v1 = Vec(mat[1][0], mat[1][1], mat[1][2])
                v2 = Vec(mat[2][0], mat[2][1], mat[2][2])
                self.assertTrue(Gf.IsClose(Gf.Dot(v0,v1), 0, 0.0001) and \
                                Gf.IsClose(Gf.Dot(v0,v2), 0, 0.0001) and \
                                Gf.IsClose(Gf.Dot(v1,v2), 0, 0.0001))

            m = Matrix()
            m.SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), 30))
            m2 = Matrix(m)
            m_o = m.GetOrthonormalized()
            self.assertEqual(m_o, m2)
            m.Orthonormalize()
            self.assertEqual(m, m2)
            m = Matrix(3)
            m_o = m.GetOrthonormalized()
            # GetOrthonormalized() should not mutate m
            self.assertNotEqual(m , m_o)
            self.assertEqual(m_o, Matrix(1))
            m.Orthonormalize()
            self.assertEqual(m, Matrix(1))
                
            m = Matrix(1,0,0, 1,0,0, 1,0,0)
            # should print a warning
            print "expect a warning about failed convergence in OrthogonalizeBasis:"
            m.Orthonormalize()

            m = Matrix(1,0,0, 1,0,.0001, 0,1,0)
            m_o = m.GetOrthonormalized()
            _VerifyOrthonormVecs(m_o)
            m.Orthonormalize()
            _VerifyOrthonormVecs(m)

            r = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0),30)).ExtractRotation()
            r2 = Gf.Rotation(Gf.Vec3d(1,0,0), 30)
            self.assertTrue(Gf.IsClose(r.axis, r2.axis, 0.0001) and \
                            Gf.IsClose(r.angle, r2.angle, 0.0001))
                
            r = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1),60)).ExtractRotation()
            r2 = Gf.Rotation(Gf.Vec3d(1,1,1), 60)
            self.assertTrue(Gf.IsClose(r.axis, r2.axis, 0.0001) and \
                            Gf.IsClose(r.angle, r2.angle, 0.0001))

            r = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1),90)).ExtractRotation()
            r2 = Gf.Rotation(Gf.Vec3d(1,1,1), 90)
            self.assertTrue(Gf.IsClose(r.axis, r2.axis, 0.0001) and \
                            Gf.IsClose(r.angle, r2.angle, 0.0001))

            r = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1),120)).ExtractRotation()
            r2 = Gf.Rotation(Gf.Vec3d(1,1,1), 120)
            self.assertTrue(Gf.IsClose(r.axis, r2.axis, 0.0001) and \
                            Gf.IsClose(r.angle, r2.angle, 0.0001))

            self.assertEqual(Matrix().SetScale(10), Matrix(10))
            m = Matrix().SetScale(Vec(1,2,3))
            self.assertTrue(m[0,0] == 1 and m[1,1] == 2 and m[2,2] == 3)

            # Initializing with GfRotation should give same results as SetRotate.
            r = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0),30)).ExtractRotation()
            r2 = Matrix(Gf.Rotation(Gf.Vec3d(1,0,0),30)).ExtractRotation()
            self.assertTrue(Gf.IsClose(r.axis, r2.axis, 0.0001) and \
                            Gf.IsClose(r.angle, r2.angle, 0.0001))

            self.assertEqual(Matrix(1,0,0, 0,1,0, 0,0,1).GetHandedness(), 1.0)
            self.assertEqual(Matrix(-1,0,0, 0,1,0, 0,0,1).GetHandedness(), -1.0)
            self.assertEqual(Matrix(0,0,0, 0,0,0, 0,0,0).GetHandedness(), 0.0)
            self.assertTrue(Matrix(1,0,0, 0,1,0, 0,0,1).IsRightHanded())
            self.assertTrue(Matrix(-1,0,0, 0,1,0, 0,0,1).IsLeftHanded())


            # Test that this does not generate a nan in the angle (bug #12744)
            mx = Gf.Matrix3d(0.999999999982236, -5.00662622471027e-06, 2.07636574601397e-06,
                             5.00666175191934e-06, 1.0000000000332, -2.19113616402155e-07,
                             -2.07635686422463e-06, 2.19131379884019e-07, 1 )
            r = mx.ExtractRotation()
            # math.isnan won't be available until python 2.6
            if (sys.version_info[0] >=2 and sys.version_info[1] >= 6):
                self.assertFalse(math.isnan(r.angle))
            else:
                # If this fails, then r.angle is Nan. Works on linux, may not be portable.
                self.assertEqual(r.angle, r.angle)

    def test_Matrix4Transforms(self):
        Matrices = [(Gf.Matrix4d, Gf.Vec4d, Gf.Matrix3d, Gf.Vec3d),
                    (Gf.Matrix4f, Gf.Vec4f, Gf.Matrix3f, Gf.Vec3f)]

        for (Matrix, Vec, Matrix3, Vec3) in Matrices:
            def _VerifyOrthonormVecs(mat):
                v0 = Vec3(mat[0][0], mat[0][1], mat[0][2])
                v1 = Vec3(mat[1][0], mat[1][1], mat[1][2])
                v2 = Vec3(mat[2][0], mat[2][1], mat[2][2])
                self.assertTrue(Gf.IsClose(Gf.Dot(v0,v1), 0, 0.0001) and \
                    Gf.IsClose(Gf.Dot(v0,v2), 0, 0.0001) and \
                    Gf.IsClose(Gf.Dot(v1,v2), 0, 0.0001))

            m = Matrix()
            m.SetLookAt(Vec3(1,0,0), Vec3(0,0,1), Vec3(0,1,0))
            self.assertTrue(Gf.IsClose(m[0], Vec(-0.707107, 0, 0.707107, 0), 0.0001) and \
                            Gf.IsClose(m[1], Vec(0, 1, 0, 0), 0.0001) and \
                            Gf.IsClose(m[2], Vec(-0.707107, 0, -0.707107, 0), 0.0001) and \
                            Gf.IsClose(m[3], Vec(0.707107, 0, -0.707107, 1), 0.0001))

            # Transform
            v = Gf.Vec3f(1,1,1)
            v2 = Matrix(3).Transform(v)
            self.assertEqual(v2, Gf.Vec3f(1,1,1))
            self.assertEqual(type(v2), Gf.Vec3f)

            v = Gf.Vec3d(1,1,1)
            v2 = Matrix(3).Transform(v)
            self.assertEqual(v2, Gf.Vec3d(1,1,1))
            self.assertEqual(type(v2), Gf.Vec3d)

            # TransformDir
            v = Gf.Vec3f(1,1,1)
            v2 = Matrix(3).TransformDir(v)
            self.assertEqual(v2, Gf.Vec3f(3,3,3))
            self.assertEqual(type(v2), Gf.Vec3f)

            v = Gf.Vec3d(1,1,1)
            v2 = Matrix(3).TransformDir(v)
            self.assertEqual(v2, Gf.Vec3d(3,3,3))
            self.assertEqual(type(v2), Gf.Vec3d)

            # TransformAffine
            v = Gf.Vec3f(1,1,1)
            v2 = Matrix(3).SetTranslateOnly((1, 2, 3)).TransformAffine(v)
            self.assertEqual(v2, Gf.Vec3f(4,5,6))
            self.assertEqual(type(v2), Gf.Vec3f)

            v = Gf.Vec3d(1,1,1)
            v2 = Matrix(3).SetTranslateOnly((1, 2, 3)).TransformAffine(v)
            self.assertEqual(v2, Gf.Vec3d(4,5,6))
            self.assertEqual(type(v2), Gf.Vec3d)

            # Constructor, SetRotate, and SetRotateOnly w/GfRotation
            m = Matrix()
            r = Gf.Rotation(Gf.Vec3d(1,0,0), 30)
            m.SetRotate(r)
            m2 = Matrix(r, Vec3(0,0,0))
            m_o = m.GetOrthonormalized()
            self.assertEqual(m_o, m2)
            m.Orthonormalize()
            self.assertEqual(m, m2)
            m3 = Matrix(1)
            m3.SetRotateOnly(r)
            self.assertEqual(m2, m3)

            # Constructor, SetRotate, and SetRotateOnly w/mx
            m3d = Matrix3()
            m3d.SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), 30))
            m = Matrix(m3d, Vec3(0,0,0))
            m2 = Matrix()
            m2 = m2.SetRotate(m3d)
            m3 = Matrix()
            m3 = m2.SetRotateOnly(m3d)
            self.assertEqual(m, m2)
            self.assertEqual(m2, m3)

            m = Matrix().SetTranslate(Vec3(12,13,14)) * Matrix(3)
            m.Orthonormalize()
            t = Matrix().SetTranslate(m.ExtractTranslation())
            mnot = m * t.GetInverse()
            self.assertEqual(mnot, Matrix(1))

            m = Matrix()
            m.SetTranslate(Vec3(1,2,3))
            m2 = Matrix(m)
            m3 = Matrix(1)
            m3.SetTranslateOnly(Vec3(1,2,3))
            m_o = m.GetOrthonormalized()
            self.assertEqual(m_o, m2)
            self.assertEqual(m_o, m3)
            m.Orthonormalize()
            self.assertEqual(m, m2)
            self.assertEqual(m, m3)

            v = Vec3(11,22,33)
            m = Matrix(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16).SetTranslateOnly(v)
            self.assertEqual(m.ExtractTranslation(), v)

            # Initializing with GfRotation should give same results as SetRotate
            # and SetTransform
            r = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0),30)).ExtractRotation()
            r2 = Matrix(Gf.Rotation(Gf.Vec3d(1,0,0),30), Vec3(1,2,3)).ExtractRotation()
            r3 = Matrix().SetTransform(Gf.Rotation(Gf.Vec3d(1,0,0),30), Vec3(1,2,3)).ExtractRotation()
            self.assertTrue(Gf.IsClose(r.axis, r2.axis, 0.0001) and \
                Gf.IsClose(r.angle, r2.angle, 0.0001))
            self.assertTrue(Gf.IsClose(r3.axis, r2.axis, 0.0001) and \
                Gf.IsClose(r3.angle, r2.angle, 0.0001))

            # Same test w/mx instead of GfRotation
            mx3d = Matrix3(Gf.Rotation(Gf.Vec3d(1,0,0),30))
            r4 = Matrix().SetTransform(mx3d, Vec3(1,2,3)).ExtractRotation()
            r5 = Matrix(mx3d, Vec3(1,2,3)).ExtractRotation()
            self.assertTrue(Gf.IsClose(r4.axis, r2.axis, 0.0001) and \
                Gf.IsClose(r4.angle, r2.angle, 0.0001))
            self.assertTrue(Gf.IsClose(r4.axis, r5.axis, 0.0001) and \
                Gf.IsClose(r4.angle, r5.angle, 0.0001))

            m4 = Matrix(mx3d, Vec3(1,2,3)).ExtractRotationMatrix()
            self.assertEqual(m4, mx3d)


            # Initializing with GfMatrix3d
            m = Matrix(1,2,3,0,4,5,6,0,7,8,9,0,10,11,12,1)
            m2 = Matrix(Matrix3(1,2,3,4,5,6,7,8,9), Vec3(10, 11, 12))
            assert(m == m2)

            m = Matrix(1,0,0,0,  1,0,0,0,  1,0,0,0,  0,0,0,1)
            # should print a warning
            print "expect a warning about failed convergence in OrthogonalizeBasis:"
            m.Orthonormalize()

            m = Matrix(1,0,0,0,  1,0,.0001,0,  0,1,0,0,  0,0,0,1)
            m_o = m.GetOrthonormalized()
            _VerifyOrthonormVecs(m_o)
            m.Orthonormalize()
            _VerifyOrthonormVecs(m)

            m = Matrix()
            m[3,0] = 4
            m[3,1] = 5
            m[3,2] = 6
            self.assertEqual(m.ExtractTranslation(), Vec3(4,5,6))

            self.assertEqual(Matrix(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1).GetHandedness(), 1.0)
            self.assertEqual(Matrix(-1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1).GetHandedness(), -1.0)
            self.assertEqual(Matrix(0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0).GetHandedness(), 0.0)
            self.assertTrue(Matrix(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1).IsRightHanded())
            self.assertTrue(Matrix(-1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1).IsLeftHanded())

            # RemoveScaleShear
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), 45))
            m = Matrix().SetScale(Vec3(3,4,2)) * m
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), -30)) * m
            r = m.RemoveScaleShear()
            ro = r
            ro.Orthonormalize()
            self.assertEqual(ro, r)
            shear = Matrix(1,0,1,0, 0,1,0,0, 0,0,1,0, 0,0,0,1)
            r = shear.RemoveScaleShear()
            ro = r
            ro.Orthonormalize()
            self.assertEqual(ro, r)
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), 45))
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), -30)) * m
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,1,0), 59)) * m
            r = m.RemoveScaleShear()
            maxEltErr = 0
            for i in range(4):
                for j in range(4):
                    maxEltErr = max(maxEltErr, abs(r[i][j] - m[i][j]))
            self.assertTrue(Gf.IsClose(maxEltErr, 0.0, 1e-5))

    def test_Matrix4Factoring(self):
        Matrices = [(Gf.Matrix4d, Gf.Vec3d),
                    (Gf.Matrix4f, Gf.Vec3f)]
        for (Matrix, Vec3) in Matrices:
            def testFactor(m, expectSuccess, eps=None):
                factor = lambda m : m.Factor()
                if eps is not None:
                    factor = lambda m : m.Factor(eps)
                (success, scaleOrientation, scale, rotation, \
                 translation, projection) = factor(m)
                self.assertEqual(success, expectSuccess)
                factorProduct = scaleOrientation * \
                                Matrix().SetScale(scale) * \
                                scaleOrientation.GetInverse() * \
                                rotation * \
                                Matrix().SetTranslate(translation) * \
                                projection
                maxEltErr = 0
                for i in range(4):
                    for j in range(4):
                        maxEltErr = max(maxEltErr, abs(factorProduct[i][j] - m[i][j]))
                self.assertTrue(Gf.IsClose(maxEltErr, 0.0, 1e-5), maxEltErr)

            # A rotate
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), 45))
            testFactor(m,True)

            # A couple of rotates
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), -45))
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), 45)) * m
            testFactor(m,True)

            # A scale
            m = Matrix().SetScale(Vec3(3,1,4))
            testFactor(m,True)

            # A scale in a rotated space
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), 45))
            m = Matrix().SetScale(Vec3(3,4,2)) * m
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), -45)) * m
            testFactor(m,True)

            # A nearly degenerate scale
            if Matrix == Gf.Matrix4d:
                eps = 1e-10
            elif Matrix == Gf.Matrix4f:
                eps = 1e-5

            m = Matrix().SetScale((eps*2, 1, 1))
            testFactor(m,True)
            # Test with epsilon.
            m = Matrix().SetScale((eps*2, 1, 1))
            testFactor(m,False,eps*3)

            # A singular (1) scale in a rotated space
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1), Gf.Vec3d(1,0,0) ))
            m = m * Matrix().SetScale(Vec3(0,1,1))
            m = m * Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), Gf.Vec3d(1,1,1) ))
            testFactor(m,False)

            # A singular (2) scale in a rotated space
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1), Gf.Vec3d(1,0,0) ))
            m = m * Matrix().SetScale(Vec3(0,0,1))
            m = m * Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), Gf.Vec3d(1,1,1) ))
            testFactor(m,False)

            # A scale in a sheared space
            shear = Matrix(1)
            shear.SetRow3(0, Vec3(1, 1, 1).GetNormalized())
            m = shear.GetInverse() * Matrix().SetScale(Vec3(2,3,4)) * shear
            testFactor(m,True)

            # A singular (1) scale in a sheared space
            shear = Matrix(1)
            shear.SetRow3(0, Vec3(1, 1, 1).GetNormalized())
            m = shear.GetInverse() * Matrix().SetScale(Vec3(2,0,4)) * shear
            testFactor(m,False)

            # A singular (2) scale in a sheared space
            shear = Matrix(1)
            shear.SetRow3(0, Vec3(1, 1, 1).GetNormalized())
            m = shear.GetInverse() * Matrix().SetScale(Vec3(0,3,0)) * shear
            testFactor(m,False)

            # A scale and a rotate
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), Gf.Vec3d(1,1,-1) ))
            m = Matrix().SetScale(Vec3(1,2,3)) * m
            testFactor(m,True)

            # A singular (1) scale and a rotate
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), Gf.Vec3d(1,1,-1) ))
            m = Matrix().SetScale(Vec3(0,2,3)) * m
            testFactor(m,False)

            # A singular (2) scale and a rotate
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), Gf.Vec3d(1,1,-1) ))
            m = Matrix().SetScale(Vec3(0,0,3)) * m
            testFactor(m,False)

            # A singular scale (1), rotate, translate
            m = Matrix().SetTranslate(Vec3(3,1,4))
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), Gf.Vec3d(1,1,-1) )) * m
            m = Matrix().SetScale(Vec3(0,2,3)) * m
            testFactor(m,False)

            # A translate, rotate, singular scale (1), translate
            m = Matrix().SetTranslate(Vec3(3,1,4))
            m = Matrix().SetRotate(Gf.Rotation(Gf.Vec3d(0,0,1), Gf.Vec3d(1,1,-1) )) * m
            m = Matrix().SetScale(Vec3(0,2,3)) * m
            m = Matrix().SetTranslate(Vec3(-10,-20,-30)) * m
            testFactor(m,False)

    def test_Matrix4Determinant(self):
        Matrices = [Gf.Matrix4d,
                    Gf.Matrix4f]
        for Matrix in Matrices:
            # Test GetDeterminant and GetInverse on Matrix4
            def AssertDeterminant(m, det):
                # Unfortunately, we don't have an override of Gf.IsClose
                # for Gf.Matrix4*
                for row1, row2 in zip(m * m.GetInverse(), Matrix()):
                    self.assertTrue(Gf.IsClose(row1, row2, 1e-6))
                self.assertTrue(Gf.IsClose(m.GetDeterminant(), det, 1e-6))

            m1   = Matrix(0.0, 1.0, 0.0, 0.0,
                            1.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, 1.0, 0.0,
                            0.0, 0.0, 0.0, 1.0)
            det1 = -1.0

            m2   = Matrix(0.0, 0.0, 1.0, 0.0,
                            0.0, 1.0, 0.0, 0.0,
                            1.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, 0.0, 1.0)
            det2 = -1.0

            m3   = Matrix(0.0, 0.0, 0.0, 1.0,
                            0.0, 1.0, 0.0, 0.0,
                            0.0, 0.0, 1.0, 0.0,
                            1.0, 0.0, 0.0, 0.0)
            det3 = -1.0

            m4   = Matrix(1.0, 2.0,-1.0, 3.0,
                            2.0, 1.0, 4.0, 5.1,
                            2.0, 3.0, 1.0, 6.0,
                            3.0, 2.0, 1.0, 1.0)
            det4 = 16.8

            AssertDeterminant(m1, det1)
            AssertDeterminant(m2, det2)
            AssertDeterminant(m3, det3)
            AssertDeterminant(m4, det4)
            AssertDeterminant(m1 * m1, det1 * det1)
            AssertDeterminant(m1 * m4, det1 * det4)
            AssertDeterminant(m1 * m3 * m4, det1 * det3 * det4)
            AssertDeterminant(m1 * m3 * m4 * m2, det1 * det3 * det4 * det2)
            AssertDeterminant(m2 * m3 * m4 * m2, det2 * det3 * det4 * det2)

if __name__ == '__main__':
    unittest.main()
