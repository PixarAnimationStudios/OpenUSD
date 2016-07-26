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

import sys, math, unittest
from pxr import Gf

vecIntTypes = [ Gf.Vec2i, Gf.Vec3i, Gf.Vec4i ]
vecDoubleTypes = [ Gf.Vec2d, Gf.Vec3d, Gf.Vec4d ]
vecFloatTypes = [ Gf.Vec2f, Gf.Vec3f, Gf.Vec4f ]
vecHalfTypes = [ Gf.Vec2h, Gf.Vec3h, Gf.Vec4h ]

def floatTypeRank(vec):
    if vec in vecDoubleTypes:
        return 3
    elif vec in vecFloatTypes:
        return 2
    elif vec in vecHalfTypes:
        return 1

def isFloatingPoint(vec):
    return ((vec in vecDoubleTypes) or
            (vec in vecFloatTypes) or
            (vec in vecHalfTypes))

def getEps(vec):
    rank = floatTypeRank(vec)
    if rank == 1:
        return 1e-2
    elif rank == 2:
        return 1e-3
    elif rank == 3:
        return 1e-4

def vecWithType( vecType, type ):
    if vecType.dimension == 2:
        if type == 'd':
            return Gf.Vec2d
        elif type == 'f':
            return Gf.Vec2f
        elif type == 'h':
            return Gf.Vec2h
        elif type == 'i':
            return Gf.Vec2i
    elif vecType.dimension == 3:
        if type == 'd':
            return Gf.Vec3d
        elif type == 'f':
            return Gf.Vec3f
        elif type == 'h':
            return Gf.Vec3h
        elif type == 'i':
            return Gf.Vec3i
    elif vecType.dimension == 4:
        if type == 'd':
            return Gf.Vec4d
        elif type == 'f':
            return Gf.Vec4f
        elif type == 'h':
            return Gf.Vec4h
        elif type == 'i':
            return Gf.Vec4i
    assert False, "No valid conversion for " + vecType + " to type " + type
    return None


def checkVec( vec, values ):
    for i in range(len(vec)):
        if vec[i] != values[i]:
            return False
    return True

def checkVecDot( v1, v2, dp ):
    if len(v1) != len(v2):
        return False
    checkdp = 0
    for i in range(len(v1)):
        checkdp += v1[i] * v2[i]
    return checkdp == dp


def SetVec( vec, values ):
    for i in range(len(vec)):
        vec[i] = values[i]

