#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Usd, UsdSkel, Vt, Sdf, Gf, Tf
import unittest, random


def MakeTargets(*paths):
    return [Sdf.Path(p) if p else Sdf.Path() for p in paths]


def MakeQuatfFromEulerAngles(rx, ry, rz):
    qd = (Gf.Rotation(Gf.Vec3d(1,0,0), rx)*
          Gf.Rotation(Gf.Vec3d(0,1,0), ry)*
          Gf.Rotation(Gf.Vec3d(0,0,1), rz)).GetQuat()
    i = qd.imaginary
    q = Gf.Quatf(qd.real, i[0], i[1], i[2])
    q.Normalize()
    return q


def RandomTranslations(count):
    return Vt.Vec3fArray(
        [Gf.Vec3f(random.random()-0.5,
                  random.random()-0.5,
                  random.random()-0.5)*200
         for _ in xrange(count)])


def RandomRotations(count):
    return Vt.QuatfArray(
        [MakeQuatfFromEulerAngles(
            (random.random()-0.5)*720,
            (random.random()-0.5)*720,
            (random.random()-0.5)*720)
         for _ in xrange(count)])
        


def RandomScales(count):
    # Flip some scales (tests variation in handedness)
    return Vt.Vec3hArray(
        [Gf.Vec3h(1 if random.random() > 0.5 else -1,
                  1 if random.random() > 0.5 else -1,
                  1 if random.random() > 0.5 else -1)
         for _ in xrange(count)])


