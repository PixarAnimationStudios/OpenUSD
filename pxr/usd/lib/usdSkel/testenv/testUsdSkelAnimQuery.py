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

from pxr import Usd, UsdSkel, Gf, Sdf, Tf, Vt
import unittest, random


def _RandomXf():
    return Gf.Matrix4d(Gf.Rotation(Gf.Vec3d(1,0,0),
                                   random.random()*360)*
                       Gf.Rotation(Gf.Vec3d(0,1,0),
                                   random.random()*360)*
                       Gf.Rotation(Gf.Vec3d(0,0,1),
                                   random.random()*360),
                       Gf.Vec3d((random.random()-0.5)*10,
                                (random.random()-0.5)*10,
                                (random.random()-0.5)*10))


class TestUsdSkelAnimQuery(unittest.TestCase):


    def assertArrayIsClose(self, a, b, epsilon=1e-5):
        self.assertEqual(len(a), len(b))
        self.assertTrue(all(Gf.IsClose(ca,cb,epsilon)
                            for ca,cb in zip(a,b)))

        
    def test_SkelAnimation(self):
        """
        Tests anim query implementation for SkelAnimation.
        """
        self._TestSkelAnimation(UsdSkel.Animation)


    def test_PackedJointAnimation(self):
        """
        Test for backwards-compatibility with the deprecated
        UsdSkelPackedJointAnimation schema.
        """
        self._TestSkelAnimation(UsdSkel.PackedJointAnimation)

        
    def _TestSkelAnimation(self, animSchema):

        numFrames = 10
        random.seed(0)

        stage = Usd.Stage.CreateInMemory()

        anim = animSchema.Define(stage, "/Anim")

        joints = Vt.TokenArray(["/A", "/B", "/C"])
        blendshapes = Vt.TokenArray(["shapeA", "shapeB", "shapeC"])

        anim.GetJointsAttr().Set(joints)
        anim.GetBlendShapesAttr().Set(blendshapes)

        xformsPerFrame = [[_RandomXf() for _ in xrange(len(joints))]
                          for _ in xrange(numFrames)]

        for frame,xforms in enumerate(xformsPerFrame):
            t,r,s = UsdSkel.DecomposeTransforms(Vt.Matrix4dArray(xforms))
            anim.GetTranslationsAttr().Set(t, frame)
            anim.GetRotationsAttr().Set(r, frame)
            anim.GetScalesAttr().Set(s, frame)

        weightsPerFrame = [[random.random() for _ in xrange(len(blendshapes))]
                           for _ in xrange(numFrames)]
        
        for frame,weights in enumerate(weightsPerFrame):
            anim.GetBlendShapeWeightsAttr().Set(Vt.FloatArray(weights), frame)

        # Now try reading that all back via computations...

        cache = UsdSkel.Cache()

        query = cache.GetAnimQuery(anim.GetPrim())

        self.assertEqual(query.GetPrim(), anim.GetPrim())
        self.assertEqual(query.GetJointOrder(), joints)
        self.assertEqual(query.GetBlendShapeOrder(), blendshapes)
        self.assertTrue(query.JointTransformsMightBeTimeVarying())
        self.assertEqual(query.GetJointTransformTimeSamples(),
                         list(xrange(numFrames)))

        for frame,xforms in enumerate(xformsPerFrame):

            computedXforms = query.ComputeJointLocalTransforms(frame)
            self.assertArrayIsClose(computedXforms, xforms)

        for frame,weights in enumerate(weightsPerFrame):
            computedWeights = query.ComputeBlendShapeWeights(frame)
            self.assertArrayIsClose(computedWeights, weights)


if __name__ == "__main__":
    unittest.main()