class TestGfVec(unittest.TestCase):

    def ConstructorsTest(self, Vec):
        # no arg constructor
        self.assertIsInstance(Vec(), Vec)
        # default constructor
        v = Vec()
        for x in v:
            self.assertEqual(0, x)
        # copy constructor
        v = Vec()
        for i in range(len(v)):
            v[i] = i
        v2 = Vec(v)
        for i in range(len(v2)):
            self.assertEqual(v[i], v2[i])
        # explicit constructor
        values = [3, 1, 4, 1]
        if Vec.dimension == 2:
            v = Vec(3,1)
            self.assertTrue(checkVec( v, values ))
        elif Vec.dimension == 3:
            v = Vec(3,1,4)
            self.assertTrue(checkVec( v, values ))
        elif Vec.dimension == 4:
            v = Vec(3,1,4,1)
            self.assertTrue(checkVec( v, values ))
        else:
            self.assertTrue(False, "No explicit constructor check for " + Vec)

        # constructor taking single scalar value.
        v = Vec(0)
        self.assertTrue(all([x == 0 for x in v]))
        v = Vec(1)
        self.assertTrue(all([x == 1 for x in v]))
        v = Vec(2)
        self.assertTrue(all([x == 2 for x in v]))

        # conversion from other types to this float type.
        if isFloatingPoint(Vec):
            for t in 'dfhi':
                V = vecWithType(Vec, t)
                self.assertTrue(Vec(V()))

        # comparison to int type
        Veci = vecWithType(Vec, 'i')
        vi = Veci()
        SetVec(vi, (3, 1, 4, 1))
        self.assertEqual(Vec(vi), vi)

        if isFloatingPoint(Vec):
            # Comparison to float type
            for t in 'dfh':
                V = vecWithType(Vec, t)
                v = V()
                SetVec(v, (0.3, 0.1, 0.4, 0.1))
                if floatTypeRank(Vec) >= floatTypeRank(V):
                    self.assertEqual(Vec(v), v)
                else:
                    self.assertNotEqual(Vec(v), v)

    def OperatorsTest(self, Vec):
        v1 = Vec()
        v2 = Vec()

        # equality
        SetVec( v1, [3, 1, 4, 1] )
        SetVec( v2, [3, 1, 4, 1] )
        self.assertEqual(v1, v2)

        # inequality
        SetVec( v1, [3, 1, 4, 1] )
        SetVec( v2, [5, 9, 2, 6] )
        self.assertNotEqual(v1, v2)

        # component-wise addition
        SetVec( v1, [3, 1, 4, 1] )
        SetVec( v2, [5, 9, 2, 6] )
        v3 = v1 + v2
        v1 += v2
        self.assertTrue(checkVec( v1, [8, 10, 6, 7] ))
        self.assertTrue(checkVec( v3, [8, 10, 6, 7] ))

        # component-wise subtraction
        SetVec( v1, [3, 1, 4, 1] )
        SetVec( v2, [5, 9, 2, 6] )
        v3 = v1 - v2
        v1 -= v2
        self.assertTrue(checkVec( v1, [-2, -8, 2, -5] ))
        self.assertTrue(checkVec( v3, [-2, -8, 2, -5] ))

        # component-wise multiplication
        SetVec( v1, [3, 1, 4, 1] )
        SetVec( v2, [5, 9, 2, 6] )
        v4 = v1 * 10
        v5 = 10 * v1
        v1 *= 10
        self.assertTrue(checkVec( v1, [30, 10, 40, 10] ))
        self.assertTrue(checkVec( v4, [30, 10, 40, 10] ))
        self.assertTrue(checkVec( v5, [30, 10, 40, 10] ))

        # component-wise division
        SetVec( v1, [3, 6, 9, 12] )
        v3 = v1 / 3
        v1 /= 3
        self.assertTrue(checkVec( v1, [1, 2, 3, 4] ))
        self.assertTrue(checkVec( v3, [1, 2, 3, 4] ))

        # dot product
        SetVec( v1, [3, 1, 4, 1] )
        SetVec( v2, [5, 9, 2, 6] )
        dp = v1 * v2
        dp2 = Gf.Dot(v1, v2)
        dp3 = v1.GetDot(v2) # 2x compatibility
        self.assertTrue(checkVecDot( v1, v2, dp ))
        self.assertTrue(checkVecDot( v1, v2, dp2 ))
        self.assertTrue(checkVecDot( v1, v2, dp3 ))

        # unary minus (negation)
        SetVec(v1, [3, 1, 4, 1])
        self.assertTrue(checkVec(-v1, [-3, -1, -4, -1]))

        # repr
        self.assertEqual(v1, eval(repr(v1)))

        # string
        self.assertTrue(len(str(Vec())) > 0)

        # indexing
        v = Vec()
        for i in range(Vec.dimension):
            v[i] = i + 1

        self.assertEqual(v[-1], v[v.dimension-1])
        self.assertEqual(v[0], 1)
        self.assertIn(v.dimension, v)
        self.assertNotIn(v.dimension+1, v)

        with self.assertRaises(IndexError):
            v[v.dimension+1] = v.dimension + 1

        # slicing
        v = Vec()
        value = [3, 1, 4, 1]
        SetVec( v, value )
        value = v[0:v.dimension]

        self.assertEqual(v[:], value)
        self.assertEqual(v[:2], value[:2])
        self.assertEqual(v[0:2], value[0:2])
        self.assertEqual(v[-2:], value[-2:])
        self.assertEqual(v[1:1], [])

        if v.dimension > 2:
            self.assertEqual(v[0:3:2], [3, 4])

        v[:2] = (8, 9)
        checkVec(v, [8, 9, 4, 1])

        if v.dimension > 2:
            v[:3:2] = [0, 1]
            checkVec(v, [0, 9, 1, 1])

        with self.assertRaises(ValueError):
            # This should fail.  Wrong length sequence
            #
            v[:2] = [1, 2, 3]

        with self.assertRaises(TypeError):
            # This should fail.  Cannot use floats for indices
            v[0.0:2.0] = [7, 7]

        with self.assertRaises(TypeError):
            # This should fail.  Cannot convert None to vector data
            #
            v[:2] = [None, None]

    def MethodsTest(self, Vec):
        v1 = Vec()
        v2 = Vec()
        if isFloatingPoint(Vec):
            eps = getEps(Vec)

            # length
            SetVec( v1, [3, 1, 4, 1] )
            l = Gf.GetLength( v1 )
            l2 = v1.GetLength()
            self.assertTrue(Gf.IsClose(l, l2, eps), repr(l) + ' ' + repr(l2))
            self.assertTrue(Gf.IsClose(l, math.sqrt(Gf.Dot(v1, v1)), eps), \
                ' '.join([repr(x) for x in [l, v1, math.sqrt(Gf.Dot(v1, v1))]]))

            # Normalize...
            SetVec( v1, [3, 1, 4, 1] )
            v2 = Vec(v1);
            v2.Normalize()
            nv = Gf.GetNormalized( v1 )
            nv2 = v1.GetNormalized()
            nvcheck = v1 / Gf.GetLength(v1);
            self.assertTrue(Gf.IsClose(nv, nvcheck, eps))
            self.assertTrue(Gf.IsClose(nv2, nvcheck, eps))
            self.assertTrue(Gf.IsClose(v2, nvcheck, eps))

            SetVec( v1, [3, 1, 4, 1] )
            nv = v1.GetNormalized()
            nvcheck = v1 / Gf.GetLength(v1);
            self.assertEqual(nv, nvcheck)

            SetVec(v1, [0,0,0,0])
            v1.Normalize()
            self.assertTrue(checkVec(v1, [0,0,0,0]))

            SetVec(v1, [2,0,0,0])
            Gf.Normalize(v1)
            self.assertTrue(checkVec(v1, [1,0,0,0]))

            # projection
            SetVec( v1, [3, 1, 4, 1] )
            SetVec( v2, [5, 9, 2, 6] )
            p1 = Gf.GetProjection( v1, v2 )
            p2 = v1.GetProjection( v2 )
            check = (v1 * v2) * v2
            self.assertTrue((p1 == check) and (p2 == check))

            # complement
            SetVec( v1, [3, 1, 4, 1] )
            SetVec( v2, [5, 9, 2, 6] )
            p1 = Gf.GetComplement( v1, v2 )
            p2 = v1.GetComplement( v2 )
            check = v1 - (v1 * v2) * v2
            self.assertTrue((p1 == check) and (p2 == check))

            # component-wise multiplication
            SetVec( v1, [3, 1, 4, 1] )
            SetVec( v2, [5, 9, 2, 6] )
            v3 = Gf.CompMult( v1, v2 )
            self.assertTrue(checkVec( v3, [15, 9, 8, 6] ))

            # component-wise division
            SetVec( v1, [3, 9, 18, 21] )
            SetVec( v2, [3, 3, 9, 7] )
            v3 = Gf.CompDiv( v1, v2 )
            self.assertTrue(checkVec( v3, [1, 3, 2, 3] ))

            # is close
            SetVec( v1, [3, 1, 4, 1] )
            SetVec( v2, [5, 9, 2, 6] )
            self.assertTrue(Gf.IsClose( v1, v1, 0.1 ))
            self.assertFalse(Gf.IsClose( v1, v2, 0.1 ))

            # static Axis methods
            for i in range(Vec.dimension):
                v1 = Vec.Axis(i)
                v2 = Vec()
                v2[i] = 1;
                self.assertEqual(v1, v2)

            v1 = Vec.XAxis()
            self.assertTrue(checkVec( v1, [1, 0, 0, 0] ))

            v1 = Vec.YAxis()
            self.assertTrue(checkVec( v1, [0, 1, 0, 0] ))

            if Vec.dimension != 2:
                v1 = Vec.ZAxis()
                self.assertTrue(checkVec( v1, [0, 0, 1, 0] ))

            if Vec.dimension == 3:
                # cross product
                SetVec( v1, [3, 1, 4, 0] )
                SetVec( v2, [5, 9, 2, 0] )
                v3 = Vec(Gf.Cross( v1, v2 ))
                v4 = v1 ^ v2
                v5 = v1.GetCross(v2) # 2x compatibility
                check = Vec()
                SetVec( check, [1 * 2 - 4 * 9,
                                4 * 5 - 3 * 2,
                                3 * 9 - 1 * 5,
                                0] )
                self.assertTrue(v3 == check and v4 == check and v5 == check)

                # orthogonalize basis

                # case 1: close to orthogonal, don't normalize
                SetVec( v1, [1,0,0.1] )
                SetVec( v2, [0.1,1,0] )
                SetVec( v3, [0,0.1,1] )
                self.assertTrue(Vec.OrthogonalizeBasis(v1,v2,v3,False))
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v3,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v3), 0, eps))

                # case 2: far from orthogonal, normalize
                SetVec( v1, [1,2,3] )
                SetVec( v2, [-1,2,3] )
                SetVec( v3, [-1,-2,3] )
                self.assertTrue(Vec.OrthogonalizeBasis(v1,v2,v3,True))
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v3,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v3), 0, eps))
                self.assertTrue(Gf.IsClose(v1.GetLength(), 1, eps))
                self.assertTrue(Gf.IsClose(v2.GetLength(), 1, eps))
                self.assertTrue(Gf.IsClose(v3.GetLength(), 1, eps))

                # case 3: already orthogonal - shouldn't change, even with large
                # tolerance
                SetVec( v1, [1,0,0] )
                SetVec( v2, [0,1,0] )
                SetVec( v3, [0,0,1] )
                vt1 = v1
                vt2 = v2
                vt3 = v3
                self.assertTrue(Vec.OrthogonalizeBasis(v1,v2,v3,False,1.0))
                self.assertTrue(v1 == vt1)
                self.assertTrue(v2 == vt2)
                self.assertTrue(v3 == vt3)

                # case 4: co-linear input vectors - should do nothing
                SetVec( v1, [1,0,0] )
                SetVec( v2, [1,0,0] )
                SetVec( v3, [0,0,1] )
                vt1 = v1
                vt2 = v2
                vt3 = v3
                self.assertFalse(Vec.OrthogonalizeBasis(v1,v2,v3,False,1.0))
                self.assertEqual(v1, vt1)
                self.assertEqual(v2, vt2)
                self.assertEqual(v3, vt3)

                # build orthonormal frame
                SetVec( v1, [1,1,1,1] )
                (v2, v3) = v1.BuildOrthonormalFrame()
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v3,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v3), 0, eps))

                SetVec( v1, [0,0,0,0] )
                (v2, v3) = v1.BuildOrthonormalFrame()
                self.assertTrue(Gf.IsClose(v2, Vec(), eps))
                self.assertTrue(Gf.IsClose(v3, Vec(), eps))

                SetVec( v1, [1,0,0,0] )
                (v2, v3) = v1.BuildOrthonormalFrame()
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v3,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v3), 0, eps))

                SetVec( v1, [1,0,0,0] )
                (v2, v3) = v1.BuildOrthonormalFrame()
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v3,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v3), 0, eps))

                SetVec( v1, [1,0,0,0] )
                (v2, v3) = v1.BuildOrthonormalFrame(2)
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v3,v1), 0, eps))
                self.assertTrue(Gf.IsClose(Gf.Dot(v2,v3), 0, eps))

                # test Slerp w/ orthogonal vectors
                SetVec( v1, [1,0,0] )
                SetVec( v2, [0,1,0] )

                v3 = Gf.Slerp(0, v1, v2)
                self.assertTrue(Gf.IsClose(v3, v1, eps))

                v3 = Gf.Slerp(1, v1, v2)
                self.assertTrue(Gf.IsClose(v3, v3, eps))

                v3 = Gf.Slerp(0.5, v1, v2)
                self.assertTrue(Gf.IsClose(v3, Vec(.7071, .7071, 0), eps))

                # test Slerp w/ nearly parallel vectors
                SetVec( v1, [1,0,0] )
                SetVec( v2, [1.001,0.0001,0] )
                v2.Normalize()

                v3 = Gf.Slerp(0, v1, v2)
                self.assertTrue(Gf.IsClose(v3, v1, eps))

                v3 = Gf.Slerp(1, v1, v2)
                self.assertTrue(Gf.IsClose(v3, v3, eps))

                v3 = Gf.Slerp(0.5, v1, v2)
                self.assertTrue(Gf.IsClose(v3, v1, eps), [v3, v1, eps])
                self.assertTrue(Gf.IsClose(v3, v2, eps))

                # test Slerp w/ opposing vectors
                SetVec( v1, [1,0,0] )
                SetVec( v2, [-1,0,0] )

                v3 = Gf.Slerp(0, v1, v2)
                self.assertTrue(Gf.IsClose(v3, v1, eps))
                v3 = Gf.Slerp(0.25, v1, v2)
                self.assertTrue(Gf.IsClose(v3, Vec(.70711, 0, -.70711), eps))
                v3 = Gf.Slerp(0.5, v1, v2)
                self.assertTrue(Gf.IsClose(v3, Vec(0, 0, -1), eps))
                v3 = Gf.Slerp(0.75, v1, v2)
                self.assertTrue(Gf.IsClose(v3, Vec(-.70711, 0, -.70711), eps))
                v3 = Gf.Slerp(1, v1, v2)
                self.assertTrue(Gf.IsClose(v3, v3, eps))

                # test Slerp w/ opposing vectors
                SetVec( v1, [0,1,0] )
                SetVec( v2, [0,-1,0] )

                v3 = Gf.Slerp(0, v1, v2)
                self.assertTrue(Gf.IsClose(v3, v1, eps))
                v3 = Gf.Slerp(0.25, v1, v2)
                self.assertTrue(Gf.IsClose(v3, Vec(0, .70711, .70711), eps))
                v3 = Gf.Slerp(0.5, v1, v2)
                self.assertTrue(Gf.IsClose(v3, Vec(0, 0, 1), eps))
                v3 = Gf.Slerp(0.75, v1, v2)
                self.assertTrue(Gf.IsClose(v3, Vec(0, -.70711, .70711), eps))
                v3 = Gf.Slerp(1, v1, v2)
                self.assertTrue(Gf.IsClose(v3, v3, eps))

    def test_Types(self):
        vecTypes = [Gf.Vec2d,
                    Gf.Vec2f,
                    Gf.Vec2h,
                    Gf.Vec2i,
                    Gf.Vec3d,
                    Gf.Vec3f,
                    Gf.Vec3h,
                    Gf.Vec3i,
                    Gf.Vec4d,
                    Gf.Vec4f,
                    Gf.Vec4h,
                    Gf.Vec4i]

        for Vec in vecTypes:
            self.ConstructorsTest( Vec )
            self.OperatorsTest( Vec )
            self.MethodsTest( Vec )


    def test_TupleToVec(self):
        # Test passing tuples for vecs.
        self.assertEqual(Gf.Dot((1,1), (1,1)), 2)
        self.assertEqual(Gf.Dot((1,1,1), (1,1,1)), 3)
        self.assertEqual(Gf.Dot((1,1,1,1), (1,1,1,1)), 4)

        self.assertEqual(Gf.Dot((1.0,1.0), (1.0,1.0)), 2.0)
        self.assertEqual(Gf.Dot((1.0,1.0,1.0), (1.0,1.0,1.0)), 3.0)
        self.assertEqual(Gf.Dot((1.0,1.0,1.0,1.0), (1.0,1.0,1.0,1.0)), 4.0)

        self.assertEqual(Gf.Vec2f((1,1)), Gf.Vec2f(1,1))
        self.assertEqual(Gf.Vec3f((1,1,1)), Gf.Vec3f(1,1,1))
        self.assertEqual(Gf.Vec4f((1,1,1,1)), Gf.Vec4f(1,1,1,1))

        # Test passing lists for vecs.
        self.assertEqual(Gf.Dot([1,1], [1,1]), 2)
        self.assertEqual(Gf.Dot([1,1,1], [1,1,1]), 3)
        self.assertEqual(Gf.Dot([1,1,1,1], [1,1,1,1]), 4)

        self.assertEqual(Gf.Dot([1.0,1.0], [1.0,1.0]), 2.0)
        self.assertEqual(Gf.Dot([1.0,1.0,1.0], [1.0,1.0,1.0]), 3.0)
        self.assertEqual(Gf.Dot([1.0,1.0,1.0,1.0], [1.0,1.0,1.0,1.0]), 4.0)

        self.assertEqual(Gf.Vec2f([1,1]), Gf.Vec2f(1,1))
        self.assertEqual(Gf.Vec3f([1,1,1]), Gf.Vec3f(1,1,1))
        self.assertEqual(Gf.Vec4f([1,1,1,1]), Gf.Vec4f(1,1,1,1))

        # Test passing both for vecs.
        self.assertEqual(Gf.Dot((1,1), [1,1]), 2)
        self.assertEqual(Gf.Dot((1,1,1), [1,1,1]), 3)
        self.assertEqual(Gf.Dot((1,1,1,1), [1,1,1,1]), 4)

        self.assertEqual(Gf.Dot((1.0,1.0), [1.0,1.0]), 2.0)
        self.assertEqual(Gf.Dot((1.0,1.0,1.0), [1.0,1.0,1.0]), 3.0)
        self.assertEqual(Gf.Dot((1.0,1.0,1.0,1.0), [1.0,1.0,1.0,1.0]), 4.0)

        self.assertEqual(Gf.Vec2f([1,1]), Gf.Vec2f(1,1))
        self.assertEqual(Gf.Vec3f([1,1,1]), Gf.Vec3f(1,1,1))
        self.assertEqual(Gf.Vec4f([1,1,1,1]), Gf.Vec4f(1,1,1,1))

    def test_Exceptions(self):
        with self.assertRaises(TypeError):
            Gf.Dot('monkey', (1, 1))
        with self.assertRaises(TypeError):
            Gf.Dot('monkey', (1, 1, 1))
        with self.assertRaises(TypeError):
            Gf.Dot('monkey', (1, 1, 1, 1))

        with self.assertRaises(TypeError):
            Gf.Dot('monkey', (1.0, 1.0))
        with self.assertRaises(TypeError):
            Gf.Dot('monkey', (1.0, 1.0, 1.0))
        with self.assertRaises(TypeError):
            Gf.Dot('monkey', (1.0, 1.0, 1.0, 1.0))


        with self.assertRaises(TypeError):
            Gf.Dot((1, 1, 1, 1, 1, 1), (1, 1))
        with self.assertRaises(TypeError):
            Gf.Dot((1, 1, 1, 1, 1, 1), (1, 1, 1))
        with self.assertRaises(TypeError):
            Gf.Dot((1, 1, 1, 1, 1, 1), (1, 1, 1, 1))

        with self.assertRaises(TypeError):
            Gf.Dot((1.0, 1.0, 1.0, 1.0, 1.0, 1.0), (1.0, 1.0))
        with self.assertRaises(TypeError):
            Gf.Dot((1.0, 1.0, 1.0, 1.0, 1.0, 1.0), (1.0, 1.0, 1.0))
        with self.assertRaises(TypeError):
            Gf.Dot((1.0, 1.0, 1.0, 1.0, 1.0, 1.0), (1.0, 1.0, 1.0, 1.0))

        with self.assertRaises(TypeError):
            Gf.Dot(('a', 'b'), (1, 1))
        with self.assertRaises(TypeError):
            Gf.Dot(('a', 'b', 'c'), (1, 1, 1))
        with self.assertRaises(TypeError):
            Gf.Dot(('a', 'b', 'c', 'd'), (1, 1, 1, 1))

        with self.assertRaises(TypeError):
            Gf.Dot(('a', 'b'), (1.0, 1.0))
        with self.assertRaises(TypeError):
            Gf.Dot(('a', 'b', 'c'), (1.0, 1.0, 1.0))
        with self.assertRaises(TypeError):
            Gf.Dot(('a', 'b', 'c', 'd'), (1.0, 1.0, 1.0, 1.0))

if __name__ == '__main__':
    unittest.main()