class TestUsdSkelUtils(unittest.TestCase):

    
    def assertArrayIsClose(self, a, b, epsilon=1e-4):
        self.assertEqual(len(a), len(b))
        self.assertTrue(all(Gf.IsClose(ca,cb,epsilon)
                            for ca,cb in zip(a,b)))
    

    def test_TransformCompositionAndDecomposition(self):
        """
        Tests UsdSkelMakeTransform(), UsdSkelDecomposeTransform()
        and their array forms.
        """
        random.seed(0)

        count = 100

        # Make some random transforms.
        
        translations = RandomTranslations(count)
        rotations = RandomRotations(count)
        scales = RandomScales(count)

        xforms = UsdSkel.MakeTransforms(translations, rotations, scales)
        assert xforms

        xformsBuiltIndividually = Vt.Matrix4dArray(
            [UsdSkel.MakeTransform(translations[i], rotations[i], scales[i])
             for i in xrange(count)])

        self.assertArrayIsClose(xforms, xformsBuiltIndividually)

        # Decompose back into components.
        decomposedComponents = UsdSkel.DecomposeTransforms(xforms)
        assert decomposedComponents

        decomposedTranslations,_,_ = decomposedComponents

        self.assertArrayIsClose(translations, decomposedTranslations)
        
        # Make sure the non-array decomposition matches.
        for i,xf in enumerate(xforms):
            t,_,_ = UsdSkel.DecomposeTransform(xf)
            self.assertTrue(Gf.IsClose(t, translations[i],1e-5))

        # We can't really directly compare the decomposed rotations and scales
        # against the original components used to build our matrices:
        # Decomposition might produce different scales and rotations, but
        # which still give the same composed matrix result.
        # Instead, we rebuild xforms and validate that they give us the
        # xforms that the components were decomposed from.
        rebuiltXforms = UsdSkel.MakeTransforms(*decomposedComponents)
        assert rebuiltXforms

        self.assertArrayIsClose(xforms, rebuiltXforms)

        # And rebuilding each matrix individually...
        rebuiltXforms = Vt.Matrix4dArray(
            [UsdSkel.MakeTransform(*UsdSkel.DecomposeTransform(xf))
             for xf in xforms])

        self.assertArrayIsClose(xforms, rebuiltXforms)

        # Decomposing singular matrices should fail!
        with self.assertRaises(Tf.ErrorException):
            print "expect a warning about decomposing a singular matrix"
            UsdSkel.DecomposeTransform(Gf.Matrix4d(0))
        with self.assertRaises(Tf.ErrorException):
            UsdSkel.DecomposeTransforms(
                Vt.Matrix4dArray([Gf.Matrix4d(1), Gf.Matrix4d(0)]))


    def test_NormalizeWeights(self):
        """Test weight normalization."""

        # Basic weight normalization.
        weights = Vt.FloatArray([3,1,1,0, 4,3,2,1, 40,30,20,10, 0,0,0,0])

        assert UsdSkel.NormalizeWeights(weights, 4)

        self.assertEqual(weights,
                         Vt.FloatArray([0.6,0.2,0.2,0.0,
                                        0.4,0.3,0.2,0.1,
                                        0.4,0.3,0.2,0.1,
                                        0, 0, 0, 0]))

        # Should be okay to normalize an empty array.
        assert UsdSkel.NormalizeWeights(Vt.FloatArray(), 4)

        # Failure cases.
        print "expect a warning about an invalid array shape"
        assert not UsdSkel.NormalizeWeights(Vt.FloatArray([1]), 4)
        print "expect a warning about an invalid array shape"
        assert not UsdSkel.NormalizeWeights(Vt.FloatArray([1]), 0)


    def test_SortInfluences(self):
        """Test influence sorting."""

        indices = Vt.IntArray([1,2,3,4, 5,6,7,8, 9,10,11,12])
        weights = Vt.FloatArray([3,1,4,2, 1,2,3,4, 4,3,2,1])

        expectedIndices = Vt.IntArray([3,1,4,2, 8,7,6,5, 9,10,11,12])
        expectedWeights = Vt.FloatArray([4,3,2,1, 4,3,2,1, 4,3,2,1])

        assert UsdSkel.SortInfluences(indices, weights, 4)

        self.assertEqual(indices, expectedIndices)
        self.assertEqual(weights, expectedWeights)
        
        # Check case of numInfluencesPerComponent=1
        indices = Vt.IntArray([1,2,3,4])
        weights = Vt.FloatArray([3,1,4,2])
        
        assert UsdSkel.SortInfluences(indices, weights, 1)
        
        assert indices == Vt.IntArray([1,2,3,4])
        assert weights == Vt.FloatArray([3,1,4,2])


    def test_ResizeInfluences(self):
        """Test influence resizing."""

        origIndices = Vt.IntArray([1,2,3,4,5,6,7,8])

        # truncation
        indices = Vt.IntArray(origIndices)
        assert UsdSkel.ResizeInfluences(indices, 4, 2)
        self.assertEqual(indices, Vt.IntArray([1,2,5,6]))
        
        # expansion
        indices = Vt.IntArray(origIndices)
        assert UsdSkel.ResizeInfluences(indices, 2, 4)
        self.assertEqual(indices, Vt.IntArray([1,2,0,0, 3,4,0,0,
                                               5,6,0,0, 7,8,0,0]))

        # Weight truncation. Weights should be renormalized.
        weights = Vt.FloatArray([3,1,1,5,6, 1,1,3,7,8, 1,3,1,9,10])
        assert UsdSkel.ResizeInfluences(weights, 5, 3)
        self.assertEqual(weights, Vt.FloatArray([0.6,0.2,0.2,
                                                 0.2,0.2,0.6,
                                                 0.2,0.6,0.2]))
        
        # Weight expansion. Holes must be zero-filled.
        weights = Vt.FloatArray([1,2,3,4,5,6,7,8,9])
        assert UsdSkel.ResizeInfluences(weights, 3, 5)
        self.assertEqual(weights, Vt.FloatArray([1,2,3,0,0,
                                                 4,5,6,0,0,
                                                 7,8,9,0,0]))

        # Should be okay to resize an empty array.
        array = Vt.IntArray()
        assert UsdSkel.ResizeInfluences(array, 10, 4)
        assert UsdSkel.ResizeInfluences(array, 4, 10)
        array = Vt.FloatArray()
        assert UsdSkel.ResizeInfluences(array, 10, 4)
        assert UsdSkel.ResizeInfluences(array, 4, 10)

        # Failure cases
        array = Vt.FloatArray([1])
        print "expect a warning about an invalid array shape"
        assert not UsdSkel.ResizeInfluences(array, 3, 2)
        print "expect a warning about an invalid array shape"
        assert not UsdSkel.ResizeInfluences(array, 3, 0)


    def test_ExpandConstantInfluencesToVarying(self):
        """Test expansion of constant influences into varying influences."""
        
        vals = [1,2,3,4,5]
        
        for vtType in (Vt.IntArray, Vt.FloatArray):
            array = vtType(vals)
            assert UsdSkel.ExpandConstantInfluencesToVarying(array, 3)
            self.assertEquals(array, vtType(vals*3))

            array = vtType(vals)
            assert UsdSkel.ExpandConstantInfluencesToVarying(array, 0)
            self.assertEquals(array, vtType())

            # Empty array case.
            array = vtType()
            assert UsdSkel.ExpandConstantInfluencesToVarying(array, 3)
            self.assertEquals(array, vtType())


    def test_ComputeJointLocalTransforms(self):
        # TODO
        pass


    def test_ConcatJointTransforms(self):
        # TODO
        pass


    def test_ComputeJointsExtent(self):
        # TODO
        pass


    def test_PosePointsLBS(self):
        # TODO
        pass


if __name__ == "__main__":
    unittest.main()
